#include <Windows.h>
#include <bcrypt.h>
#include <SPF_Plugin.h>
#include <SPF_Manifest_API.h>
#include <SPF_Logger_API.h>
#include <SPF_Telemetry_API.h>
#include <SPF_TelemetryData.h>
#include "DriverSamples.hpp"
#include "SteeringPose.hpp"
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <vector>

extern "C" {
void* gDriverOriginal=nullptr;
void* gAdvanceOriginal=nullptr;
void DriverObserveStub();
void AdvanceObserveStub();
}
namespace {
constexpr char kName[]="SPF_DriverAnimationDiagnostics";
constexpr char kHash[]="5d3b4e343154de6b25dd366b6048c5bc5c452a4daa50bc38b33fd2e571d54257";
constexpr char kDriverPattern[]="40 53 48 83 EC 50 48 83 79 08 00 48 8B D9 0F 84 43 01 00 00 83 79 10 FF 0F 84 39 01 00 00 80 79";
constexpr char kAdvancePattern[]="48 8B C4 55 41 57 48 81 EC 88 00 00 00 48 8B E9 44 8B FA 48 81 C1 08 02 00 00 0F 29 70 C8 0F 28";
const SPF_Core_API* core=nullptr;
const SPF_Load_API* load=nullptr;
SPF_Logger_Handle* logger=nullptr;
SPF_Hook_Handle* driverHook=nullptr;
SPF_Hook_Handle* advanceHook=nullptr;
std::atomic<bool> enabled{false};
uintptr_t expectedReturn=0;
using ResetAnimation=uint32_t (*)(uintptr_t,uint32_t,uint32_t);
using EnableAnimation=void (*)(uintptr_t,uint32_t,bool);
using DurationQuery=float (*)(uintptr_t,uint32_t);
using FrameQuery=uint32_t (*)(uintptr_t,uint32_t);
ResetAnimation resetAnimation=nullptr;
EnableAnimation enableAnimation=nullptr;
DurationQuery durationQuery=nullptr;
FrameQuery frameQuery=nullptr;
std::atomic<uint64_t> steeringOverrides{0},steeringFaults{0};
std::atomic<float> lastPose{0};
std::atomic<uint64_t> driverCalls{0}, advanceCalls{0}, rejected{0};
std::atomic<float> userSteering{0}, effectiveSteering{0};
std::atomic<uint64_t> telemetryTick{0};
std::mutex sampleMutex;
SampleLedger ledger;
struct ModelObservation { uintptr_t model=0;uint32_t index=0;float requested=0;uint64_t calls=0,tick=0;char path[192]{}; };
std::array<ModelObservation,16> modelObservations{};
std::array<uintptr_t,512> inspectedModels{};
size_t inspectedCount=0;
std::atomic<uint64_t> identityAttempts{0},identityFailures{0};
thread_local DriverSample currentDriver;
std::atomic<uint64_t> worldGeneration{1};
thread_local uint64_t currentGeneration=0;
void Log(SPF_LogLevel level,const char* text) {
 if(load && load->logger && logger) load->logger->Log(logger,level,text);
}
bool SafeRead(uintptr_t address,void* output,size_t size) noexcept {
 if(!address || !output || size>4096) return false;
 __try { std::memcpy(output,reinterpret_cast<const void*>(address),size); return true; }
 __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
// Read-only identity lead inferred from the engine's own model error-reporting code.
// A matching readable path remains runtime discovery evidence, not a pose-control proof.
bool ReadModelPath(uintptr_t model,char (&path)[192]) noexcept {
 uintptr_t resource=0,descriptor=0,name=0;
 if(!SafeRead(model+0x118,&resource,8) || !resource ||
    !SafeRead(resource+0x10,&descriptor,8) || !descriptor ||
    !SafeRead(descriptor+0x28,&name,8) || !name) return false;
 for(size_t i=0;i<sizeof(path)-1;++i) {
  if(!SafeRead(name+i,path+i,1)) return false;
  const auto c=static_cast<unsigned char>(path[i]);
  if(!c) return i>0;
  if(c<32 || c>126) return false;
 }
 return false;
}
void ObserveModel(uintptr_t model,uint32_t index,float requested) noexcept {
 if(!model || !std::isfinite(requested))return;
 std::lock_guard lock(sampleMutex);
 for(auto& item:modelObservations) if(item.model==model && item.index==index) {
  item.requested=requested;item.tick=GetTickCount64();++item.calls;return;
 }
 for(size_t i=0;i<inspectedCount;++i) if(inspectedModels[i]==model)return;
 if(inspectedCount==inspectedModels.size())return;
 inspectedModels[inspectedCount++]=model;identityAttempts.fetch_add(1);
 char path[192]{};
 if(!ReadModelPath(model,path)) {identityFailures.fetch_add(1);return;}
 if(!std::strstr(path,"spf_stock_driver") && !std::strstr(path,"asset/character/human/driver"))return;
 for(auto& item:modelObservations) if(!item.model) {
  item.model=model;item.index=index;item.requested=requested;item.tick=GetTickCount64();item.calls=1;
  std::memcpy(item.path,path,sizeof(path));return;
 }
}
void Controls(const SPF_Controls* data,void*) {
 if(!data) return;
 userSteering.store(data->userInput.steering,std::memory_order_relaxed);
 effectiveSteering.store(data->effectiveInput.steering,std::memory_order_relaxed);
 telemetryTick.store(GetTickCount64(),std::memory_order_relaxed);
}
void Reset() {
 worldGeneration.fetch_add(1);
 std::lock_guard lock(sampleMutex); ledger={}; modelObservations={};inspectedModels={};inspectedCount=0;
 telemetryTick.store(0);
}
bool HashExecutable(char (&hex)[65]) {
    wchar_t path[32768];
    if (!GetModuleFileNameW(nullptr,path,32768)) return false;
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (file==INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm=nullptr; BCRYPT_HASH_HANDLE hash=nullptr;
    DWORD size=0, returned=0;
    bool okay=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if (okay) okay=BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&returned,0)>=0;
    std::vector<BYTE> object(size);
    if (okay) okay=BCryptCreateHash(algorithm,&hash,object.data(),size,nullptr,0,0)>=0;
    std::array<BYTE,65536> buffer{}; DWORD bytes=0;
    while (okay) {
        if (!ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&bytes,nullptr)) { okay=false; break; }
        if (!bytes) break;
        okay=BCryptHashData(hash,buffer.data(),bytes,0)>=0;
    }
    std::array<BYTE,32> digest{};
    if (okay) okay=BCryptFinishHash(hash,digest.data(),32,0)>=0;
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm,0);
    CloseHandle(file);
    if (okay) for (size_t i=0;i<32;++i) std::snprintf(hex+i*2,3,"%02x",digest[i]);
    return okay;
}

