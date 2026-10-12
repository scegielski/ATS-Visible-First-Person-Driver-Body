#include <Windows.h>
#include <bcrypt.h>
#include <SPF_Plugin.h>
#include <SPF_Manifest_API.h>
#include <SPF_Logger_API.h>
#include <SPF_Telemetry_API.h>
#include <SPF_TelemetryData.h>
#include <SPF_Camera_API.h>
#include "BodyVisibility.hpp"
#include "PlayerExteriorDriver.hpp"
#include "GameHorn.hpp"
#include "GameEngine.hpp"
#include "EngineStartGate.hpp"
#include "IgnitionMotion.hpp"
#include "IgnitionTargets.hpp"
#include "RuntimeArmIK.hpp"
#include "SignalMotion.hpp"
#include "SignalRequestGate.hpp"
#include "SignalReplayGuard.hpp"
#include "SignalTiming.hpp"
#include "HeadTracking.hpp"
#include "HeadTorso.hpp"
#include "SignalTargets.hpp"
#include "DriverSamples.hpp"
#include "SteeringPose.hpp"
#include "PedalMotion.hpp"
#include "HornPose.hpp"
#include "WindowGesture.hpp"
#include "LiveGesturePose.hpp"
#include "RightGestureTargets.hpp"
#include <filesystem>
#include <fstream>
#include <SPF_KeyBinds_API.h>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <vector>

