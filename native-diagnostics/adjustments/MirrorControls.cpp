#include "../RetiredKeybinds.hpp"
#include <Windows.h>
#include <algorithm>
#include <bcrypt.h>
#include <mmsystem.h>
#include <filesystem>
#include <string>
#include <fstream>
#include "ServoLoop.hpp"
#include <SPF_Plugin.h>
#include <SPF_Manifest_API.h>
#include <SPF_Logger_API.h>
#include <SPF_Formatting_API.h>
#include "Signatures.hpp"
#include <SPF_Config_API.h>
#include <SPF_KeyBinds_API.h>
#include <SPF_GameWorld_API.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

extern "C" {
uintptr_t gOverrideContext=0;
unsigned int gOverrideSlot=0;
unsigned char gInputMask=0;
void* gLeftOriginal=nullptr;
void LeftInputStub();
void* gRightOriginal=nullptr;
void RightInputStub();
void* gUpOriginal=nullptr;
void UpInputStub();
void* gDownOriginal=nullptr;
void DownInputStub();
void* gResetOriginal=nullptr;
void ResetInputStub();

uintptr_t gSyntheticSeatContext=0;
void* gSeatUiOriginal=nullptr;
void* gSeatApplyReturn=nullptr;
void SeatUiStub();
void* gSelectOriginal=nullptr;
void* gSlotOriginal=nullptr;
void CenterSelectStub();
void CenterSlotStub();
}
namespace {
constexpr char kName[] = "SPF_VisibleFirstPersonDriverBody";
constexpr char kExecutableHash[] = "c60e8ee02cc97b3cc6ef8c6298bb5c3c23c4ffd330b8185b646fb1b608a3c7b4";
using UpdateFn = void(__fastcall*)(void*, float);
using SetRotationFn = void(__fastcall*)(void*, const float*);
UpdateFn originalUpdate = nullptr;
std::atomic<void(*)(uintptr_t,int)> displayObserver{nullptr};
SetRotationFn originalSetRotation = nullptr;
const SPF_Core_API* core = nullptr;
const SPF_Load_API* load = nullptr;
SPF_Logger_Handle* logger = nullptr;
SPF_Hook_Handle* updateHook = nullptr;
SPF_Hook_Handle* rotationHook = nullptr;
SPF_Hook_Handle* selectHook=nullptr;
SPF_Hook_Handle* slotHook=nullptr;
ServoLoop servo;
std::filesystem::path PluginFolder();
std::atomic<uintptr_t> currentContext{0};
std::array<SPF_Hook_Handle*,5> inputHooks{};
std::atomic<bool> movementAllowed{false};
std::mutex updateMutex;
#include "MirrorControllerBridge.hpp"
void Log(SPF_LogLevel,const char*);
bool SeatReady();
void ClearSeatCalibration();
void MoveNativeSeat(float dt,unsigned char mask,int mode=6);
void RegisterSeatHooks();
bool HooksReady() {
 if (!core || !core->hooks || !updateHook || !rotationHook || !selectHook || !slotHook) return false;
 for(auto h:inputHooks) if(!h || !core->hooks->Hook_IsInstalled(h) || !core->hooks->Hook_IsEnabled(h)) return false;
 for (auto h:{updateHook,rotationHook,selectHook,slotHook})
  if (!core->hooks->Hook_IsInstalled(h) || !core->hooks->Hook_IsEnabled(h)) return false;
 return true;
}
void MapContext(uintptr_t) noexcept;
void WorldReset() { ClearSeatCalibration(); controllerMirrorLease.Put(-1,0,0,GetTickCount64()); servo.SetPlaying(false); movementAllowed.store(false); currentContext.store(0); MapContext(0); }

std::atomic<bool> observing{false};
std::atomic<uint64_t> contextCalls{0};

// SEH is isolated from C++ objects with destructors. Only reads diagnostic data.
bool SafeRead(uintptr_t address, void* output, size_t bytes) noexcept {
    if (!address || !output || bytes > 4096) return false;
    __try { std::memcpy(output, reinterpret_cast<const void*>(address), bytes); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
template<class T> bool Read(uintptr_t address, T& value) noexcept {
    return SafeRead(address, &value, sizeof(value));
}
struct Event {
    int slot = -1;
    uint64_t calls = 0;
    bool hasAngles = false;
    std::array<float, 3> angles{};
    std::array<float, 4> rotation{};
};
struct State {
    std::mutex mutex;
    uintptr_t entries = 0;
    uint64_t count = 0;
    uint64_t generation = 0;
    std::array<Event, 9> pending{};
    std::array<bool, 9> changed{};
    std::array<float, 36> snapshot{};
    bool hasSnapshot = false;
    uint64_t matchedCalls = 0;
} state;

int Slot(uintptr_t object, uintptr_t entries, uint64_t count) noexcept {
    if (!entries || count == 0 || count > 9 || object < entries) return -1;
    const uintptr_t relative = object - entries;
    if (relative % 16 || relative / 16 >= count) return -1;
    return static_cast<int>(relative / 16);
}
bool Finite(const float* values, size_t count) noexcept {
    for (size_t i=0; i<count; ++i) if (!std::isfinite(values[i])) return false;
    return true;
}
void MapContext(uintptr_t context) noexcept {
    uintptr_t actor=0, mirrors=0, entries=0;
    uint64_t count=0;
    bool valid = Read(context+0x118, actor) && actor &&
        Read(actor+0x1f8, mirrors) && mirrors &&
        Read(mirrors+0x1d0, entries) && entries &&
        Read(mirrors+0x1d8, count) && count > 0 && count <= 9;
    std::array<float,36> sample{};
    valid = valid && SafeRead(entries, sample.data(), static_cast<size_t>(count)*16) &&
        Finite(sample.data(), static_cast<size_t>(count)*4);
    if (!valid) { entries=0; count=0; }
    std::lock_guard lock(state.mutex);
    if (state.entries != entries || state.count != count) {
        state.entries=entries;
        state.count=count;
        ++state.generation;
        state.hasSnapshot=false;
        state.changed.fill(false);
        state.pending.fill(Event{});
    }
}
void CaptureSnapshot() noexcept {
    uintptr_t entries; uint64_t count, generation;
    { std::lock_guard lock(state.mutex); entries=state.entries; count=state.count; generation=state.generation; }
    if (!entries || !count) return;
    std::array<float,36> sample{};
    if (!SafeRead(entries,sample.data(),static_cast<size_t>(count)*16) ||
        !Finite(sample.data(),static_cast<size_t>(count)*4)) return;
    std::lock_guard lock(state.mutex);
    if (state.generation!=generation) return;
    if (state.hasSnapshot) {
        for (size_t i=0;i<count;++i) {
            if (std::memcmp(sample.data()+i*4,state.snapshot.data()+i*4,16)) {
                auto& e=state.pending[i];
                e.slot=static_cast<int>(i);
                if (std::memcmp(e.rotation.data(),sample.data()+i*4,16)) e.hasAngles=false;
                std::memcpy(e.rotation.data(),sample.data()+i*4,16);
                state.changed[i]=true;
            }
        }
    }
    state.snapshot=sample;
    state.hasSnapshot=true;
}
bool CanOverride(uintptr_t context,int slot) {
 if (slot<0 || slot>=6 || !movementAllowed.load()) return false;
 std::lock_guard lock(state.mutex);
 return context && state.entries && static_cast<uint64_t>(slot)<state.count && state.count<=9;
}
void DetourUpdate(void* context,float dt) {
 // Serializes scoped register override across calls; no host input or object writes.
 std::lock_guard callLock(updateMutex);
 if(observing.load()) {
  currentContext.store(reinterpret_cast<uintptr_t>(context)); ++contextCalls;
  MapContext(reinterpret_cast<uintptr_t>(context));
 }
 int slot=-1;unsigned char directions=0;
 if(movementAllowed.load())controllerMirrorLease.Get(GetTickCount64(),slot,directions);
 bool enabled=CanOverride(reinterpret_cast<uintptr_t>(context),slot);
 if (enabled && std::isfinite(dt) && dt>=0 && dt<=0.1f)
  { gOverrideSlot=static_cast<unsigned int>(slot); gInputMask=directions; gOverrideContext=reinterpret_cast<uintptr_t>(context); }
 else gOverrideContext=0;
 int beforeDisplay=-1;auto observer=displayObserver.load();if(observer)SafeRead(reinterpret_cast<uintptr_t>(context)+0x15d4,&beforeDisplay,sizeof(beforeDisplay));
 originalUpdate(context,dt); // Exactly one call; native getter, setter and camera refresh.
 if(observer)observer(reinterpret_cast<uintptr_t>(context),beforeDisplay);
 gOverrideContext=0; gInputMask=0;
 if(slot>=6 && movementAllowed.load()){if(slot==9){MoveNativeSeat(dt,directions,7);MoveNativeSeat(dt,directions,8);}else MoveNativeSeat(dt,directions,slot);}
}
void DetourSetRotation(void* object, const float* angles) {
    int slot=-1; uint64_t generation=0;
    std::array<float,3> requested{};
    bool gotAngles=false;
    if (observing.load(std::memory_order_relaxed)) {
        { std::lock_guard lock(state.mutex); slot=Slot(reinterpret_cast<uintptr_t>(object),state.entries,state.count); generation=state.generation; }
        if (slot>=0) gotAngles=SafeRead(reinterpret_cast<uintptr_t>(angles),requested.data(),sizeof(requested)) && Finite(requested.data(),3);
    }
    originalSetRotation(object,angles); // Exactly the original pointers, exactly once.
    if (slot<0) return;
    std::array<float,4> rotation{};
    if (!SafeRead(reinterpret_cast<uintptr_t>(object),rotation.data(),sizeof(rotation)) || !Finite(rotation.data(),4)) return;
    std::lock_guard lock(state.mutex);
    if (generation!=state.generation || slot!=Slot(reinterpret_cast<uintptr_t>(object),state.entries,state.count)) return;
    auto& e=state.pending[slot];
    e.slot=slot;
    ++e.calls;
    e.hasAngles=gotAngles;
    e.angles=requested;
    e.rotation=rotation;
    state.changed[slot]=true;
    ++state.matchedCalls;
}
void Log(SPF_LogLevel level,const char* message) {
    if (load && load->logger && logger) load->logger->Log(logger,level,message);
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
std::filesystem::path PluginFolder() {
 HMODULE module=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
   reinterpret_cast<LPCWSTR>(&gOverrideContext),&module)) return {};
 wchar_t path[32768]{};
 if(!GetModuleFileNameW(module,path,32768)) return {};
 return std::filesystem::path(path).parent_path();
}
#include "NativeSeat.inc"
void OnLoad(const SPF_Load_API* api) { load=api; if(api && api->config) { auto config=api->config->Cfg_GetContext(kName); if(api->config->Cfg_RemoveKey)for(const char* key:{"settings.hold_selectors","settings.voice_cues","settings.servo_sound","settings.selection_clicks"})api->config->Cfg_RemoveKey(config,key); } if (api && api->logger) logger=api->logger->Log_GetContext(kName); }
void OnActivated(const SPF_Core_API* api) {
    core=api;
    if(core && core->keybinds) {
      auto keys=core->keybinds->Kbind_GetContext(kName);
      if(core->keybinds->Kbind_RegisterActionMetadata&&core->keybinds->Kbind_UnregisterActionMetadata)RetireControllerKeybinds(
       [&](const char* action){core->keybinds->Kbind_RegisterActionMetadata(keys,action,"Retired controller binding","",nullptr,nullptr);},
       [&](const char* action){core->keybinds->Kbind_UnregisterActionMetadata(keys,action);});
    }
    if (!core || !core->hooks || !core->gameworld) { Log(SPF_LOG_ERROR,"DIAG: hooks API unavailable; observation disabled."); return; }
    char hash[65]{};
    if (!HashExecutable(hash) || std::strcmp(hash,kExecutableHash)) {
        Log(SPF_LOG_WARN,"DIAG: executable does not match the inspected ATS build; no hooks registered."); return;
    }
    auto hooks=core->hooks;
    const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if (hooks->Hook_FindPattern(kUpdatePattern)!=base+kUpdateRva || hooks->Hook_FindPattern(kSetRotationPattern)!=base+kSetRotationRva || hooks->Hook_FindPattern(kSelectPattern)!=base+kSelectRva || hooks->Hook_FindPattern(kSlotPattern)!=base+kSlotRva || hooks->Hook_FindPattern(kLeftPattern)!=base+kLeftRva || hooks->Hook_FindPattern(kRightPattern)!=base+kRightRva || hooks->Hook_FindPattern(kUpPattern)!=base+kUpRva || hooks->Hook_FindPattern(kDownPattern)!=base+kDownRva || hooks->Hook_FindPattern(kResetPattern)!=base+kResetRva) {
        Log(SPF_LOG_ERROR,"DIAG: expected function signatures not found at verified locations; no hooks registered."); return;
    }
    if(!servo.Open(PluginFolder()/"sounds"/"servo.wav")) Log(SPF_LOG_WARN,"MIRRORS: servo sound unavailable; check sounds/servo.wav and Windows audio output.");
    RegisterSeatHooks();
    observing.store(true);
    updateHook=hooks->Hook_Register(kName,"MirrorContext","Observe mirror context",reinterpret_cast<void*>(DetourUpdate),reinterpret_cast<void**>(&originalUpdate),kUpdatePattern,true);
    rotationHook=hooks->Hook_Register(kName,"MirrorRotation","Observe mirror rotation",reinterpret_cast<void*>(DetourSetRotation),reinterpret_cast<void**>(&originalSetRotation),kSetRotationPattern,true);
    selectHook=hooks->Hook_Register(kName,"CenterSelect","Center selector override",reinterpret_cast<void*>(CenterSelectStub),&gSelectOriginal,kSelectPattern,true);
    slotHook=hooks->Hook_Register(kName,"CenterSlot","Center physical slot override",reinterpret_cast<void*>(CenterSlotStub),&gSlotOriginal,kSlotPattern,true);
    inputHooks[0]=hooks->Hook_Register(kName,"InputLeft","Mirror Left input",reinterpret_cast<void*>(LeftInputStub),&gLeftOriginal,kLeftPattern,true);
    inputHooks[1]=hooks->Hook_Register(kName,"InputRight","Mirror Right input",reinterpret_cast<void*>(RightInputStub),&gRightOriginal,kRightPattern,true);
    inputHooks[2]=hooks->Hook_Register(kName,"InputUp","Mirror Up input",reinterpret_cast<void*>(UpInputStub),&gUpOriginal,kUpPattern,true);
    inputHooks[3]=hooks->Hook_Register(kName,"InputDown","Mirror Down input",reinterpret_cast<void*>(DownInputStub),&gDownOriginal,kDownPattern,true);
    inputHooks[4]=hooks->Hook_Register(kName,"InputReset","Mirror Reset input",reinterpret_cast<void*>(ResetInputStub),&gResetOriginal,kResetPattern,true);
    Log(SPF_LOG_INFO,"MIRRORS: native adjustment hooks requested for controller interaction boxes.");
}
void OnUpdate() {
    if (!observing.load() || !core || !core->hooks) return;
    DWORD foregroundPid=0;
    GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
    const bool allowed=foregroundPid==GetCurrentProcessId() && core->gameworld && !core->gameworld->GW_IsGamePaused();
    movementAllowed.store(allowed && HooksReady());
    if (!allowed) controllerMirrorLease.Put(-1,0,0,GetTickCount64());
    int servoSlot=-1;unsigned char servoMask=0;
    if(movementAllowed.load())controllerMirrorLease.Get(GetTickCount64(),servoSlot,servoMask);
    servo.SetMirror(servoSlot>=6?4:servoSlot);
    servo.SetPlaying(ServoMovement(servoSlot,servoMask,true,movementAllowed.load()));
    static bool reported=false;
    if (!reported && HooksReady()) {
        Log(SPF_LOG_INFO,"DIAG: all nine hooks installed and enabled. Mirror controls ready when driving."); reported=true;
    }
    const auto context=currentContext.load(std::memory_order_relaxed);
    if (context) { MapContext(context); CaptureSnapshot(); }
    static uint64_t last=0;
    const uint64_t now=GetTickCount64();
    if (now-last<250) return;
    last=now;
    std::array<Event,9> pending{}; std::array<bool,9> changed{};
    static uint64_t lastGeneration=~uint64_t{0};
    uint64_t count,generation;
    { std::lock_guard lock(state.mutex); pending=state.pending; changed=state.changed; state.changed.fill(false); count=state.count; generation=state.generation; }
    char message[512];
    const auto fmt=core->formatting;
    if (!fmt) return;
    if (generation!=lastGeneration) {
        fmt->Fmt_Format(message,sizeof(message),"DIAG: mirror array discovered: %llu entries; capture generation %llu.",static_cast<unsigned long long>(count),static_cast<unsigned long long>(generation));
        Log(SPF_LOG_INFO,message); lastGeneration=generation;
    }
    for (size_t i=0;i<9;++i) if (changed[i]) {
        const auto& e=pending[i];
        fmt->Fmt_Format(message,sizeof(message),"DIAG: slot=%d rotation=(%.6f,%.6f,%.6f,%.6f) setter_calls=%llu angles_valid=%d angles=(%.6f,%.6f,%.6f)",
            static_cast<int>(i),e.rotation[0],e.rotation[1],e.rotation[2],e.rotation[3],static_cast<unsigned long long>(e.calls),e.hasAngles?1:0,e.angles[0],e.angles[1],e.angles[2]);
        Log(SPF_LOG_INFO,message);
    }
}
void OnUnload() {
    servo.Close();
    // SPF removes hook objects after OnUnload. Preserve trampolines until then.
    ClearSeatCalibration(); controllerMirrorLease.Put(-1,0,0,GetTickCount64()); movementAllowed.store(false);
    observing.store(false);
    currentContext.store(0);
}
}
extern "C" {
int __cdecl SetIntegratedAdjustmentInput(int slot,float x,float y){
 if(slot!=-1 && slot!=0 && slot!=2 && slot!=4 && slot!=6 && slot!=9)return 2;
 if(!std::isfinite(x)||!std::isfinite(y))return 2;
 if(slot>=0 && (!movementAllowed.load() || !HooksReady()))return 1;
 if((slot==6 || slot==9) && !SeatReady())return 3;
 controllerMirrorLease.Put(slot,std::clamp(x,-1.f,1.f),std::clamp(y,-1.f,1.f),GetTickCount64());return 0;
}

} // extern C
void LoadIntegratedAdjustments(const SPF_Load_API* api){OnLoad(api);}
void ActivateIntegratedAdjustments(const SPF_Core_API* api){OnActivated(api);}
void PollIntegratedAdjustments(){OnUpdate();}
void ResetIntegratedAdjustments(){WorldReset();}
void CloseIntegratedAdjustments(){OnUnload();}

void DescribeIntegratedAdjustments(SPF_Manifest_Builder_Handle* h,const SPF_Manifest_Builder_API* api){
 api->Policy_AddConfigurableSystem(h,"settings");
    api->Settings_SetJson(h,"{}");
    api->Defaults_SetLogging(h,"info",true);
}

void SetIntegratedDisplayObserver(void(*observer)(uintptr_t,int)){displayObserver.store(observer);}