void Manifest(SPF_Manifest_Builder_Handle* h,const SPF_Manifest_Builder_API* api) {
 api->Info_SetName(h,kName); api->Info_SetVersion(h,"0.2.0");
 api->Info_SetMinFrameworkVersion(h,"1.2.5");
 api->Info_SetAuthor(h,"Local driver animation diagnostics");
 api->Info_SetDescriptionLiteral(h,"Experimental live steering for the private Mustang interior body.");
 api->Policy_SetAllowUserConfig(h,true);
 api->Policy_AddConfigurableSystem(h,"logging");
 api->Defaults_SetLogging(h,"info",true);
}
void OnLoad(const SPF_Load_API* api) {
 load=api; if(api && api->logger) logger=api->logger->Log_GetContext(kName);
}
void Activated(const SPF_Core_API* api) {
 core=api;
 if(!core || !core->hooks) {Log(SPF_LOG_ERROR,"DIAG: hooks API unavailable.");return;}
 char hash[65]{};
 if(!HashExecutable(hash) || std::strcmp(hash,kHash)!=0) {
  Log(SPF_LOG_ERROR,"DIAG: executable build mismatch; no hooks registered.");return;
 }
 const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 auto hooks=core->hooks;
 if(hooks->Hook_FindPattern(kDriverPattern)!=base+0x9e1c40 ||
    hooks->Hook_FindPattern(kAdvancePattern)!=base+0x15deed0) {
  Log(SPF_LOG_ERROR,"DIAG: expected unique entry signatures absent; no hooks registered.");return;
 }
 expectedReturn=base+0x9e1d92;
 resetAnimation=reinterpret_cast<ResetAnimation>(base+0x15de620);
 enableAnimation=reinterpret_cast<EnableAnimation>(base+0x15decc0);
 durationQuery=reinterpret_cast<DurationQuery>(base+0x359eb0);
 frameQuery=reinterpret_cast<FrameQuery>(base+0x359f20);
 driverHook=hooks->Hook_Register(kName,"DriverUpdate","Observe driver update",reinterpret_cast<void*>(DriverObserveStub),&gDriverOriginal,kDriverPattern,true);
 advanceHook=hooks->Hook_Register(kName,"DriverSample","Observe animation advancement",reinterpret_cast<void*>(AdvanceObserveStub),&gAdvanceOriginal,kAdvancePattern,true);
 if(!driverHook || !advanceHook) {
  Log(SPF_LOG_ERROR,"DIAG: incomplete hook registration; observers remain inactive.");return;
 }
 if(core->telemetry) {
  auto context=core->telemetry->Tel_GetContext(kName);
  core->telemetry->Tel_RegisterForControls(context,Controls,nullptr);
 }
 enabled.store(true);
 Log(SPF_LOG_INFO,"STEERING: hooks requested; only the private interior model with the 47-frame, 1.916667-second clip may be overridden. Runtime pose validation pending.");
}
void Update() {
 if(!enabled.load()) return;
 static bool installed=false;
 if(!installed && core->hooks->Hook_IsInstalled(driverHook) && core->hooks->Hook_IsInstalled(advanceHook)) {
  installed=true; Log(SPF_LOG_INFO,"DIAG: both hooks installed; awaiting driver update and sample evidence.");
 }
 static uint64_t last=0;
 const uint64_t tick=GetTickCount64(); if(tick-last<250) return; last=tick;
 SampleLedger copy;std::array<ModelObservation,16> models;size_t inspectedSnapshot=0;
 {std::lock_guard lock(sampleMutex);copy=ledger;models=modelObservations;inspectedSnapshot=inspectedCount;}
 char text[512];
 const auto telTick=telemetryTick.load();
 for(const auto& sample:copy.slots) {
  if(!sample.controller || tick-sample.tick>750) continue;
  std::snprintf(text,sizeof(text),"SAMPLE controller=%p model=%p index=%u duration=%.6f requested=%.6f normalized=%.6f calls=%llu user_steering=%.6f effective_steering=%.6f telemetry_valid=%d telemetry_age_ms=%llu sample_age_ms=%llu generation=%llu",
   reinterpret_cast<void*>(sample.controller),reinterpret_cast<void*>(sample.model),sample.index,
   sample.duration,sample.requested,sample.requested/sample.duration,
   static_cast<unsigned long long>(sample.calls),userSteering.load(),effectiveSteering.load(),telTick?1:0,
   static_cast<unsigned long long>(telTick?tick-telTick:0),static_cast<unsigned long long>(tick-sample.tick),
   static_cast<unsigned long long>(worldGeneration.load()));
  Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"STEERING overrides=%llu faults=%llu normalized=%.6f",
   static_cast<unsigned long long>(steeringOverrides.load()),static_cast<unsigned long long>(steeringFaults.load()),lastPose.load());
  Log(SPF_LOG_INFO,text);
 }
 for(const auto& model:models) if(model.model && tick-model.tick<=750) {
  std::snprintf(text,sizeof(text),"MODEL_SAMPLE model=%p path=%s index=%u requested=%.6f calls=%llu age_ms=%llu",
   reinterpret_cast<void*>(model.model),model.path,model.index,model.requested,
   static_cast<unsigned long long>(model.calls),static_cast<unsigned long long>(tick-model.tick));
  Log(SPF_LOG_INFO,text);
 }
 static uint64_t lastHealth=0;
 if(tick-lastHealth>=2000) {
  lastHealth=tick;
  std::snprintf(text,sizeof(text),"HEALTH driver_calls=%llu matched_advance_calls=%llu rejected_samples=%llu ledger_dropped=%llu",
   static_cast<unsigned long long>(driverCalls.load()),static_cast<unsigned long long>(advanceCalls.load()),
   static_cast<unsigned long long>(rejected.load()),static_cast<unsigned long long>(copy.dropped));
  Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"IDENTITY attempts=%llu unreadable=%llu inspected=%zu",
   static_cast<unsigned long long>(identityAttempts.load()),static_cast<unsigned long long>(identityFailures.load()),inspectedSnapshot);
  Log(SPF_LOG_INFO,text);
 }
}
void Unload() { enabled.store(false); Reset(); }
}
extern "C" void CaptureDriver(uintptr_t controller) noexcept {
 currentDriver={}; currentGeneration=worldGeneration.load();
 if(!enabled.load()) return;
 driverCalls.fetch_add(1,std::memory_order_relaxed);
 unsigned char data[0x30]{};
 if(!SafeRead(controller,data,sizeof(data))) return;
 DriverSample sample{};sample.controller=controller;
 std::memcpy(&sample.model,data+8,8);
 std::memcpy(&sample.index,data+0x10,4);
 std::memcpy(&sample.duration,data+0x14,4);
 if(ValidSample(sample)) currentDriver=sample;
}
// Invoke the same reset/enable sequence as the exterior driver's verified caller.
// Only the saved XMM2 float is changed; the assembly stub forwards everything else,
// including the original function's return value and caller's stack arguments.
void ApplyInteriorSteering(uintptr_t model,uint32_t index,float* replacement) noexcept {
 static thread_local bool applying=false;
 if(applying || !replacement || index>=4096 || steeringFaults.load())return;
 char path[192]{};
 if(!ReadModelPath(model,path) || std::strcmp(path,"/model/spf_stock_driver/driver.pmd"))return;
 uintptr_t states=0,count=0,animation=0;
 if(!SafeRead(model+0x210,&states,8) || !states ||
    !SafeRead(model+0x218,&count,8) || index>=count || count>4096 ||
    !SafeRead(states+index*0x58+8,&animation,8) || !animation)return;
 applying=true;
 __try {
  const float duration=durationQuery(model,index);
  if(!IsSteeringClip(duration,frameQuery(model,index))) {applying=false;return;}
  const uint64_t now=GetTickCount64(),tick=telemetryTick.load();
  const float input=effectiveSteering.load();
  const float fraction=(tick && now-tick<=250 && std::isfinite(input)) ? SteeringFraction(input) : 0.f;
  resetAnimation(model,index,0);
  enableAnimation(model,index,true);
  *replacement=duration*fraction;
  lastPose.store(fraction);steeringOverrides.fetch_add(1);
 } __except(EXCEPTION_EXECUTE_HANDLER) {steeringFaults.fetch_add(1);}
 applying=false;
}
extern "C" void CaptureAdvance(uintptr_t model,uint32_t index,float requested,uintptr_t returnAddress,float* replacement) noexcept {
 if(!enabled.load()) return;
 ObserveModel(model,index,requested);
 ApplyInteriorSteering(model,index,replacement);
 if(returnAddress!=expectedReturn) return;
 advanceCalls.fetch_add(1,std::memory_order_relaxed);
 auto sample=currentDriver;
 if(currentGeneration!=worldGeneration.load() || model!=sample.model || index!=sample.index) {
  rejected.fetch_add(1,std::memory_order_relaxed);return;
 }
 sample.requested=requested;sample.tick=GetTickCount64();
 std::lock_guard lock(sampleMutex);
 if(!ledger.Record(sample)) rejected.fetch_add(1,std::memory_order_relaxed);
}
extern "C" SPF_PLUGIN_EXPORT bool SPF_GetManifestAPI(SPF_Manifest_API* api) {
 if(!api)return false;api->BuildManifest=Manifest;return true;
}
extern "C" SPF_PLUGIN_EXPORT bool SPF_GetPlugin(SPF_Plugin_Exports* api) {
 if(!api)return false;
 api->OnLoad=OnLoad;api->OnActivated=Activated;api->OnUpdate=Update;api->OnUnload=Unload;
 api->OnWorldUnloaded=Reset;api->OnGameWorldReady=Reset;return true;
}