extern "C" {
void* gDriverOriginal=nullptr;
void* gEngineOriginal=nullptr;
void* gHeadOriginal=nullptr;void HeadObserveStub();
void* gSignalLeftOriginal=nullptr;void* gSignalRightOriginal=nullptr;
void SignalLeftRequestStub();void SignalRightRequestStub();
void EngineRequestObserveStub();
void* gAdvanceOriginal=nullptr;
void* gBoneOriginal=nullptr;
void BoneObserveStub();
void DriverObserveStub();
void AdvanceObserveStub();
}
namespace {
constexpr char kName[]="SPF_DriverAnimationDiagnostics";
constexpr char kHash[]="5d3b4e343154de6b25dd366b6048c5bc5c452a4daa50bc38b33fd2e571d54257";
constexpr char kDriverPattern[]="40 53 48 83 EC 50 48 83 79 08 00 48 8B D9 0F 84 43 01 00 00 83 79 10 FF 0F 84 39 01 00 00 80 79";
constexpr char kAdvancePattern[]="48 8B C4 55 41 57 48 81 EC 88 00 00 00 48 8B E9 44 8B FA 48 81 C1 08 02 00 00 0F 29 70 C8 0F 28";
constexpr char kBonePattern[]="48 8B C4 48 89 68 18 56 57 41 54 41 55 41 57 48 81 EC 00 02 00 00 8B AC 24 50 02 00 00 45 33 ED";
SPF_Hook_Handle* boneHook=nullptr;
SPF_Hook_Handle* engineRequestHook=nullptr;
std::atomic<uint64_t> engineRequestCalls{0},playerEngineRequestCalls{0};
constexpr char kEngineRequestPattern[]="48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 60 33 F6 0F 29 74 24 50 48 8B D9 84 D2 0F 84 47 02 00 00 0F 57 F6";
std::atomic<bool> hornHeld{false};
std::atomic<bool> bodyCameraValid{false},hideBody{false};
std::atomic<int> bodyCameraType{-1};
std::atomic<uint64_t> hiddenBodyFrames{0},visibilityFaults{0};
std::atomic<int> bodyVisibilityMode{1};
std::atomic<bool> hideStockDriver{false};
std::atomic<uintptr_t> expectedPlayerDriverController{0},expectedPlayerDriverModel{0},confirmedPlayerDriverModel{0};
std::atomic<uint64_t> playerDriverTick{0},playerDriverMatches{0},hiddenStockFrames{0},hiddenHeadFrames{0};
std::atomic<uint64_t> boneSamples{0},hornOverrides{0},hornFaults{0};
std::atomic<uint64_t> hornTick{0};
std::atomic<bool> gameHornValid{false};
SPF_KeyBinds_Handle* gestureKeys=nullptr;
LiveGesturePose liveGesturePose;
LiveGesturePose liveSignalLeft{kSignalLeftTargets},liveSignalRight{kSignalRightTargets};
SPF_Hook_Handle* headHook=nullptr;
constexpr char kHeadPattern[]="8B D8 85 C0 0F 89 9F 00 00 00 48 8B 46 58 48 85 C0 74 7E 48 8B 4E 10 48 85";
std::mutex headMutex;ObservedHead latestHead{};
std::atomic<uint64_t> headAccepted{0},headRejected{0},headTorsoUpdates{0},headTorsoRejected{0},exteriorHeadUpdates{0},exteriorHeadRejected{0};
std::mutex headDriveMutex;HeadTorsoDrive headDrive{};SeatedHeadCalibration headCalibration;
std::atomic<float> headForwardLimit{.30f},headLeanLimitDegrees{28.f},headLateralLimit{.24f};
std::atomic<float> headTrackingGain{.65f},headTorsoError{0},headTorsoStrength{0};
SignalSelection signalSelection;
SignalRequestGate signalGate;std::mutex signalGateMutex;
std::atomic<uint64_t> signalReady{0},signalCancels{0},signalQueued{0},signalReleased{0},signalTimeouts{0};
std::atomic<float> signalActivationDelay{.22f},signalActuationSeconds{.16f};
std::atomic<int> skipSignalDirection{0};std::atomic<uint64_t> signalReplayTick{0};
SPF_Hook_Handle* signalLeftHook=nullptr;SPF_Hook_Handle* signalRightHook=nullptr;
constexpr char kSignalLeftPattern[]="84 C9 74 6A 0F B6 87 48 02 00 00 80 A7 49 02 00 00 F9 0F B6 C8 C0 E9 02";
constexpr char kSignalRightPattern[]="84 C9 74 65 0F B6 87 48 02 00 00 80 A7 49 02 00 00 F9 0F B6 C8 C0 E9 03";
std::atomic<uint64_t> signalSequence{0},signalTick{0},signalEventTick{0},signalOverrides{0};
std::atomic<int> signalDirection{0};
std::atomic<bool> selectedLeft{false},selectedRight{false};
LiveGesturePose liveRightGesturePose{kRightGestureTargets};
LiveGesturePose liveIgnitionPose{kIgnitionTargets};
std::atomic<uint64_t> engineTick{0},engineStartSequence{0},ignitionOverrides{0};
std::atomic<bool> engineStarting{false};
std::atomic<uint64_t> runtimeIKUpdates{0},runtimeIKRejected{0};
std::atomic<float> runtimeIKDegrees{0},runtimeIKError{0};
std::atomic<float> leftElbowInset{.06f},rightElbowInset{.04f};
std::atomic<uint64_t> rightElbowUpdates{0},rightElbowRejected{0};
std::atomic<uint64_t> leftElbowUpdates{0},leftElbowRejected{0};
int priorEngineSample=-1;
EngineStartGate engineStartGate;std::mutex engineGateMutex;
std::atomic<uint64_t> deferredEngineSequence{0},engineKeyReached{0},engineDelayCancels{0},privateBodyTick{0};
std::atomic<uint64_t> engineDeferredCount{0},engineReleasedCount{0},engineTimeoutCount{0};
std::atomic<bool> skipNextEngineStartEvent{false};
std::atomic<bool> rightGestureHeld{false};
std::atomic<uint64_t> rightGestureTick{0},rightGestureOverrides{0};
std::atomic<bool> windowGestureHeld{false};
std::atomic<uint64_t> windowGestureTick{0},windowGestureOverrides{0};
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
std::atomic<float> userThrottle{0},lastThrottle{0};
std::atomic<uint64_t> throttleOverrides{0};
std::atomic<float> userBrake{0},lastBrake{0},lastTransfer{0},lastAnklePress{0};
std::atomic<uint64_t> brakeOverrides{0};
std::atomic<uint32_t> lastPedalFrame{0};
std::atomic<float> userClutch{0},lastClutch{0},lastClutchPress{0};
std::atomic<uint64_t> clutchOverrides{0};
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
 userThrottle.store(data->userInput.throttle,std::memory_order_relaxed);
 userBrake.store(data->userInput.brake,std::memory_order_relaxed);
 userClutch.store(data->userInput.clutch,std::memory_order_relaxed);
 telemetryTick.store(GetTickCount64(),std::memory_order_relaxed);
}
void TruckSignals(const SPF_TruckData* data,void*) {
 if(!data)return;
 selectedLeft.store(data->lblinker);selectedRight.store(data->rblinker);signalTick.store(GetTickCount64());
 int direction=signalSelection.Sample(data->lblinker,data->rblinker,data->hazard_warning);
 const int replayed=skipSignalDirection.load();
 const int observed=data->lblinker==data->rblinker?0:data->lblinker?1:2;
 if(replayed){
  if(KeepSignalReplay(replayed,signalReplayTick.load(),GetTickCount64(),observed,data->hazard_warning)){
   if(observed==replayed){skipSignalDirection.store(0);if(direction==replayed)direction=0;}
  }else skipSignalDirection.store(0);
 }
 if(direction){signalDirection.store(direction);signalEventTick.store(GetTickCount64());signalSequence.fetch_add(1);char text[128];std::snprintf(text,sizeof(text),"SIGNAL_EVENT direction=%s",direction==1?"left":"right");Log(SPF_LOG_INFO,text);}
}
void Reset() {
 headCalibration.Reset();{std::lock_guard lock(headDriveMutex);headDrive={};}
 {std::lock_guard lock(signalGateMutex);signalGate.Cancel();}
 signalReady.store(0);skipSignalDirection.store(0);signalCancels.fetch_add(1);
 {std::lock_guard lock(engineGateMutex);engineStartGate.Cancel();}
 deferredEngineSequence.store(0);engineKeyReached.store(0);privateBodyTick.store(0);skipNextEngineStartEvent.store(false);
 bodyCameraValid.store(false);hideBody.store(false);bodyCameraType.store(-1);hideStockDriver.store(false);expectedPlayerDriverController.store(0);expectedPlayerDriverModel.store(0);confirmedPlayerDriverModel.store(0);playerDriverTick.store(0);
 signalSelection.Reset();signalTick.store(0);signalEventTick.store(0);selectedLeft.store(false);selectedRight.store(false);
 worldGeneration.fetch_add(1);engineTick.store(0);engineStarting.store(false);priorEngineSample=-1;
 std::lock_guard lock(sampleMutex); ledger={}; modelObservations={};inspectedModels={};inspectedCount=0;
 telemetryTick.store(0);hornTick.store(0);hornHeld.store(false);gameHornValid.store(false);windowGestureHeld.store(false);windowGestureTick.store(0);rightGestureHeld.store(false);rightGestureTick.store(0);
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
 api->Info_SetName(h,kName); api->Info_SetVersion(h,"0.16.4");
 api->Info_SetMinFrameworkVersion(h,"1.2.5");
 api->Info_SetAuthor(h,"Local driver animation diagnostics");
 api->Info_SetDescriptionLiteral(h,"Live steering, independent pedals, game horn and independent left/right middle-finger gestures with live pose files.");
 api->Policy_SetAllowUserConfig(h,true);
 api->Policy_AddConfigurableSystem(h,"logging");
 api->Defaults_SetLogging(h,"info",true);
 api->Policy_AddConfigurableSystem(h,"keybinds");
 api->Defaults_AddKeybind(h,"HeadTracking","Recenter","keyboard","KEY_F8","always");
 api->Defaults_AddKeybind(h,"Gestures","LeftWindowBird","keyboard","KEY_F10","always");
 api->Defaults_AddKeybind(h,"Gestures","RightHandBird","keyboard","KEY_F11","always");

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
 if(hooks->Hook_FindPattern(kEngineRequestPattern)!=base+0x5e4a20){Log(SPF_LOG_ERROR,"ENGINE_REQUEST: signature mismatch; diagnostic disabled.");return;}
 if(hooks->Hook_FindPattern(kBonePattern)!=base+0x15dfbf0) {Log(SPF_LOG_ERROR,"HORN: bone entry signature mismatch; diagnostic disabled.");return;}
 if(hooks->Hook_FindPattern(kSignalLeftPattern)!=base+0x9f85e2 || hooks->Hook_FindPattern(kSignalRightPattern)!=base+0x9f8670){Log(SPF_LOG_ERROR,"SIGNAL_DELAY: input-site signature mismatch; no hooks registered.");return;}
 if(hooks->Hook_FindPattern(kHeadPattern)==base+0x1632dde)headHook=hooks->Hook_Register(kName,"HeadTrackingObserver","Observe local-space OpenXR head views",reinterpret_cast<void*>(HeadObserveStub),&gHeadOriginal,kHeadPattern,true);
 else Log(SPF_LOG_WARN,"HEAD_TRACKING: view-site signature mismatch; observer disabled.");
 expectedReturn=base+0x9e1d92;
 resetAnimation=reinterpret_cast<ResetAnimation>(base+0x15de620);
 enableAnimation=reinterpret_cast<EnableAnimation>(base+0x15decc0);
 durationQuery=reinterpret_cast<DurationQuery>(base+0x359eb0);
 frameQuery=reinterpret_cast<FrameQuery>(base+0x359f20);
 driverHook=hooks->Hook_Register(kName,"DriverUpdate","Observe driver update",reinterpret_cast<void*>(DriverObserveStub),&gDriverOriginal,kDriverPattern,true);
 advanceHook=hooks->Hook_Register(kName,"DriverSample","Observe animation advancement",reinterpret_cast<void*>(AdvanceObserveStub),&gAdvanceOriginal,kAdvancePattern,true);
 boneHook=hooks->Hook_Register(kName,"BonePose","Observe private bone pose before hierarchy assembly",reinterpret_cast<void*>(BoneObserveStub),&gBoneOriginal,kBonePattern,true);
 engineRequestHook=hooks->Hook_Register(kName,"EngineRequest","Observe player engine start/stop request",reinterpret_cast<void*>(EngineRequestObserveStub),&gEngineOriginal,kEngineRequestPattern,true);
 signalLeftHook=hooks->Hook_Register(kName,"SignalLeftRequest","Delay player left toggle until stalk contact",reinterpret_cast<void*>(SignalLeftRequestStub),&gSignalLeftOriginal,kSignalLeftPattern,true);
 signalRightHook=hooks->Hook_Register(kName,"SignalRightRequest","Delay player right toggle until stalk contact",reinterpret_cast<void*>(SignalRightRequestStub),&gSignalRightOriginal,kSignalRightPattern,true);
 if(!driverHook || !advanceHook || !boneHook || !engineRequestHook || !signalLeftHook || !signalRightHook) {
  Log(SPF_LOG_ERROR,"DIAG: incomplete hook registration; observers remain inactive.");return;
 }
 if(core->telemetry) {
  auto context=core->telemetry->Tel_GetContext(kName);
  core->telemetry->Tel_RegisterForControls(context,Controls,nullptr);
  core->telemetry->Tel_RegisterForTruckData(context,TruckSignals,nullptr);
 }
 if(core->keybinds)gestureKeys=core->keybinds->Kbind_GetContext(kName);
 enabled.store(true);
 Log(SPF_LOG_INFO,"PEDALS: hooks requested for private interior model; supports prior steering/pedal clips and independent clutch clip. New brake alignment and clutch visual validation pending.");
}
struct PoseFileWatch {std::filesystem::path path;uint64_t lastPoll=0,lastStamp=0;bool attempted=false;};
void PollOneGestureFile(const wchar_t* filename,const char* hand,LiveGesturePose& store,PoseFileWatch& watch) {
 auto& path=watch.path;auto& lastPoll=watch.lastPoll;auto& lastStamp=watch.lastStamp;auto& attempted=watch.attempted;
 const auto now=GetTickCount64();if(now-lastPoll<500)return;lastPoll=now;
 if(path.empty()) {
  HMODULE module=nullptr;wchar_t modulePath[32768]{};
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollOneGestureFile),&module) || !GetModuleFileNameW(module,modulePath,32768))return;
  path=std::filesystem::path(modulePath).parent_path()/L"config"/filename;
 }
 WIN32_FILE_ATTRIBUTE_DATA info{};
 if(!GetFileAttributesExW(path.c_str(),GetFileExInfoStandard,&info))return;
 const uint64_t stamp=(static_cast<uint64_t>(info.ftLastWriteTime.dwHighDateTime)<<32)|info.ftLastWriteTime.dwLowDateTime;
 if(attempted && stamp==lastStamp)return;
 attempted=true;lastStamp=stamp;
 if(info.nFileSizeHigh || info.nFileSizeLow>65536){Log(SPF_LOG_WARN,"POSE_RELOAD rejected: file too large; last valid pose retained.");return;}
 std::ifstream file(path,std::ios::binary);if(!file)return;
 std::string content(info.nFileSizeLow,'\0');file.read(content.data(),static_cast<std::streamsize>(content.size()));
 if(!file && !content.empty()){Log(SPF_LOG_WARN,"POSE_RELOAD rejected: incomplete read; last valid pose retained.");return;}
 std::string error;char text[256];
 if(store.Replace(content,error))std::snprintf(text,sizeof(text),"POSE_RELOAD hand=%s accepted generation=%llu joints=21",hand,static_cast<unsigned long long>(store.Generation()));
 else std::snprintf(text,sizeof(text),"POSE_RELOAD hand=%s rejected: %s; last valid pose retained.",hand,error.c_str());
 Log(SPF_LOG_INFO,text);
}
void PollBodyVisibilityMode() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t path[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollBodyVisibilityMode),&module) || !GetModuleFileNameW(module,path,32768))return;
 auto filePath=std::filesystem::path(path).parent_path()/L"config"/L"body_visibility.txt";std::error_code ec;auto size=std::filesystem::file_size(filePath,ec);if(ec || size>64)return;
 std::ifstream file(filePath);std::string value,extra;if(!(file>>value) || (file>>extra))return;
 int mode=value=="interior"?0:value=="replace-exterior"?1:-1;
 if(mode>=0 && mode!=bodyVisibilityMode.load()){bodyVisibilityMode.store(mode);Log(SPF_LOG_INFO,mode==0?"VISIBILITY_MODE interior":"VISIBILITY_MODE replace-exterior");}
}
void PollSignalActivationDelay() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t modulePath[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollSignalActivationDelay),&module) || !GetModuleFileNameW(module,modulePath,32768))return;
 auto path=std::filesystem::path(modulePath).parent_path()/L"config"/L"signal_activation_delay.txt";
 std::error_code ec;auto size=std::filesystem::file_size(path,ec);if(ec || size>128)return;
 std::ifstream file(path,std::ios::binary);if(!file)return;std::string text(static_cast<size_t>(size),'\0');file.read(text.data(),static_cast<std::streamsize>(size));if(!file)return;
 float value=0;if(!ParseSignalDelay(text,value))return;
 if(signalActivationDelay.exchange(value)!=value){char message[128];std::snprintf(message,sizeof(message),"SIGNAL_TIMING_RELOAD accepted delay_seconds=%.3f",value);Log(SPF_LOG_INFO,message);}
}
void PollSignalActuationSeconds() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t modulePath[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollSignalActuationSeconds),&module) || !GetModuleFileNameW(module,modulePath,32768))return;
 auto path=std::filesystem::path(modulePath).parent_path()/L"config"/L"signal_actuation_seconds.txt";
 std::error_code ec;auto size=std::filesystem::file_size(path,ec);if(ec || size>128)return;
 std::ifstream file(path,std::ios::binary);if(!file)return;std::string text(static_cast<size_t>(size),'\0');file.read(text.data(),static_cast<std::streamsize>(size));if(!file)return;
 float value=0;if(!ParseSignalActuation(text,value))return;
 if(signalActuationSeconds.exchange(value)!=value){char message[128];std::snprintf(message,sizeof(message),"SIGNAL_TIMING_RELOAD accepted actuation_seconds=%.3f",value);Log(SPF_LOG_INFO,message);}
}
void PollLeftElbowInset() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t modulePath[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollLeftElbowInset),&module) || !GetModuleFileNameW(module,modulePath,32768))return;
 auto path=std::filesystem::path(modulePath).parent_path()/L"config"/L"left_elbow_inset.txt";
 std::error_code ec;auto size=std::filesystem::file_size(path,ec);if(ec || size>128)return;
 std::ifstream file(path);float value;std::string extra;
 if(!(file>>value) || (file>>extra) || !std::isfinite(value) || value<0 || value>.15f)return;
 if(value!=leftElbowInset.load()){leftElbowInset.store(value);char text[128];std::snprintf(text,sizeof(text),"ELBOW_RELOAD accepted inset_m=%.4f",value);Log(SPF_LOG_INFO,text);}
}
void PollRightElbowInset() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t modulePath[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollRightElbowInset),&module) || !GetModuleFileNameW(module,modulePath,32768))return;
 auto path=std::filesystem::path(modulePath).parent_path()/L"config"/L"right_elbow_inset.txt";
 std::error_code ec;auto size=std::filesystem::file_size(path,ec);if(ec || size>128)return;
 std::ifstream file(path);float value;std::string extra;
 if(!(file>>value) || (file>>extra) || !std::isfinite(value) || value<0 || value>.15f)return;
 if(value!=rightElbowInset.load()){rightElbowInset.store(value);char text[128];std::snprintf(text,sizeof(text),"RIGHT_ELBOW_RELOAD accepted inset_m=%.4f",value);Log(SPF_LOG_INFO,text);}
}
void PollGesturePoseFile() {
 static PoseFileWatch left,right,ignition,signalLeft,signalRight;
 PollOneGestureFile(L"gesture_pose.txt","left",liveGesturePose,left);
 PollOneGestureFile(L"right_gesture_pose.txt","right",liveRightGesturePose,right);
 PollOneGestureFile(L"ignition_pose.txt","ignition",liveIgnitionPose,ignition);
 PollOneGestureFile(L"signal_left_pose.txt","signal-left",liveSignalLeft,signalLeft);
 PollOneGestureFile(L"signal_right_pose.txt","signal-right",liveSignalRight,signalRight);
}
void BlendLiveWindowGesture(std::array<std::array<float,4>,61>& rotations,const WindowGestureMotion& motion) {
 const auto pose=liveGesturePose.Snapshot();BlendWindowGesture(rotations,motion,*pose);
}
void BlendLiveRightGesture(std::array<std::array<float,4>,61>& rotations,const WindowGestureMotion& motion) {
 const auto pose=liveRightGesturePose.Snapshot();BlendWindowGesture(rotations,motion,*pose);
}
void BlendLiveIgnition(std::array<std::array<float,4>,61>& rotations,std::array<std::array<float,4>,61>& unturned,const IgnitionMotion& motion) { const auto pose=liveIgnitionPose.Snapshot();unturned=rotations;auto reach=motion.Pose();reach.extend=0;BlendWindowGesture(unturned,reach,*pose);BlendWindowGesture(rotations,motion.Pose(),*pose); }
void BlendLiveSignal(std::array<std::array<float,4>,61>& rotations,const SignalMotion& motion) {const auto pose=motion.direction==1?liveSignalLeft.Snapshot():liveSignalRight.Snapshot();BlendWindowGesture(rotations,motion.Pose(),*pose);}
void PollDeferredEngineStart(bool engineValid,PlayerEngineState state) {
 DeferredEngineStart request;EngineStartGate::Outcome outcome;
 const auto now=GetTickCount64(),tick=telemetryTick.load(),controller=expectedPlayerDriverController.load();
 unsigned char electric=0;const bool electricityValid=controller && SafeRead(controller-0x1df0+0x1bc,&electric,1) && electric==1;
 const bool fresh=engineValid && tick && now-tick<=250 && electricityValid;
 {std::lock_guard lock(engineGateMutex);outcome=engineStartGate.Poll(controller?controller-0x1df0:0,worldGeneration.load(),now,fresh,static_cast<uint32_t>(state),engineKeyReached.load(),request);}
 if(outcome==EngineStartGate::None)return;
 deferredEngineSequence.store(0);engineKeyReached.store(0);
 if(outcome==EngineStartGate::Cancelled){if(!engineValid || state==PlayerEngineState::Off)engineDelayCancels.fetch_add(1);Log(SPF_LOG_INFO,"ENGINE_DELAY cancelled: vehicle/state/freshness changed.");return;}
 // Original requests use these three arguments; all static callers ignore its return.
 using EngineControl=void(*)(uintptr_t,bool,bool);
 skipNextEngineStartEvent.store(true);
 reinterpret_cast<EngineControl>(gEngineOriginal)(request.vehicle,true,false);
 uint32_t after=99;SafeRead(request.vehicle+0x1b8,&after,4);
 if(after!=1 && after!=2)skipNextEngineStartEvent.store(false);
 engineReleasedCount.fetch_add(1);if(outcome==EngineStartGate::Timeout)engineTimeoutCount.fetch_add(1);
 char message[192];std::snprintf(message,sizeof(message),"ENGINE_DELAY released reason=%s delay_ms=%llu state_after=%u",outcome==EngineStartGate::KeyReached?"key-reached":"timeout",static_cast<unsigned long long>(now-request.created),after);Log(SPF_LOG_INFO,message);
}
void PollHeadTrackingLimits() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t name[32768]{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollHeadTrackingLimits),&module) || !GetModuleFileNameW(module,name,32768))return;
 auto path=std::filesystem::path(name).parent_path()/L"config"/L"head_tracking_limits.txt";std::error_code ec;auto size=std::filesystem::file_size(path,ec);if(ec || size>256)return;
 std::ifstream file(path,std::ios::binary);if(!file)return;std::string text(static_cast<size_t>(size),'\0');file.read(text.data(),static_cast<std::streamsize>(size));if(!file)return;
 HeadTrackingLimits limits;if(!ParseHeadTrackingLimits(text,limits))return;
 const bool changed=limits.forward!=headForwardLimit.load() || limits.degrees!=headLeanLimitDegrees.load() || limits.lateral!=headLateralLimit.load();headForwardLimit.store(limits.forward);headLeanLimitDegrees.store(limits.degrees);headLateralLimit.store(limits.lateral);
 if(changed){char message[160];std::snprintf(message,sizeof(message),"HEAD_LIMITS_RELOAD forward_m=%.3f max_lean_degrees=%.1f lateral_m=%.3f",limits.forward,limits.degrees,limits.lateral);Log(SPF_LOG_INFO,message);}
}
void PollHeadTrackingGain() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t name[32768]{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollHeadTrackingGain),&module) || !GetModuleFileNameW(module,name,32768))return;
 auto path=std::filesystem::path(name).parent_path()/L"config"/L"head_tracking_gain.txt";std::error_code ec;auto size=std::filesystem::file_size(path,ec);if(ec || size>128)return;
 std::ifstream file(path);float value;std::string extra;if(!(file>>value) || (file>>extra) || !std::isfinite(value) || value<0 || value>1)return;
 if(headTrackingGain.exchange(value)!=value)Log(SPF_LOG_INFO,"HEAD_TORSO live gain accepted.");
}
HeadTorsoDrive SnapshotHeadDrive(){std::lock_guard lock(headDriveMutex);return headDrive;}
void UpdateHeadTorso() {
 static uint64_t previous=0;static bool wasRecenter=false;const auto now=GetTickCount64();float dt=previous?static_cast<float>(now-previous)/1000.f:0;previous=now;
 ObservedHead pose;{std::lock_guard lock(headMutex);pose=latestHead;}
 const auto controls=telemetryTick.load(),body=privateBodyTick.load();
 const bool interior=bodyCameraValid.load() && bodyCameraType.load()==2;
 const bool cameraKnown=bodyCameraValid.load() && bodyCameraType.load()>=0 && bodyCameraType.load()<=9;
 const bool fresh=cameraKnown && pose.tick && now-pose.tick<=250 && (pose.flags&15)==15 && controls && now-controls<=250 && body && now-body<=250;
 bool recenter=gestureKeys && core->keybinds->Kbind_GetActionValue(gestureKeys,"HeadTracking.Recenter")>.5f;
 if(!cameraKnown){headCalibration.filtered={};}
 else if(headCalibration.Step(pose,worldGeneration.load(),now,fresh,recenter && !wasRecenter,dt,headTrackingGain.load(),headForwardLimit.load(),headLateralLimit.load(),interior))Log(SPF_LOG_INFO,"HEAD_TORSO seated calibration captured.");
 wasRecenter=recenter;{std::lock_guard lock(headDriveMutex);headDrive=headCalibration.filtered;}
}
void PollObservedHead() {
 static uint64_t last=0,generation=0;static ObservedHead baseline{};
 const auto now=GetTickCount64();if(now-last<500)return;last=now;
 ObservedHead pose;{std::lock_guard lock(headMutex);pose=latestHead;}
 if(!pose.tick || now-pose.tick>250 || !bodyCameraValid.load() || bodyCameraType.load()!=2)return;
 if(generation!=worldGeneration.load() || baseline.space!=pose.space){baseline=pose;generation=worldGeneration.load();Log(SPF_LOG_INFO,"HEAD_TRACKING baseline captured in interior camera; observation only.");}
 char text[384];std::snprintf(text,sizeof(text),"HEAD_TRACKING valid=1 samples=%llu rejected=%llu flags=%llu position_m=%.4f,%.4f,%.4f delta_m=%.4f,%.4f,%.4f quat_xyzw=%.5f,%.5f,%.5f,%.5f",static_cast<unsigned long long>(headAccepted.load()),static_cast<unsigned long long>(headRejected.load()),static_cast<unsigned long long>(pose.flags),pose.position[0],pose.position[1],pose.position[2],pose.position[0]-baseline.position[0],pose.position[1]-baseline.position[1],pose.position[2]-baseline.position[2],pose.orientation[0],pose.orientation[1],pose.orientation[2],pose.orientation[3]);Log(SPF_LOG_INFO,text);
}
void Update() {
 if(!enabled.load()) return;
 PollBodyVisibilityMode();
 PollObservedHead();
 SPF_CameraType cameraType{};const bool cameraValid=core && core->camera && core->camera->Cam_GetCurrentCamera && core->camera->Cam_GetCurrentCamera(&cameraType);
 bodyCameraValid.store(cameraValid);bodyCameraType.store(cameraValid?static_cast<int>(cameraType):-1);hideBody.store(bodyVisibilityMode.load()==0 && HideInteriorBody(cameraValid,static_cast<int>(cameraType)));hideStockDriver.store(bodyVisibilityMode.load()==1 && HideInteriorBody(cameraValid,static_cast<int>(cameraType)));
 uintptr_t playerController=0,playerModel=0;ReadPlayerExteriorDriver(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),SafeRead,playerController,playerModel);expectedPlayerDriverController.store(playerController);expectedPlayerDriverModel.store(playerModel);
 PollGesturePoseFile();PollHeadTrackingGain();PollHeadTrackingLimits();UpdateHeadTorso();PollSignalActivationDelay();PollSignalActuationSeconds();PollLeftElbowInset();PollRightElbowInset();
 PlayerEngineState engineState{};
 const bool engineValid=ReadPlayerEngine(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),SafeRead,engineState);
 if(engineValid){const int value=static_cast<int>(engineState);if(priorEngineSample>=0 && priorEngineSample!=1 && value==1){if(!skipNextEngineStartEvent.exchange(false))engineStartSequence.fetch_add(1);}priorEngineSample=value;}else priorEngineSample=-1;
 engineStarting.store(engineValid && engineState==PlayerEngineState::Starting);engineTick.store(engineValid?GetTickCount64():0);
 PollDeferredEngineStart(engineValid,engineState);
 static int priorEngine=-2;const int engineValue=engineValid?static_cast<int>(engineState):-1;
 if(engineValue!=priorEngine){char message[128];std::snprintf(message,sizeof(message),"ENGINE_STATE valid=%d state=%d (0=off 1=starting 2=running 3=stopping)",engineValid?1:0,engineValue);Log(SPF_LOG_INFO,message);priorEngine=engineValue;}
 bool held=false;
 const bool valid=ReadPlayerHorn(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),SafeRead,held);
 gameHornValid.store(valid);hornHeld.store(valid && held);hornTick.store(valid?GetTickCount64():0);
 if(gestureKeys){windowGestureHeld.store(core->keybinds->Kbind_GetActionValue(gestureKeys,"Gestures.LeftWindowBird")>.5f);windowGestureTick.store(GetTickCount64());rightGestureHeld.store(core->keybinds->Kbind_GetActionValue(gestureKeys,"Gestures.RightHandBird")>.5f);rightGestureTick.store(GetTickCount64());}
 static bool installed=false;
 if(!installed && core->hooks->Hook_IsInstalled(driverHook) && core->hooks->Hook_IsInstalled(advanceHook) && core->hooks->Hook_IsInstalled(boneHook) && core->hooks->Hook_IsInstalled(engineRequestHook)) {
  installed=true; Log(SPF_LOG_INFO,"ENGINE_DELAY: four hooks installed; guarded manual starts deferred until key reach with cancellation and timeout fallback.");
 }
 static uint64_t last=0;
 const uint64_t tick=GetTickCount64(); if(tick-last<250) return; last=tick;
 SampleLedger copy;std::array<ModelObservation,16> models;size_t inspectedSnapshot=0;
 {std::lock_guard lock(sampleMutex);copy=ledger;models=modelObservations;inspectedSnapshot=inspectedCount;}
 char text[512];
 static uint64_t lastGameHornLog=0;static int lastGameHornState=-1;
 const int gameState=gameHornValid.load()?(hornHeld.load()?1:0):-1;
 if(gameState!=lastGameHornState || tick-lastGameHornLog>=2000) {
  lastGameHornState=gameState;lastGameHornLog=tick;
  std::snprintf(text,sizeof(text),"GAME_HORN valid=%d held=%d",gameHornValid.load()?1:0,hornHeld.load()?1:0);Log(SPF_LOG_INFO,text);
 }
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
  std::snprintf(text,sizeof(text),"EXTERIOR_HEAD_TRACKING updates=%llu rejected=%llu",static_cast<unsigned long long>(exteriorHeadUpdates.load()),static_cast<unsigned long long>(exteriorHeadRejected.load()));Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"HEAD_TORSO updates=%llu rejected=%llu strength=%.4f hand_error_m=%.6f",static_cast<unsigned long long>(headTorsoUpdates.load()),static_cast<unsigned long long>(headTorsoRejected.load()),headTorsoStrength.load(),headTorsoError.load());Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"SIGNAL_DELAY_HEALTH queued=%llu released=%llu timeouts=%llu cancels=%llu",static_cast<unsigned long long>(signalQueued.load()),static_cast<unsigned long long>(signalReleased.load()),static_cast<unsigned long long>(signalTimeouts.load()),static_cast<unsigned long long>(signalCancels.load()));Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"ENGINE_DELAY_HEALTH queued=%llu released=%llu timeouts=%llu cancels=%llu",static_cast<unsigned long long>(engineDeferredCount.load()),static_cast<unsigned long long>(engineReleasedCount.load()),static_cast<unsigned long long>(engineTimeoutCount.load()),static_cast<unsigned long long>(engineDelayCancels.load()));Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"ENGINE_REQUEST_HEALTH total=%llu player=%llu",static_cast<unsigned long long>(engineRequestCalls.load()),static_cast<unsigned long long>(playerEngineRequestCalls.load()));Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"HEAD_VISIBILITY hidden_frames=%llu camera=%d valid=%d",static_cast<unsigned long long>(hiddenHeadFrames.load()),bodyCameraType.load(),bodyCameraValid.load()?1:0);Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"PLAYER_EXTERIOR candidate_controller=%p candidate_model=%p confirmed_model=%p matches=%llu hidden_frames=%llu mode=%d",reinterpret_cast<void*>(expectedPlayerDriverController.load()),reinterpret_cast<void*>(expectedPlayerDriverModel.load()),reinterpret_cast<void*>(confirmedPlayerDriverModel.load()),static_cast<unsigned long long>(playerDriverMatches.load()),static_cast<unsigned long long>(hiddenStockFrames.load()),bodyVisibilityMode.load());Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"BODY_VISIBILITY camera=%d valid=%d hide=%d hidden_frames=%llu faults=%llu",bodyCameraType.load(),bodyCameraValid.load()?1:0,hideBody.load()?1:0,static_cast<unsigned long long>(hiddenBodyFrames.load()),static_cast<unsigned long long>(visibilityFaults.load()));Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"TURN_SIGNAL events=%llu overrides=%llu left=%d right=%d",static_cast<unsigned long long>(signalSequence.load()),static_cast<unsigned long long>(signalOverrides.load()),selectedLeft.load()?1:0,selectedRight.load()?1:0);Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"RIGHT_ELBOW updates=%llu rejected=%llu guide_inset_m=%.4f",static_cast<unsigned long long>(rightElbowUpdates.load()),static_cast<unsigned long long>(rightElbowRejected.load()),rightElbowInset.load());Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"LEFT_ELBOW updates=%llu rejected=%llu guide_inset_m=%.4f",static_cast<unsigned long long>(leftElbowUpdates.load()),static_cast<unsigned long long>(leftElbowRejected.load()),leftElbowInset.load());Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"RUNTIME_IK updates=%llu rejected=%llu lean_degrees=%.4f hand_error_m=%.6f",static_cast<unsigned long long>(runtimeIKUpdates.load()),static_cast<unsigned long long>(runtimeIKRejected.load()),runtimeIKDegrees.load(),runtimeIKError.load());Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"IGNITION events=%llu overrides=%llu starting=%d",static_cast<unsigned long long>(engineStartSequence.load()),static_cast<unsigned long long>(ignitionOverrides.load()),engineStarting.load()?1:0);Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"RIGHT_GESTURE overrides=%llu held=%d",static_cast<unsigned long long>(rightGestureOverrides.load()),rightGestureHeld.load()?1:0);Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"WINDOW_GESTURE overrides=%llu held=%d",static_cast<unsigned long long>(windowGestureOverrides.load()),windowGestureHeld.load()?1:0);Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"HORN overrides=%llu faults=%llu held=%d",static_cast<unsigned long long>(hornOverrides.load()),static_cast<unsigned long long>(hornFaults.load()),hornHeld.load()?1:0);Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"CLUTCH overrides=%llu input=%.6f ankle=%.6f",
   static_cast<unsigned long long>(clutchOverrides.load()),lastClutch.load(),lastClutchPress.load());
  Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"BRAKE overrides=%llu input=%.6f transfer=%.6f ankle=%.6f start_frame=%u",
   static_cast<unsigned long long>(brakeOverrides.load()),lastBrake.load(),lastTransfer.load(),lastAnklePress.load(),lastPedalFrame.load());
  Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"STEERING overrides=%llu faults=%llu normalized=%.6f THROTTLE overrides=%llu input=%.6f start_frame=%u",
   static_cast<unsigned long long>(steeringOverrides.load()),static_cast<unsigned long long>(steeringFaults.load()),lastPose.load(),
   static_cast<unsigned long long>(throttleOverrides.load()),lastThrottle.load(),ThrottleStartFrame(lastThrottle.load()));
  Log(SPF_LOG_INFO,text);
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
// Blend only requested private arm rotations before the original hierarchy assembly.
// The current steering pose is freshly computed by the engine on every call.
extern "C" void CaptureBone(uintptr_t output,uintptr_t bounds,uintptr_t arrays,uintptr_t bones,uint32_t count) noexcept {
 if(!enabled.load() || count!=61 || bounds<0x1e0)return;
 const uintptr_t model=bounds-0x1e0;char path[192]{};
 if(!ReadModelPath(model,path) || std::strcmp(path,"/model/spf_stock_driver/driver.pmd"))return;
 uintptr_t expected=0,pointers[4]{};
 if(!SafeRead(model+0x1c0,&expected,8) || output!=expected || !SafeRead(arrays,pointers,sizeof(pointers)))return;
 privateBodyTick.store(GetTickCount64());
 float arm[4]{},hand[4]{},handPosition[3]{};
 if(!SafeRead(pointers[2]+5*16,arm,sizeof(arm)) || !SafeRead(pointers[2]+7*16,hand,sizeof(hand)) || !SafeRead(pointers[3]+7*12,handPosition,sizeof(handPosition)))return;
 for(float value:arm)if(!std::isfinite(value))return;
 for(float value:hand)if(!std::isfinite(value))return;
 static thread_local LeftArmGestures gestures;
 static thread_local SignalMotion signal;static thread_local uint64_t seenSignal=0,seenSignalCancels=0;
 static thread_local WindowGestureMotion rightMotion;
 static thread_local uint64_t observedDelayCancels=0;
 static thread_local IgnitionMotion ignition;static thread_local uint64_t seenEngineStart=0;
 static thread_local uintptr_t priorModel=0;
 static thread_local uint64_t priorGeneration=0,priorTick=0;
 const uint64_t now=GetTickCount64(),generation=worldGeneration.load();
 if(priorModel!=model || priorGeneration!=generation){gestures={};rightMotion={};ignition={};signal={};seenSignal=signalSequence.load();seenEngineStart=engineStartSequence.load();priorTick=now;priorModel=model;priorGeneration=generation;}
 const float dt=std::clamp(static_cast<float>(now-priorTick)/1000.f,0.f,.05f);priorTick=now;
 const auto inputTick=hornTick.load(),telTick=telemetryTick.load();
 const bool held=hornHeld.load() && inputTick && now-inputTick<=250 && telTick && now-telTick<=250;
 const auto gestureTick=windowGestureTick.load();
 const bool windowHeld=windowGestureHeld.load() && gestureTick && now-gestureTick<=250 && telTick && now-telTick<=250;
 const auto sigSequence=signalSequence.load(),sigTick=signalTick.load(),sigEvent=signalEventTick.load();
 const bool signalFresh=sigTick && now-sigTick<=250 && telTick && now-telTick<=250;
 const auto cancelSignal=signalCancels.load();if(cancelSignal!=seenSignalCancels){seenSignalCancels=cancelSignal;signal.phase=signal.reach>0?SignalMotion::Return:SignalMotion::Idle;seenSignal=sigSequence;}
 const bool pendingSignal=sigSequence!=seenSignal;
 if(signal.phase!=SignalMotion::Idle)gestures.Step(false,false,dt);
 else gestures.Step(held,windowHeld && !pendingSignal,dt);
 if(pendingSignal && signal.phase==SignalMotion::Idle && gestures.horn.reach==0 && gestures.window.reach==0 && !held){seenSignal=sigSequence;if(signalFresh && sigEvent && now-sigEvent<2000)signal.Start(signalDirection.load());}
 signal.Step(signalFresh,held,dt,signalActuationSeconds.load());
 const auto rightTick=rightGestureTick.load();
 const bool rightHeld=rightGestureHeld.load() && rightTick && now-rightTick<=250 && telTick && now-telTick<=250;
 const auto cancels=engineDelayCancels.load();if(cancels!=observedDelayCancels){observedDelayCancels=cancels;ignition.active=false;seenEngineStart=engineStartSequence.load();}
 const auto engTick=engineTick.load(),engSequence=engineStartSequence.load();
 const bool engineFresh=engTick && now-engTick<=250 && telTick && now-telTick<=250;
 const bool pendingStart=engSequence!=seenEngineStart;
 rightMotion.Step(rightHeld && !pendingStart && ignition.reach==0 && !ignition.active,dt);
 if(pendingStart && rightMotion.reach==0){seenEngineStart=engSequence;if(engineFresh)ignition.Start();}
 ignition.Step(engineStarting.load(),engineFresh,dt);
 const auto delayedSequence=deferredEngineSequence.load();
 const bool leftActive=gestures.horn.reach>0 || gestures.window.reach>0 || signal.reach>0,rightActive=rightMotion.reach>0 || ignition.reach>0;
 const HeadTorsoDrive trackedHead=SnapshotHeadDrive();
 const bool headTorsoActive=bodyCameraValid.load() && bodyCameraType.load()>=0 && bodyCameraType.load()<=9 && trackedHead.tick && now-trackedHead.tick<=250 && (DriverIK::Length(trackedHead.delta)>1e-5f || std::abs(trackedHead.yaw)>1e-5f);
  const bool exteriorHeadActive=bodyCameraValid.load() && bodyCameraType.load()>=0 && bodyCameraType.load()<=9 && bodyCameraType.load()!=2 && trackedHead.tick && now-trackedHead.tick<=250;
 const bool elbowFresh=telTick && now-telTick<=250;
 const bool elbowActive=leftElbowInset.load()>0 && elbowFresh,rightElbowActive=rightElbowInset.load()>0 && elbowFresh;
 if((leftActive || rightActive || elbowActive || rightElbowActive || headTorsoActive || exteriorHeadActive) && !hornFaults.load()) {
  std::array<std::array<float,4>,61> rotations{};
  if(!SafeRead(pointers[2],rotations.data(),sizeof(rotations)))return;
  for(const auto& q:rotations){float norm=0;for(float v:q){if(!std::isfinite(v))return;norm+=v*v;}if(norm<.98f || norm>1.02f)return;}
  BlendHornPose(rotations,gestures.horn);
  BlendLiveWindowGesture(rotations,gestures.window);
  if(signal.reach>0)BlendLiveSignal(rotations,signal);
  BlendLiveRightGesture(rotations,rightMotion);
  DriverIK::Pose unturned{};
  if(ignition.reach>0)BlendLiveIgnition(rotations,unturned,ignition);
  DriverIK::Positions headPositions{};DriverIK::Q headNeutralWorld{1,0,0,0};
  bool headPositionsValid=exteriorHeadActive && SafeRead(pointers[3],headPositions.data(),sizeof(headPositions));
  if(headPositionsValid)headNeutralWorld=DriverIK::FK(rotations,headPositions).q[47];
  bool torsoApplied=false;
  if(ignition.reach>0){
   DriverIK::Positions positions{};std::array<std::array<float,3>,61> scales{};
   float degrees=0,error=0;bool unitScale=true;
   if(!SafeRead(pointers[1],scales.data(),sizeof(scales)))unitScale=false;
   if(unitScale)for(const auto& scale:scales)for(float value:scale)if(!std::isfinite(value) || std::abs(value-1.f)>.001f)unitScale=false;
   const float lean=ignition.reach*ignition.reach*(3-2*ignition.reach);
   bool solved=false;
   if(unitScale && SafeRead(pointers[3],positions.data(),sizeof(positions))){
    auto basePose=DriverIK::FK(unturned,positions),turnPose=DriverIK::FK(rotations,positions);
    auto pivotOffset=DriverIK::Sub(basePose.p[43],turnPose.p[43]);
    solved=DriverIK::LeanAndCompensate(rotations,positions,lean,degrees,error,.1524f,pivotOffset);
   }
   if(solved){
    torsoApplied=degrees>0;runtimeIKUpdates.fetch_add(1);runtimeIKDegrees.store(degrees);runtimeIKError.store(error);
   }else{runtimeIKRejected.fetch_add(1);runtimeIKDegrees.store(0);}
  }
  bool headTorsoApplied=false;
  if(headTorsoActive){
   DriverIK::Positions positions{};std::array<std::array<float,3>,61> scales{};bool unitScale=SafeRead(pointers[1],scales.data(),sizeof(scales));
   if(unitScale)for(const auto& scale:scales)for(float value:scale)if(!std::isfinite(value) || std::abs(value-1.f)>.001f)unitScale=false;
   float strength=0,error=0;
   if(unitScale && SafeRead(pointers[3],positions.data(),sizeof(positions)) && DriverIK::TrackHeadTorso(rotations,positions,trackedHead.delta,trackedHead.yaw,strength,error,headLeanLimitDegrees.load())){
    headTorsoApplied=strength>0;headTorsoUpdates.fetch_add(1);headTorsoStrength.store(strength);headTorsoError.store(error);
   }else headTorsoRejected.fetch_add(1);
  }
  bool elbowApplied=false;
  if(elbowActive){
   DriverIK::Positions positions{};std::array<std::array<float,3>,61> scales{};bool valid=true;float error=0;
   if(!SafeRead(pointers[1],scales.data(),sizeof(scales)))valid=false;
   if(valid)for(const auto& scale:scales)for(float value:scale)if(!std::isfinite(value) || std::abs(value-1.f)>.001f)valid=false;
   if(valid && SafeRead(pointers[3],positions.data(),sizeof(positions)) && DriverIK::InsetLeftElbow(rotations,positions,leftElbowInset.load(),error)){elbowApplied=true;leftElbowUpdates.fetch_add(1);}else leftElbowRejected.fetch_add(1);
  }
  bool rightElbowApplied=false;
  if(rightElbowActive){
   DriverIK::Positions positions{};std::array<std::array<float,3>,61> scales{};bool valid=true;float error=0;
   if(!SafeRead(pointers[1],scales.data(),sizeof(scales)))valid=false;
   if(valid)for(const auto& scale:scales)for(float value:scale)if(!std::isfinite(value) || std::abs(value-1.f)>.001f)valid=false;
   if(valid && SafeRead(pointers[3],positions.data(),sizeof(positions)) && DriverIK::InsetRightElbow(rotations,positions,rightElbowInset.load(),error)){rightElbowApplied=true;rightElbowUpdates.fetch_add(1);}else rightElbowRejected.fetch_add(1);
  }
  bool exteriorHeadApplied=false;
  if(headPositionsValid){
   if(DriverIK::ApplyTrackedHead(rotations,headPositions,trackedHead.headRotation,headNeutralWorld)){exteriorHeadApplied=true;exteriorHeadUpdates.fetch_add(1);}else exteriorHeadRejected.fetch_add(1);
  }
  __try {
   if(rightElbowApplied)for(unsigned bone: {26u,27u,28u})std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);
   if(elbowApplied)for(unsigned bone: {5u,6u,7u})std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);
   if(exteriorHeadApplied)for(unsigned bone:{46u,47u})std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);
   if(headTorsoApplied)for(unsigned bone:{5u,6u,7u,26u,27u,28u})std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);
   if(torsoApplied || headTorsoApplied){
    for(unsigned bone=1;bone<=3;++bone)std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);
    for(unsigned bone: {5u,6u,7u})std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);
   }
   if(leftActive)for(const auto& target:kHornTargets)std::memcpy(reinterpret_cast<void*>(pointers[2]+target.bone*16),rotations[target.bone].data(),16);
   if(rightActive)for(const auto& target:kRightGestureTargets)std::memcpy(reinterpret_cast<void*>(pointers[2]+target.bone*16),rotations[target.bone].data(),16);
   if(rightMotion.reach>0)rightGestureOverrides.fetch_add(1);
   if(ignition.reach>0)ignitionOverrides.fetch_add(1);
   if(delayedSequence && seenEngineStart==delayedSequence && ignition.reach>=1 && torsoApplied)engineKeyReached.store(delayedSequence);
   if(signal.phase!=SignalMotion::Idle && signal.reach>0 && signalFresh && !held && signal.age>=signalActivationDelay.load())signalReady.store(seenSignal);
   if(signal.reach>0)signalOverrides.fetch_add(1);
   if(gestures.horn.reach>0)hornOverrides.fetch_add(1);
   if(gestures.window.reach>0)windowGestureOverrides.fetch_add(1);
  } __except(EXCEPTION_EXECUTE_HANDLER){hornFaults.fetch_add(1);}
 }
 const auto sample=boneSamples.fetch_add(1)+1;
 if(sample<=3 || sample%120==0) {
  char text[512];std::snprintf(text,sizeof(text),"HORN_BONE sample=%llu count=%u held=%d arm_quat=%.6f,%.6f,%.6f,%.6f hand_quat=%.6f,%.6f,%.6f,%.6f hand_local=%.6f,%.6f,%.6f bones=%p",
   static_cast<unsigned long long>(sample),count,hornHeld.load()?1:0,arm[0],arm[1],arm[2],arm[3],hand[0],hand[1],hand[2],hand[3],handPosition[0],handPosition[1],handPosition[2],reinterpret_cast<void*>(bones));Log(SPF_LOG_INFO,text);
 }
}
extern "C" void PostBone(uintptr_t output,uintptr_t bounds,uint32_t count) noexcept {
 if(!enabled.load() || visibilityFaults.load() || count!=61 || bounds<0x1e0)return;
 const uintptr_t model=bounds-0x1e0;char path[192]{};uintptr_t expected=0;
 if(!ReadModelPath(model,path) || !SafeRead(model+0x1c0,&expected,8) || output!=expected)return;
 const bool privateModel=!std::strcmp(path,"/model/spf_stock_driver/driver.pmd");
 const bool privateBody=hideBody.load() && privateModel;
 const bool privateHead=privateModel && !privateBody && (!bodyCameraValid.load() || bodyCameraType.load()==2);
 const auto now=GetTickCount64(),tick=playerDriverTick.load();
 const bool playerStock=hideStockDriver.load() && model==expectedPlayerDriverModel.load() && model==confirmedPlayerDriverModel.load() && tick && now-tick<=750 && !std::strcmp(path,"/asset/character/human/driver/man/driver.pmd");
 if(!privateBody && !playerStock && !privateHead)return;
 DriverPalette palette{};if(!SafeRead(output,palette.data(),sizeof(palette)))return;
 if(privateHead){if(!CollapseDriverHead(palette))return;}else if(!CollapseDriverPalette(palette))return;
 __try{std::memcpy(reinterpret_cast<void*>(output),palette.data(),sizeof(palette));if(privateBody)hiddenBodyFrames.fetch_add(1);if(playerStock)hiddenStockFrames.fetch_add(1);if(privateHead)hiddenHeadFrames.fetch_add(1);}
 __except(EXCEPTION_EXECUTE_HANDLER){visibilityFaults.fetch_add(1);}
}
extern "C" void CaptureHeadViews(uintptr_t pointer,uint32_t count,uint64_t flags,int32_t result,uintptr_t info) noexcept {
 if(!enabled.load())return;
 std::array<ObservedXRView,2> views{};ObservedXRLocate locate{};ObservedHead pose;
 if(result<0 || count!=2 || !SafeRead(pointer,views.data(),sizeof(views)) || !SafeRead(info,&locate,sizeof(locate)) || !HeadFromStereo(views,flags,locate,pose)){headRejected.fetch_add(1);return;}
 pose.tick=GetTickCount64();{std::lock_guard lock(headMutex);latestHead=pose;}headAccepted.fetch_add(1);
}
extern "C" unsigned char CaptureSignalRequest(uintptr_t vehicle,uint32_t side,uint32_t input) noexcept {
 const unsigned char edge=static_cast<unsigned char>(input);
 if(!enabled.load())return edge;
 const auto controller=expectedPlayerDriverController.load();if(!controller || vehicle!=controller-0x1df0 || !expectedPlayerDriverModel.load())return edge;
 unsigned char flags=0;if(!SafeRead(vehicle+0x248,&flags,1))return edge;
 const auto now=GetTickCount64(),tick=telemetryTick.load(),body=privateBodyTick.load();
 const bool fresh=tick && now-tick<=250 && body && now-body<=250;
 int outcome=0;uint64_t delay=0;bool wasPending=false;
 {std::lock_guard lock(signalGateMutex);
  wasPending=signalGate.vehicle!=0;delay=wasPending?now-signalGate.created:0;
  outcome=signalGate.Step(vehicle,worldGeneration.load(),now,fresh,flags,side,edge!=0,signalReady.load(),signalSequence.load()+1);
  if(outcome==1){signalReady.store(0);signalDirection.store(side);signalEventTick.store(now);signalSequence.store(signalGate.sequence);}
  if(wasPending && !signalGate.vehicle && outcome==0)signalCancels.fetch_add(1);
 }
 if(outcome==1){signalQueued.fetch_add(1);char text[128];std::snprintf(text,sizeof(text),"SIGNAL_DELAY queued direction=%u",side);Log(SPF_LOG_INFO,text);return 0;}
 if(outcome==2){signalCancels.fetch_add(1);Log(SPF_LOG_INFO,"SIGNAL_DELAY cancelled by repeated toggle.");return 0;}
 if(outcome==3 || outcome==4){signalReplayTick.store(now);skipSignalDirection.store(side);signalReleased.fetch_add(1);if(outcome==4)signalTimeouts.fetch_add(1);char text[160];std::snprintf(text,sizeof(text),"SIGNAL_DELAY released direction=%u reason=%s delay_ms=%llu",side,outcome==3?"stalk-contact":"timeout",static_cast<unsigned long long>(delay));Log(SPF_LOG_INFO,text);return 1;}
 return edge;
}
extern "C" bool CaptureEngineRequest(uintptr_t vehicle,uint32_t request,uint32_t immediate,uintptr_t returnAddress) noexcept {
 if(!enabled.load())return false;engineRequestCalls.fetch_add(1);
 const auto expected=expectedPlayerDriverController.load();
 if(!expected || vehicle!=expected-0x1df0 || !expectedPlayerDriverModel.load())return false;
 const auto sample=playerEngineRequestCalls.fetch_add(1)+1;
 uint32_t state=99;unsigned char electric=0;
 const bool valid=SafeRead(vehicle+0x1b8,&state,4) && state<=3 && SafeRead(vehicle+0x1bc,&electric,1) && electric<=1;
 const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 const bool manual=IsManualEngineStartCaller(base,returnAddress);
 if(sample<=128){char message[256];std::snprintf(message,sizeof(message),"ENGINE_REQUEST player=1 start=%u immediate=%u state_before=%u electric=%u valid=%d manual=%d caller_rva=%llx sample=%llu",request&255u,immediate&255u,state,electric,valid?1:0,manual?1:0,static_cast<unsigned long long>(returnAddress-base),static_cast<unsigned long long>(sample));Log(SPF_LOG_INFO,message);}
 if(!(request&255u)){
  bool cancelled=false;{std::lock_guard lock(engineGateMutex);cancelled=engineStartGate.Cancel();}
  if(cancelled){deferredEngineSequence.store(0);engineKeyReached.store(0);engineDelayCancels.fetch_add(1);Log(SPF_LOG_INFO,"ENGINE_DELAY cancelled by stop request.");}
  return false;
 }
 const auto now=GetTickCount64(),tick=telemetryTick.load(),bodyTick=privateBodyTick.load();
 if(!manual || !valid || state!=0 || (immediate&255u) || !electric || !tick || now-tick>250 || !bodyTick || now-bodyTick>250)return false;
 bool queued=false;
 {std::lock_guard lock(engineGateMutex);
  if(engineStartGate.pending.vehicle==vehicle)return true;
  if(engineStartGate.pending.vehicle)return false;
  const auto sequence=engineStartSequence.fetch_add(1)+1;
  queued=engineStartGate.Queue(vehicle,worldGeneration.load(),now,sequence);
  if(queued){deferredEngineSequence.store(sequence);engineKeyReached.store(0);}
 }
 if(queued){engineDeferredCount.fetch_add(1);Log(SPF_LOG_INFO,"ENGINE_DELAY queued: engine held off while hand reaches key.");}
 return queued;
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
 if(ValidSample(sample)) {
  currentDriver=sample;
  if(IsConfirmedPlayerDriver(controller,sample.model,expectedPlayerDriverController.load(),expectedPlayerDriverModel.load())){
   char path[192]{};if(ReadModelPath(sample.model,path) && !std::strcmp(path,"/asset/character/human/driver/man/driver.pmd")){confirmedPlayerDriverModel.store(sample.model);playerDriverTick.store(GetTickCount64());playerDriverMatches.fetch_add(1);}
  }
 }
}
// Invoke the same reset/enable sequence as the exterior driver's verified caller.
// Only the saved XMM2 float is changed; the assembly stub forwards everything else,
// including the original function's return value and caller's stack arguments.
void ApplyInteriorSteering(uintptr_t model,uint32_t index,float* replacement) noexcept {
 static thread_local bool applying=false;
 static thread_local PedalMotion motion;
 static thread_local uintptr_t motionModel=0;
 static thread_local uint32_t motionIndex=0;
 static thread_local uint64_t motionGeneration=0,motionTick=0;
 static thread_local float clutchPress=0;
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
  const uint32_t frames=frameQuery(model,index);
  const bool hasThrottle=IsThrottleClip(duration,frames);
  const bool hasBrake=IsBrakeClip(duration,frames);
  const bool hasClutch=IsClutchClip(duration,frames);
  if(!hasClutch && !hasBrake && !hasThrottle && !IsSteeringClip(duration,frames)) {applying=false;return;}
  const uint64_t now=GetTickCount64(),tick=telemetryTick.load();
  const float input=effectiveSteering.load();
  const bool fresh=tick && now-tick<=250;
  const float fraction=(fresh && std::isfinite(input)) ? SteeringFraction(input) : 0.f;
  const float gas=userThrottle.load();
  const float throttle=(fresh && std::isfinite(gas)) ? std::clamp(gas,0.f,1.f) : 0.f;
  const float brake=fresh?PedalValue(userBrake.load()):0.f;
  const float clutch=fresh?PedalValue(userClutch.load()):0.f;
  uint32_t startFrame=hasThrottle?ThrottleStartFrame(throttle):0;
  if(hasBrake || hasClutch) {
   const auto generation=worldGeneration.load();
   if(model!=motionModel || index!=motionIndex || generation!=motionGeneration) {
    motion={};clutchPress=0;motionTick=now;motionModel=model;motionIndex=index;motionGeneration=generation;
   }
   const auto prior=motion.phase;
   const float dt=std::clamp(static_cast<float>(now-motionTick)/1000.f,0.f,.05f);
   if(fresh) {
    motion.Step(throttle,brake,dt);
    clutchPress=Approach(clutchPress,clutch,dt/.1f);
   } else {motion={};clutchPress=0;}
   motionTick=now;startFrame=motion.StartFrame();
   if(prior!=motion.phase) {
    char phaseText[192];std::snprintf(phaseText,sizeof(phaseText),"PEDAL_PHASE %s brake=%.4f gas=%.4f transfer=%.4f ankle=%.4f",
     PedalPhaseName(motion.phase),brake,throttle,motion.transfer,motion.press);Log(SPF_LOG_INFO,phaseText);
   }
   lastBrake.store(brake);lastTransfer.store(motion.transfer);lastAnklePress.store(motion.press);lastPedalFrame.store(startFrame);
   brakeOverrides.fetch_add(1);
   if(hasClutch) {
    startFrame=ClutchStartFrame(startFrame,clutchPress);
    lastClutch.store(clutch);lastClutchPress.store(clutchPress);clutchOverrides.fetch_add(1);
   }
  }
  resetAnimation(model,index,startFrame);
  enableAnimation(model,index,true);
  *replacement=((hasClutch || hasBrake || hasThrottle) ? 1.9166667f : duration)*fraction;
  lastPose.store(fraction);steeringOverrides.fetch_add(1);
  if(hasClutch || hasBrake || hasThrottle) {lastThrottle.store(throttle);throttleOverrides.fetch_add(1);}
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
