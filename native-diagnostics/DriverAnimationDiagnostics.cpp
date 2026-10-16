#include <SPF_VirtInput_API.h>
#include "MenuKeyPulse.hpp"
#include "NativeRadialActions.hpp"
#include "DriverSettings.hpp"
#include "IntegratedAdjustments.hpp"
#include "EarlyMenuCursor.hpp"
#include <thread>
#include "vendor/minhook/include/MinHook.h"
#include "NativeMenuCursor.hpp"
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
#include "NativeDriverRender.hpp"
#include "GameHorn.hpp"
#include "GameEngine.hpp"
#include "GameWindow.hpp"
#include "WiperCycle.hpp"
#include <SPF_GameWorld_API.h>
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
#include "GestureHeadFollow.hpp"
#include "ControllerRuntime.hpp"
#include "MenuLaser.hpp"
#include "NativeControllerInput.hpp"
#include "NativeRecenter.hpp"
#include "SignalTargets.hpp"
#include "DriverSamples.hpp"
#include "SteeringPose.hpp"
#include "PedalMotion.hpp"
#include "NativePedalPose.hpp"
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
#include <limits>

extern "C" {
void* gDriverOriginal=nullptr;
void* gEngineOriginal=nullptr;void* gHornControlOriginal=nullptr;void HornControlObserveStub();
void* gRadialHudOriginal=nullptr;void RadialHudObserveStub();
void* gRadialCameraOriginal=nullptr;void RadialCameraObserveStub();
void* gRadialActivateOriginal=nullptr;void RadialActivateObserveStub();
void* gRadialMapOriginal=nullptr;void RadialMapObserveStub();
void* gRadialQuickOriginal=nullptr;void RadialQuickObserveStub();
void* gRadialServicesOriginal=nullptr;void RadialServicesObserveStub();
void* gRecenterOriginal=nullptr;void NativeRecenterObserveStub();
void* gHeadOriginal=nullptr;void HeadObserveStub();
void* gXRActionAttachOriginal=nullptr;void XRActionAttachStub();
void* gRadioInputOriginal=nullptr;void RadioInputObserveStub();
void* gWipersOriginal=nullptr;void WipersRequestStub();
void* gLightsOriginal=nullptr;void LightsRequestStub();
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
using NativeDialogActionFn=bool(__fastcall*)(uintptr_t,uintptr_t,unsigned,uintptr_t);NativeDialogActionFn originalRadialDialogAction=nullptr;
bool __fastcall NativeRadialDialogAction(uintptr_t object,uintptr_t source,unsigned event,uintptr_t payload);
SPF_VirtualDevice_Handle* activateDevice=nullptr;std::atomic<bool> activateDeviceReady{false};MenuKeyPulse activatePulse;MenuKeyPulse displayPulses[4];
std::atomic<uint64_t> infotainmentCyclePending{0},parkingCameraRequest{0},infotainmentStateTick{0};std::atomic<bool> infotainmentCameraActive{false};void ObserveInfotainmentUpdate(uintptr_t,int);
std::atomic<uint64_t> radialActionTicks[7]{};bool radialActionsReady=false;
thread_local bool controllerIgnitionCycle=false;
constexpr char kName[]="SPF_VisibleFirstPersonDriverBody";
constexpr char kHash[]="c60e8ee02cc97b3cc6ef8c6298bb5c3c23c4ffd330b8185b646fb1b608a3c7b4";
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
std::atomic<int> bodyVisibilityMode{1};std::atomic<float> interiorHeadBackshift{0.f};std::atomic<bool> collapseInteriorHead{false};
std::atomic<bool> hideStockDriver{false};
std::atomic<uintptr_t> expectedPlayerDriverController{0},expectedPlayerDriverModel{0},confirmedPlayerDriverModel{0};
std::atomic<uint64_t> playerDriverTick{0},playerDriverMatches{0},hiddenStockFrames{0},hiddenHeadFrames{0};
using NativeDriverRenderFn=void(__fastcall*)(uintptr_t,uintptr_t);
NativeDriverRenderFn originalCabinModelRender=nullptr;
bool nativeDriverRenderReady=false;
void __fastcall RenderCabinModelWithBody(uintptr_t model,uintptr_t view);
void RefreshBorrowedDriverPose();
struct NativeRigAudit{uintptr_t model=0;DriverIK::V wrists[2]{};};
thread_local NativeRigAudit nativeRigAudit;


std::atomic<uint64_t> boneSamples{0},hornOverrides{0},hornFaults{0};
std::atomic<uint64_t> hornTick{0};
std::atomic<bool> gameHornValid{false};
SPF_KeyBinds_Handle* gestureKeys=nullptr;
LiveGesturePose liveGesturePose;
LiveGesturePose liveSignalLeft{kSignalLeftTargets},liveSignalRight{kSignalRightTargets};
SPF_Hook_Handle* nativeRecenterHook=nullptr;
constexpr char kNativeRecenterPattern[]="84 C9 74 76 49 8B 8C 24 08 0E 00 00 48 85 C9 74 06 48 8B 01 FF 50 58 48 8D 45";
std::atomic<uint64_t> nativeRecenterSequence{0},nativeRecenterTick{0},controllerRecenterTick{0};NativeRecenterGate nativeRecenterGate,recenterFeedbackGate;
SPF_Hook_Handle* headHook=nullptr;
SPF_Hook_Handle* controllerAttachHook=nullptr;
constexpr char kControllerAttachPattern[]="FF 96 38 01 00 00 8B D8 85 C0 79 58 48 8B 46 58 48 85 C0";
constexpr char kHeadPattern[]="8B D8 85 C0 0F 89 9F 00 00 00 48 8B 46 58 48 85 C0 74 7E 48 8B 4E 10 48 85";
std::mutex headMutex;ObservedHead latestHead{};
std::atomic<uint64_t> headAccepted{0},headRejected{0},headTorsoUpdates{0},headTorsoRejected{0},exteriorHeadUpdates{0},exteriorHeadRejected{0};
std::mutex headDriveMutex;HeadTorsoDrive headDrive{};SeatedHeadCalibration headCalibration;
std::atomic<float> headForwardLimit{.30f},headLeanLimitDegrees{28.f},headLateralLimit{.24f};
std::atomic<float> gestureHeadFollowGain{1.f};std::atomic<uint64_t> gestureHeadFollowUpdates{0},gestureHeadFollowRejected{0};
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
std::atomic<bool> enabled{false};std::atomic<bool> earlyMenuOwned{false};std::atomic<bool> earlyMenuStopped{false};
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
void EarlyMenuTrace(const char* message) noexcept {
 HMODULE self=nullptr;wchar_t path[MAX_PATH]{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&EarlyMenuTrace),&self)||!GetModuleFileNameW(self,path,MAX_PATH))return;
 auto slash=wcsrchr(path,L'\\');if(!slash)return;wcscpy_s(slash+1,MAX_PATH-(slash+1-path),L"logs\\controller_menu_early.log");FILE* file=nullptr;if(!_wfopen_s(&file,path,L"a")){SYSTEMTIME t{};GetLocalTime(&t);std::fprintf(file,"%04u-%02u-%02u %02u:%02u:%02u %s\n",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,message);std::fclose(file);}
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
bool IsBorrowedPlayerBody(uintptr_t model,const char* path) {
 return nativeDriverRenderReady && model && model==expectedPlayerDriverModel.load() && model==confirmedPlayerDriverModel.load() && !std::strcmp(path,"/asset/character/human/driver/man/driver.pmd");
}
bool UseAnimatedBody(uintptr_t model,const char* path) {
 if(IsBorrowedPlayerBody(model,path))return true;
 return !std::strcmp(path,"/model/spf_stock_driver/driver.pmd") && !(nativeDriverRenderReady && confirmedPlayerDriverModel.load() && confirmedPlayerDriverModel.load()==expectedPlayerDriverModel.load());
}
void __fastcall RenderCabinModelWithBody(uintptr_t model,uintptr_t view) {
 originalCabinModelRender(model,view);
 const auto now=GetTickCount64(),tick=telemetryTick.load();
 if(!view || !enabled.load() || !bodyCameraValid.load() || bodyCameraType.load()!=2 || !tick || now<tick || now-tick>250)return;
 char path[192]{};if(!ReadModelPath(model,path)||!IsCabinBodyAnchor(path))return;
 auto body=expectedPlayerDriverModel.load(),controller=expectedPlayerDriverController.load();
 uintptr_t current=0,table=0,draw=0;auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 if(!body||body==model||body!=confirmedPlayerDriverModel.load()||!controller||!SafeRead(controller+8,&current,8)||current!=body||!SafeRead(body,&table,8)||!SafeRead(table+0x90,&draw,8)||draw!=base+0x15e3f80)return;
 uint32_t frame=0;if(!SafeRead(base+0x369eaec,&frame,4))return;
 static thread_local NativeBodyViewLedger bodyViews;if(!bodyViews.Take(frame,worldGeneration.load(),view))return;
 // Submit in the interior model's own view, not the exterior vehicle's shadow /
 // mirror passes. ATS continues to own and animate the player driver instance.
 originalCabinModelRender(body,view);
 static std::atomic<uint64_t> draws{0};auto n=++draws;
 if(n==1||n%3000==0){uint64_t flags=0;float placement[8]{};SafeRead(view+0x18,&flags,8);SafeRead(body+0x30,placement,sizeof(placement));char text[384];std::snprintf(text,sizeof(text),"NATIVE_BODY interior-anchored draws=%llu anchor=%s flags=%llx position=%.4f,%.4f,%.4f frame=%u",n,path,flags,placement[0],placement[1],placement[2],frame);Log(SPF_LOG_INFO,text);}
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
SPF_Telemetry_Handle* vehicleTelemetry=nullptr;
void VehicleConstants(const SPF_TruckConstants* data,void*){if(!data)return;std::string brand(data->brand_id,strnlen_s(data->brand_id,sizeof(data->brand_id))),model(data->id,strnlen_s(data->id,sizeof(data->id)));SetControllerVehicleIdentity(brand.c_str(),model.c_str());
 bool valid=SetControllerVehicleCamera({data->cabin_position.x,data->cabin_position.y,data->cabin_position.z},{data->head_position.x,data->head_position.y,data->head_position.z});
 static std::string prior;static DriverIK::V priorHead{};DriverIK::V position{data->head_position.x,data->head_position.y,data->head_position.z};if(prior!=model||position!=priorHead){prior=model;priorHead=position;char text[256];std::snprintf(text,sizeof(text),"VEHICLE_CAMERA valid=%d cabin=%.4f,%.4f,%.4f head=%.4f,%.4f,%.4f",valid?1:0,data->cabin_position.x,data->cabin_position.y,data->cabin_position.z,position[0],position[1],position[2]);Log(SPF_LOG_INFO,text);}
}
extern "C" void ReportControllerVehiclePose(const char* path,int status) noexcept{char text[1024];std::snprintf(text,sizeof(text),"VEHICLE_POSE file=%s status=%d (1=live -1=invalid edit retained prior settings)",path,status);Log(status<0?SPF_LOG_WARN:SPF_LOG_INFO,text);}
extern "C" void ReportControllerSetupGraphics(int state,unsigned boxes) noexcept{char text[160];std::snprintf(text,sizeof(text),"SETUP_GRAPHICS state=%d boxes=%u (0=hidden 1=independent stereo 2=bridge update needed)",state,boxes);Log(state==2?SPF_LOG_WARN:SPF_LOG_INFO,text);}
extern "C" void ReportControllerVehicleBoxes(const char* model,const char* file,int status) noexcept{char text[1024];std::snprintf(text,sizeof(text),"VEHICLE_BOXES model=%s file=%s status=%d (1=loaded 2=Mustang migrated -1=load failed -2=old save blocked)",model,file,status);Log(status<0?SPF_LOG_WARN:SPF_LOG_INFO,text);}
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
 SetControllerVehicleIdentity("","");
 ResetIntegratedAdjustments();
 controllerRecenterTick.store(0);headCalibration.Reset();nativeRecenterGate={nativeRecenterSequence.load(),0,0};recenterFeedbackGate={nativeRecenterSequence.load(),0,0};{std::lock_guard lock(headDriveMutex);headDrive={};}
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
 api->Info_SetName(h,kName); api->Info_SetVersion(h,"0.39.0");
 api->Info_SetMinFrameworkVersion(h,"1.2.5");
 api->Info_SetAuthor(h,"Local driver animation diagnostics");
 api->Info_SetDescriptionLiteral(h,"Live steering, independent pedals, game horn and independent left/right middle-finger gestures with live pose files.");
 api->Policy_SetAllowUserConfig(h,true);
 api->Policy_AddConfigurableSystem(h,"logging");
 api->Defaults_SetLogging(h,"info",true);
 api->Policy_AddConfigurableSystem(h,"keybinds");
 api->Defaults_AddKeybind(h,"Gestures","LeftWindowBird","keyboard","KEY_F10","always");
 api->Defaults_AddKeybind(h,"Gestures","RightHandBird","keyboard","KEY_F11","always");
 DescribeIntegratedAdjustments(h,api);

}
void OnLoad(const SPF_Load_API* api) {
 LoadIntegratedAdjustments(api);
 load=api; if(api && api->logger) logger=api->logger->Log_GetContext(kName);
 activateDevice=nullptr;activateDeviceReady.store(false);activatePulse={};displayPulses[0]={};displayPulses[1]={};
 if(api&&api->input){auto input=api->input;if(input->Virt_CreateDevice&&input->Virt_AddButton&&input->Virt_Register&&input->Virt_PressButton&&input->Virt_ReleaseButton){activateDevice=input->Virt_CreateDevice(kName,"controller_actions","VR Controller Actions",SPF_INPUT_DEVICE_TYPE_SEMANTICAL);if(activateDevice){input->Virt_AddButton(activateDevice,"activate","Activate");input->Virt_AddButton(activateDevice,"display","Dashboard display mode");input->Virt_AddButton(activateDevice,"infotainment","Infotainment display mode");input->Virt_AddButton(activateDevice,"parking_cams","Parking cameras");input->Virt_AddButton(activateDevice,"dashmapzoom","Navigation zoom mode");activateDeviceReady.store(input->Virt_Register(activateDevice));}}}
 Log(SPF_LOG_INFO,activateDeviceReady.load()?"RADIAL_ACTIVATE semantic input device registered.":"RADIAL_ACTIVATE semantic device unavailable; restart ATS with plugin enabled.");
}
extern "C" {void* gMapWidgetOriginal=nullptr;void MapWidgetObserveStub();}
std::atomic<uint64_t> mapWidgetCalls{0},mapWidgetTick{0},worldMapWidgetTick{0};std::atomic<uintptr_t> mapWidgetVtable{0};
extern "C" bool IsWorldMapMenuActive() noexcept {auto tick=worldMapWidgetTick.load();auto now=GetTickCount64();return enabled.load()&&tick&&now>=tick&&now-tick<=100;}
extern "C" void CaptureMapWidget(uintptr_t object) noexcept {uintptr_t vt=0;if(SafeRead(object,&vt,8)){mapWidgetVtable.store(vt);mapWidgetCalls.fetch_add(1);mapWidgetTick.store(GetTickCount64());if(vt==reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))+0x239a150)worldMapWidgetTick.store(GetTickCount64());}}
extern "C" {void* gMenuModeOriginal=nullptr;void* gMenuSurfaceOriginal=nullptr;void MenuModeObserveStub();void MenuSurfaceObserveStub();}
extern "C" void ReportMenuShortcut(int x,int y,int enter,int escape,int key,int edge,unsigned sent) noexcept {char text[180];std::snprintf(text,sizeof(text),"MENU_SHORTCUT x=%d y=%d enter=%d escape=%d scan=%d edge=%d sent=%u",x,y,enter,escape,key,edge,sent);Log(SPF_LOG_INFO,text);EarlyMenuTrace(text);}
extern "C" void ReportMenuLaser(int owner,int leftHit,int rightHit,float leftTrigger,float rightTrigger,int button,unsigned sent,unsigned requested) noexcept {char text[192];std::snprintf(text,sizeof(text),"MENU_LASER owner=%d hit=%d,%d trigger=%.3f,%.3f button=%d mouse_events=%u/%u",owner,leftHit,rightHit,leftTrigger,rightTrigger,button,sent,requested);Log(SPF_LOG_INFO,text);}
std::atomic<bool> earlyMenuFocused{false};EarlyMenuCursor earlyCursor;thread_local bool earlyMenuWorker=false;
extern "C" void ReportMenuModel(unsigned side,const char* name,unsigned part,int status) noexcept {char text[600];std::snprintf(text,sizeof(text),"MENU_MODEL side=%u part=%u status=%d name=%s",side,part,status,name?name:"");Log(SPF_LOG_INFO,text);EarlyMenuTrace(text);}
using NativeCursorEmit=void(__fastcall*)(uintptr_t,uint64_t,const NativeMenuCursorEvent*);NativeCursorEmit nativeCursorEmit=nullptr;
extern "C" bool ApplyNativeMenuCursor(float u,float v) noexcept {
 if(earlyMenuWorker)return earlyCursor.Publish(u,v,GetTickCount64());
 if((!enabled.load() && (!earlyMenuOwned.load() || earlyMenuStopped.load())) || !nativeCursorEmit)return false;
 auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));uintptr_t inputRoot=0,input=0,vt=0,window=0;unsigned char alternate=0;float rect[4]{};NativeMenuCursorEvent event;
 if(!SafeRead(base+0x368de98,&inputRoot,8)||!SafeRead(inputRoot+0x100,&input,8)||!SafeRead(input,&vt,8)||vt!=base+0x2204e88||!SafeRead(input+0x170,&window,8)||!window||!SafeRead(base+0x368e14c,&alternate,1)||!SafeRead(base+(alternate?0x2d0a1f8:0x2d0a1e8),rect,sizeof(rect))||!BuildNativeMenuCursorEvent(u,v,rect,event))return false;
 __try {
  auto position=reinterpret_cast<float*>(input+0x340);float oldX=position[0],oldY=position[1];position[0]=event.x;position[1]=event.y;
  auto delta=reinterpret_cast<float*>(input+0x338);delta[0]=event.x-oldX;delta[1]=event.y-oldY;
  *reinterpret_cast<unsigned char*>(input+0x335)=1;*reinterpret_cast<unsigned char*>(input+0x336)=1;
  nativeCursorEmit(input,0,&event);
 } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
 static std::atomic<bool> firstNative{false};if(earlyMenuOwned.load() && !enabled.load() && !firstNative.exchange(true))EarlyMenuTrace("first early native cursor event");
 static uint64_t report=0;auto now=GetTickCount64();if(now-report>=2000){report=now;char text[180];std::snprintf(text,sizeof(text),"MENU_NATIVE_CURSOR uv=%.3f,%.3f position=%.1f,%.1f viewport=%.0f,%.0f",u,v,event.x,event.y,rect[1]-rect[0],rect[2]-rect[3]);Log(SPF_LOG_INFO,text);}return true;
}
std::atomic<bool> earlyMenuGame{false};
thread_local bool nativeMenuGame=false;
extern "C" void CaptureMenuMode(uint32_t inGame) noexcept {nativeMenuGame=(inGame&255)!=0;earlyMenuGame.store(nativeMenuGame);}
extern "C" void CaptureMenuSurface(uintptr_t frame) noexcept {
 float transform[8]{},size[2]{},eye[3]{};if(!SafeRead(frame-0x28,transform,sizeof(transform)) || !SafeRead(frame-0x60,size,sizeof(size)) || !SafeRead(frame-0x40,eye,sizeof(eye)))return;RemoveMenuEyeOffset(transform,eye);
 using FloatQuery=float(__fastcall*)(uintptr_t);auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));float radius=reinterpret_cast<FloatQuery>(base+0x1dc850)(base+(nativeMenuGame?0x2d47d10:0x2d47bd0));
 PublishControllerMenuSurface(transform,size,radius);
 if(earlyMenuOwned.load() && !enabled.load() && earlyMenuFocused.load()){float u=0,v=0;if(earlyCursor.Take(u,v,GetTickCount64()))ApplyNativeMenuCursor(u,v);}
 if(earlyMenuOwned.load() && !earlyMenuStopped.load() && !enabled.load()){static std::atomic<bool> firstSurface{false};if(!firstSurface.exchange(true))EarlyMenuTrace("first early menu surface; capture only");}
 static uint64_t lastLog=0;auto now=GetTickCount64();if(now-lastLog>=2000){lastLog=now;char text[256];std::snprintf(text,sizeof(text),"MENU_SURFACE game=%d center=%.3f,%.3f,%.3f q=%.3f,%.3f,%.3f,%.3f size=%.3f,%.3f radius=%.3f",nativeMenuGame?1:0,transform[0],transform[1],transform[2],transform[4],transform[5],transform[6],transform[7],size[0],size[1],radius);Log(SPF_LOG_INFO,text);}

}
extern "C" __declspec(dllexport) void SPF_PollEarlyMenuSupport(int focused) {
 earlyMenuFocused.store(focused!=0);
}
// Called by the OpenXR bridge after instance creation, outside its input lock.
// Own one set of menu hooks for this DLL's process lifetime; SPF later reuses it.
extern "C" __declspec(dllexport) bool SPF_StartEarlyMenuSupport() {
 static std::mutex startMutex;std::lock_guard lock(startMutex);if(earlyMenuOwned.load())return true;
 char hash[65]{};if(!HashExecutable(hash)||std::strcmp(hash,kHash)!=0)return false;
 auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 const unsigned char modeCode[]{0x0f,0x28,0xd8,0x48,0x8d,0x4d,0x8c,0x0f,0x57,0xc9,0xf3,0x0f,0x11,0x9d,0,9,0,0};
 const unsigned char surfaceCode[]{0x45,0x8b,0xc6,0x48,0x8d,0x55,0xd4,0x48,0x8b,0xce,0xe8};
 const unsigned char cursorCode[]{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,0x41,0x54,0x41,0x56};
 unsigned char actual[sizeof(modeCode)]{};if(!SafeRead(base+0x42c826,actual,sizeof(modeCode))||std::memcmp(actual,modeCode,sizeof(modeCode)))return false;
 if(!SafeRead(base+0x42d144,actual,sizeof(surfaceCode))||std::memcmp(actual,surfaceCode,sizeof(surfaceCode)))return false;
 unsigned char cursorActual[sizeof(cursorCode)]{};if(!SafeRead(base+0x269650,cursorActual,sizeof(cursorCode))||std::memcmp(cursorActual,cursorCode,sizeof(cursorCode)))return false;
 auto status=MH_Initialize();if(status!=MH_OK && status!=MH_ERROR_ALREADY_INITIALIZED)return false;
 void* mode=reinterpret_cast<void*>(base+0x42c826);void* surface=reinterpret_cast<void*>(base+0x42d144);
 if(MH_CreateHook(mode,reinterpret_cast<void*>(MenuModeObserveStub),&gMenuModeOriginal)!=MH_OK)return false;
 if(MH_CreateHook(surface,reinterpret_cast<void*>(MenuSurfaceObserveStub),&gMenuSurfaceOriginal)!=MH_OK){MH_RemoveHook(mode);return false;}
 nativeCursorEmit=reinterpret_cast<NativeCursorEmit>(base+0x269650);earlyMenuStopped.store(false);earlyMenuOwned.store(true);
 MH_QueueEnableHook(mode);MH_QueueEnableHook(surface);if(MH_ApplyQueued()!=MH_OK){earlyMenuOwned.store(false);MH_DisableHook(mode);MH_DisableHook(surface);MH_RemoveHook(mode);MH_RemoveHook(surface);nativeCursorEmit=nullptr;return false;}
 std::thread([]{earlyMenuWorker=true;while(!earlyMenuStopped.load() && !enabled.load()){PollControllerMenuMouse(earlyMenuFocused.load() && !earlyMenuGame.load());std::this_thread::sleep_for(std::chrono::milliseconds(8));}earlyMenuWorker=false;}).detach();
 return true;
}
using RadioStationFn=void(__fastcall*)(uintptr_t,unsigned char);using RadioVolumeFn=void(__fastcall*)(uintptr_t,float);RadioStationFn nativeRadioStation=nullptr;RadioVolumeFn nativeRadioVolume=nullptr;using RadioSelectFn=void(__fastcall*)(uintptr_t,uint64_t);RadioSelectFn nativeRadioSelect=nullptr;using RadioPauseFn=void(__fastcall*)(uintptr_t);RadioPauseFn nativeRadioPause=nullptr;RadioPauseFn nativeRadioInitialize=nullptr;
using WiperCountFn=uint64_t(__fastcall*)(uintptr_t);using WiperSetFn=void(__fastcall*)(uintptr_t,uint32_t);WiperCountFn nativeWiperCount=nullptr;WiperSetFn nativeWiperSet=nullptr;
using WindowControlFn=bool(__fastcall*)(uintptr_t,uintptr_t,float,unsigned char,unsigned char,unsigned char);
WindowControlFn originalWindowControl=nullptr;
bool __fastcall ControllerWindowControl(uintptr_t actor,uintptr_t state,float dt,unsigned char open,unsigned char close,unsigned char electricity){
 auto controller=expectedPlayerDriverController.load();uintptr_t player=controller>=0x1df0?controller-0x1df0:0;int side=NativeWindowSide(actor,state,player);DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
 bool allowed=enabled.load() && foreground==GetCurrentProcessId() && core && core->gameworld && !core->gameworld->GW_IsGamePaused();float direction=side>=0?ControllerWindowDirection(static_cast<unsigned>(side)):0;
 return ForwardControllerWindow(originalWindowControl,actor,state,dt,open,close,electricity,player,allowed,direction);
}
void Activated(const SPF_Core_API* api) {
 SetIntegratedDisplayObserver(ObserveInfotainmentUpdate);
 ActivateIntegratedAdjustments(api);
 core=api;
 if(!core || !core->hooks) {Log(SPF_LOG_ERROR,"DIAG: hooks API unavailable.");return;}
 char hash[65]{};
 if(!HashExecutable(hash) || std::strcmp(hash,kHash)!=0) {
  Log(SPF_LOG_ERROR,"DIAG: executable build mismatch; no hooks registered.");return;
 }
 const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 auto hooks=core->hooks;
 if(hooks->Hook_FindPattern(kDriverPattern)!=base+0x9e2400 ||
    hooks->Hook_FindPattern(kAdvancePattern)!=base+0x15e09e0) {
  Log(SPF_LOG_ERROR,"DIAG: expected unique entry signatures absent; no hooks registered.");return;
 }
 if(hooks->Hook_FindPattern(kEngineRequestPattern)!=base+0x5e4c80){Log(SPF_LOG_ERROR,"ENGINE_REQUEST: signature mismatch; diagnostic disabled.");return;}
 if(hooks->Hook_FindPattern(kBonePattern)!=base+0x15e1700) {Log(SPF_LOG_ERROR,"HORN: bone entry signature mismatch; diagnostic disabled.");return;}
 if(hooks->Hook_FindPattern(kSignalLeftPattern)!=base+0x9f8da2 || hooks->Hook_FindPattern(kSignalRightPattern)!=base+0x9f8e30){Log(SPF_LOG_ERROR,"SIGNAL_DELAY: input-site signature mismatch; no hooks registered.");return;}
 if(hooks->Hook_FindPattern(kHeadPattern)==base+0x16348ee)headHook=hooks->Hook_Register(kName,"HeadTrackingObserver","Observe local-space OpenXR head views",reinterpret_cast<void*>(HeadObserveStub),&gHeadOriginal,kHeadPattern,true);
 else Log(SPF_LOG_WARN,"HEAD_TRACKING: view-site signature mismatch; observer disabled.");
 if(hooks->Hook_FindPattern(kNativeRecenterPattern)==base+0x428852)nativeRecenterHook=hooks->Hook_Register(kName,"NativeHeadRecenter","Observe ATS head-reset input without consuming it",reinterpret_cast<void*>(NativeRecenterObserveStub),&gRecenterOriginal,kNativeRecenterPattern,true);
 else Log(SPF_LOG_WARN,"HEAD_RECENTER native input-site signature mismatch; observer disabled.");
 constexpr char radialDialogPattern[]="48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 83 EC 20 48 8B D9 4D 8B F1 41 8B C8 41 8B E8 48 8B FA 83 E9 01 0F 84 01 01 00 00 83 E9 01 0F";
 SPF_Hook_Handle* radialDialogHook=nullptr;
 if(hooks->Hook_FindPattern(radialDialogPattern)==base+0xfc6ed0)radialDialogHook=hooks->Hook_Register(kName,"RadialMapDialogAction","Route native navmap request through open map dialog",reinterpret_cast<void*>(NativeRadialDialogAction),reinterpret_cast<void**>(&originalRadialDialogAction),radialDialogPattern,true);
 constexpr char radialMapPattern[]="48 8B 8E C8 01 00 00 48 85 C9 74 1B 83 B9 F8 00 00 00 02 75 12";
 constexpr char radialQuickPattern[]="48 8B 8B 88 02 00 00 48 85 C9 0F 84 C7 00 00 00 83 B9 F8 00 00 00 02";
 constexpr char radialServicesPattern[]="48 8B 8E 78 02 00 00 48 85 C9 74 4B 83 B9 F8 00 00 00 02 75 42";
 if(hooks->Hook_FindPattern(radialMapPattern)==base+0x68b1a7 && hooks->Hook_FindPattern(radialQuickPattern)==base+0x68a69f && hooks->Hook_FindPattern(radialServicesPattern)==base+0x68b2fe){
 auto map=hooks->Hook_Register(kName,"RadialMapAction","Merge radial request with native navmap binding",reinterpret_cast<void*>(RadialMapObserveStub),&gRadialMapOriginal,radialMapPattern,true);
 auto quick=hooks->Hook_Register(kName,"RadialQuickInfoAction","Merge radial request with native advmouse binding",reinterpret_cast<void*>(RadialQuickObserveStub),&gRadialQuickOriginal,radialQuickPattern,true);
 auto services=hooks->Hook_Register(kName,"RadialServicesAction","Merge radial request with native services binding",reinterpret_cast<void*>(RadialServicesObserveStub),&gRadialServicesOriginal,radialServicesPattern,true);
 constexpr char radialHudPattern[]="48 8B 8E B8 01 00 00 48 85 C9 74 4D 83 B9 F8 00 00 00 02 75 44 E8 05 85 B6 FF 48 8D";
 constexpr char radialCameraPattern[]="48 8B 89 A0 EB 01 00 49 8B DA 48 85 C9 74 12 83 B9 F8 00 00 00 02 75 09 E8 EF 97 C0 FF";
 constexpr char radialActivatePattern[]="48 8B BB 98 09 00 00 48 8B CF E8 68 0C E5 FF 80 BB F4 08 00 00 00 0F 85 65 01 00 00 48";
 SPF_Hook_Handle *hud=nullptr,*camera=nullptr;
 if(hooks->Hook_FindPattern(radialHudPattern)==base+0x68ada1)hud=hooks->Hook_Register(kName,"RadialHudAction","Merge native showhud binding",reinterpret_cast<void*>(RadialHudObserveStub),&gRadialHudOriginal,radialHudPattern,true);
 if(hooks->Hook_FindPattern(radialCameraPattern)==base+0x5e9ab4)camera=hooks->Hook_Register(kName,"RadialCameraActions","Merge native camcycle and cam1 bindings",reinterpret_cast<void*>(RadialCameraObserveStub),&gRadialCameraOriginal,radialCameraPattern,true);

 radialActionsReady=map&&quick&&services&&radialDialogHook&&hud&&camera;
 }else Log(SPF_LOG_ERROR,"RADIAL_ACTION native binding signatures mismatch; actions unavailable.");
 constexpr char mapWidgetPattern[]="48 89 5C 24 10 55 57 41 56 48 8D 6C 24 B9 48 81 EC 00 01 00 00";
 if(hooks->Hook_FindPattern(mapWidgetPattern)==base+0x1012ac0)hooks->Hook_Register(kName,"MapWidgetProbe","Observe map widget updates without changing input",reinterpret_cast<void*>(MapWidgetObserveStub),&gMapWidgetOriginal,mapWidgetPattern,true);
 constexpr char menuModePattern[]="0F 28 D8 48 8D 4D 8C 0F 57 C9 F3 0F 11 9D 00 09 00 00";
 constexpr char menuSurfacePattern[]="45 8B C6 48 8D 55 D4 48 8B CE E8 ?? ?? ?? ?? 48 8D 8D A0 03 00 00";
 if(earlyMenuOwned.load())Log(SPF_LOG_INFO,"MENU_EARLY reusing early-owned menu hooks; framework polling takes over on activation.");
 else if(hooks->Hook_FindPattern(menuModePattern)==base+0x42c826 && hooks->Hook_FindPattern(menuSurfacePattern)==base+0x42d144){hooks->Hook_Register(kName,"MenuMode","Observe VR menu context",reinterpret_cast<void*>(MenuModeObserveStub),&gMenuModeOriginal,menuModePattern,true);hooks->Hook_Register(kName,"MenuSurface","Observe final VR menu surface with eye translation removed",reinterpret_cast<void*>(MenuSurfaceObserveStub),&gMenuSurfaceOriginal,menuSurfacePattern,true);}else Log(SPF_LOG_WARN,"MENU_LASER surface signatures unavailable; menu laser input disabled.");
 constexpr char cursorEmitPattern[]="48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 41 54 41 56 48 83 EC 70 48 8D B1 98 00 00 00";
 if(hooks->Hook_FindPattern(cursorEmitPattern)==base+0x269650)nativeCursorEmit=reinterpret_cast<NativeCursorEmit>(base+0x269650);else Log(SPF_LOG_WARN,"MENU_NATIVE_CURSOR signature unavailable; relative input retained.");
 expectedReturn=base+0x9e2552;
 resetAnimation=reinterpret_cast<ResetAnimation>(base+0x15e0130);
 enableAnimation=reinterpret_cast<EnableAnimation>(base+0x15e07d0);
 durationQuery=reinterpret_cast<DurationQuery>(base+0x359e70);
 frameQuery=reinterpret_cast<FrameQuery>(base+0x359ee0);
 constexpr char nativeCabinModelRenderPattern[]="48 89 5C 24 08 57 48 83 EC 20 48 8B DA 48 8B F9 E8 3B 04 00 00 45 33 C0 48 8B D3 48 8B CF";
 if(hooks->Hook_FindPattern(nativeCabinModelRenderPattern)==base+0x15e3f80){
  nativeDriverRenderReady=hooks->Hook_Register(kName,"BorrowedCabinBody","Draw engine-owned player body in the interior model view",reinterpret_cast<void*>(RenderCabinModelWithBody),reinterpret_cast<void**>(&originalCabinModelRender),nativeCabinModelRenderPattern,true)!=nullptr;
  Log(SPF_LOG_INFO,nativeDriverRenderReady?"NATIVE_BODY interior-anchored engine-owned renderer registered; no model creation":"NATIVE_BODY renderer registration failed; accessory fallback retained");
 }
 driverHook=hooks->Hook_Register(kName,"DriverUpdate","Observe driver update",reinterpret_cast<void*>(DriverObserveStub),&gDriverOriginal,kDriverPattern,true);
 advanceHook=hooks->Hook_Register(kName,"DriverSample","Observe animation advancement",reinterpret_cast<void*>(AdvanceObserveStub),&gAdvanceOriginal,kAdvancePattern,true);
 boneHook=hooks->Hook_Register(kName,"BonePose","Observe private bone pose before hierarchy assembly",reinterpret_cast<void*>(BoneObserveStub),&gBoneOriginal,kBonePattern,true);
 engineRequestHook=hooks->Hook_Register(kName,"EngineRequest","Observe player engine start/stop request",reinterpret_cast<void*>(EngineRequestObserveStub),&gEngineOriginal,kEngineRequestPattern,true);
 constexpr char radioPausePattern[]="8B 91 F0 00 00 00 8B C2 83 E0 01 F6 C2 01 74 0E 85 C0 74 2D BA 01 00 00 00";
 if(hooks->Hook_FindPattern(radioPausePattern)==base+0x9aee30)nativeRadioPause=reinterpret_cast<RadioPauseFn>(base+0x9aee30);else Log(SPF_LOG_ERROR,"RADIO_BOX pause signature mismatch.");
 constexpr char radioInitializePattern[]="40 55 53 56 57 41 54 48 8D 6C 24 C0 48 81 EC 40 01 00 00 48 8B F1 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 F8 02";
 if(hooks->Hook_FindPattern(radioInitializePattern)==base+0x59df20)nativeRadioInitialize=reinterpret_cast<RadioPauseFn>(base+0x59df20);else Log(SPF_LOG_ERROR,"RADIO_BOX station initializer signature mismatch.");
 constexpr char radioSelectPattern[]="48 89 5C 24 08 57 48 83 EC 40 48 8B 01 48 8B FA 48 8B D9 FF 50 58 48 3B 7B 18";
 if(hooks->Hook_FindPattern(radioSelectPattern)==base+0x59dc50)nativeRadioSelect=reinterpret_cast<RadioSelectFn>(base+0x59dc50);else Log(SPF_LOG_ERROR,"RADIO_BOX station selector signature mismatch.");
 constexpr char radioInputPattern[]="48 8B 8E 08 02 00 00 48 85 C9 74 28 83 B9 F8 00 00 00 02 75 1F";
 constexpr char radioStationPattern[]="48 89 5C 24 08 48 89 74 24 10 57 48 81 EC A0 00 00 00 48 8B 1D ?? ?? ?? ?? 0F B6 F2 48 8B F9";
 constexpr char radioVolumePattern[]="48 8B C4 53 48 81 EC 30 01 00 00 0F 29 70 E8 48 8B D9 48 8D 0D ?? ?? ?? ?? 0F 29 78 D8 0F 28 F1";
 if(hooks->Hook_FindPattern(radioInputPattern)==base+0x68affe && hooks->Hook_FindPattern(radioStationPattern)==base+0x68ee00 && hooks->Hook_FindPattern(radioVolumePattern)==base+0x68ef10){nativeRadioStation=reinterpret_cast<RadioStationFn>(base+0x68ee00);nativeRadioVolume=reinterpret_cast<RadioVolumeFn>(base+0x68ef10);hooks->Hook_Register(kName,"ControllerRadio","Apply controller radio station and volume input",reinterpret_cast<void*>(RadioInputObserveStub),&gRadioInputOriginal,radioInputPattern,true);}else Log(SPF_LOG_ERROR,"RADIO_BOX native signatures mismatch; controller radio disabled.");
 constexpr char wiperCountPattern[]="48 83 EC 28 48 8B 41 18 48 8D 0D ?? ?? ?? ?? 48 8B 90 F8 01 00 00 48 8B 52 38 E8 ?? ?? ?? ?? 48 85 C0 74 0F 48 8B 80 B8 0F 00 00 48 FF C0";
 constexpr char wiperSetPattern[]="48 89 5C 24 08 57 48 83 EC 20 48 8B 41 18 8B FA 48 8B D9 48 8D 0D ?? ?? ?? ?? 48 8B 90 F8 01 00 00 48 8B 52 38 E8 ?? ?? ?? ?? 48 85 C0 74 0C";
 if(hooks->Hook_FindPattern(wiperCountPattern)==base+0x5ea5c0 && hooks->Hook_FindPattern(wiperSetPattern)==base+0x5ea480){nativeWiperCount=reinterpret_cast<WiperCountFn>(base+0x5ea5c0);nativeWiperSet=reinterpret_cast<WiperSetFn>(base+0x5ea480);}else Log(SPF_LOG_ERROR,"WIPERS_BOX cycle setter signature mismatch.");
 constexpr char wipersPattern[]="84 C9 74 5D 48 8B 47 18 48 8D 0D ?? ?? ?? ?? 8B 9F 30 02 00 00 FF C3";
 if(hooks->Hook_FindPattern(wipersPattern)==base+0x9f834c)hooks->Hook_Register(kName,"ControllerWipers","Combine player wiper cycle with controller trigger",reinterpret_cast<void*>(WipersRequestStub),&gWipersOriginal,wipersPattern,true);else Log(SPF_LOG_ERROR,"WIPERS_BOX input signature mismatch; controller wipers disabled.");
 constexpr char lightsPattern[]="84 C9 0F 84 99 00 00 00 0F B6 87 48 02 00 00 0F B6 8F 49 02 00 00";
 if(hooks->Hook_FindPattern(lightsPattern)==base+0x9f8bbb)hooks->Hook_Register(kName,"ControllerLights","Combine player lights cycle with controller trigger",reinterpret_cast<void*>(LightsRequestStub),&gLightsOriginal,lightsPattern,true);else Log(SPF_LOG_ERROR,"LIGHTS_BOX input signature mismatch; controller lights disabled.");
 constexpr char windowPattern[]="48 83 EC 68 4C 8B 51 20 4D 8B 42 30 49 8D 4A 20 0F 29 7C 24 40 0F 28 FA 49 83 F8 2F";
 if(hooks->Hook_FindPattern(windowPattern)==base+0x9ec8d0)hooks->Hook_Register(kName,"ControllerWindows","Combine player window controls with controller box input",reinterpret_cast<void*>(ControllerWindowControl),reinterpret_cast<void**>(&originalWindowControl),windowPattern,true);else Log(SPF_LOG_ERROR,"WINDOW_BOX native signature mismatch; controller window adjustment disabled.");
 constexpr char hornControlPattern[]="4C 8B DC 49 89 5B 08 49 89 6B 10 49 89 73 18 49 89 7B 20 41 56 48 81 EC 90 00 00 00 44 0F B7 91 10 06 00 00";
 if(hooks->Hook_FindPattern(hornControlPattern)==base+0x9eae60)hooks->Hook_Register(kName,"ControllerHornBox","Combine player horn with controller box contact",reinterpret_cast<void*>(HornControlObserveStub),&gHornControlOriginal,hornControlPattern,true);else Log(SPF_LOG_ERROR,"HORN_BOX control signature mismatch.");
 signalLeftHook=hooks->Hook_Register(kName,"SignalLeftRequest","Delay player left toggle until stalk contact",reinterpret_cast<void*>(SignalLeftRequestStub),&gSignalLeftOriginal,kSignalLeftPattern,true);
 signalRightHook=hooks->Hook_Register(kName,"SignalRightRequest","Delay player right toggle until stalk contact",reinterpret_cast<void*>(SignalRightRequestStub),&gSignalRightOriginal,kSignalRightPattern,true);
 if(!driverHook || !advanceHook || !boneHook || !engineRequestHook || !signalLeftHook || !signalRightHook) {
  Log(SPF_LOG_ERROR,"DIAG: incomplete hook registration; observers remain inactive.");return;
 }
 if(core->telemetry) {
  auto context=core->telemetry->Tel_GetContext(kName);vehicleTelemetry=context;
  if(core->telemetry->Tel_RegisterForTruckConstants)core->telemetry->Tel_RegisterForTruckConstants(context,VehicleConstants,nullptr);
  core->telemetry->Tel_RegisterForControls(context,Controls,nullptr);
  core->telemetry->Tel_RegisterForTruckData(context,TruckSignals,nullptr);
 }
 if(core->keybinds)gestureKeys=core->keybinds->Kbind_GetContext(kName);
 enabled.store(true);
 Log(SPF_LOG_INFO,"PEDALS: hooks requested for private interior model; supports prior steering/pedal clips and independent clutch clip. New brake alignment and clutch visual validation pending.");
}
struct PoseFileWatch {std::filesystem::path path;uint64_t lastPoll=0;bool attempted=false;std::string previous;};
void PollOneGestureFile(const wchar_t* filename,const char* hand,LiveGesturePose& store,PoseFileWatch& watch) {
 auto& path=watch.path;auto& lastPoll=watch.lastPoll;auto& attempted=watch.attempted;
 const auto now=GetTickCount64();if(now-lastPoll<500)return;lastPoll=now;
 if(path.empty()) {
  HMODULE module=nullptr;wchar_t modulePath[32768]{};
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollOneGestureFile),&module) || !GetModuleFileNameW(module,modulePath,32768))return;
  path=std::filesystem::path(modulePath).parent_path()/L"config"/filename;
 }
 std::string content;if(!ReadDriverSetting(path,content))return;
 if(attempted && content==watch.previous)return;attempted=true;watch.previous=content;
 if(content.size()>65536){Log(SPF_LOG_WARN,"POSE_RELOAD rejected: section too large; last valid pose retained.");return;}
 std::string error;char text[256];
 if(store.Replace(content,error))std::snprintf(text,sizeof(text),"POSE_RELOAD hand=%s accepted generation=%llu joints=21",hand,static_cast<unsigned long long>(store.Generation()));
 else std::snprintf(text,sizeof(text),"POSE_RELOAD hand=%s rejected: %s; last valid pose retained.",hand,error.c_str());
 Log(SPF_LOG_INFO,text);
}
void PollBodyVisibilityMode() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t path[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollBodyVisibilityMode),&module) || !GetModuleFileNameW(module,path,32768))return;
 auto filePath=std::filesystem::path(path).parent_path()/L"config"/L"body_visibility.txt";std::error_code ec;auto size=DriverSettingSize(filePath,ec);if(ec || size>64)return;
 auto file=OpenDriverSetting(filePath);std::string value,extra;if(!(file>>value) || (file>>extra))return;
 int mode=value=="interior"?0:value=="replace-exterior"?1:-1;
 if(mode>=0 && mode!=bodyVisibilityMode.load()){bodyVisibilityMode.store(mode);Log(SPF_LOG_INFO,mode==0?"VISIBILITY_MODE interior":"VISIBILITY_MODE replace-exterior");}
}
void PollSignalActivationDelay() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t modulePath[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollSignalActivationDelay),&module) || !GetModuleFileNameW(module,modulePath,32768))return;
 auto path=std::filesystem::path(modulePath).parent_path()/L"config"/L"signal_activation_delay.txt";
 std::error_code ec;auto size=DriverSettingSize(path,ec);if(ec || size>128)return;
 auto file=OpenDriverSetting(path,std::ios::binary);if(!file)return;std::string text(static_cast<size_t>(size),'\0');file.read(text.data(),static_cast<std::streamsize>(size));if(!file)return;
 float value=0;if(!ParseSignalDelay(text,value))return;
 if(signalActivationDelay.exchange(value)!=value){char message[128];std::snprintf(message,sizeof(message),"SIGNAL_TIMING_RELOAD accepted delay_seconds=%.3f",value);Log(SPF_LOG_INFO,message);}
}
void PollSignalActuationSeconds() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t modulePath[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollSignalActuationSeconds),&module) || !GetModuleFileNameW(module,modulePath,32768))return;
 auto path=std::filesystem::path(modulePath).parent_path()/L"config"/L"signal_actuation_seconds.txt";
 std::error_code ec;auto size=DriverSettingSize(path,ec);if(ec || size>128)return;
 auto file=OpenDriverSetting(path,std::ios::binary);if(!file)return;std::string text(static_cast<size_t>(size),'\0');file.read(text.data(),static_cast<std::streamsize>(size));if(!file)return;
 float value=0;if(!ParseSignalActuation(text,value))return;
 if(signalActuationSeconds.exchange(value)!=value){char message[128];std::snprintf(message,sizeof(message),"SIGNAL_TIMING_RELOAD accepted actuation_seconds=%.3f",value);Log(SPF_LOG_INFO,message);}
}
void PollLeftElbowInset() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t modulePath[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollLeftElbowInset),&module) || !GetModuleFileNameW(module,modulePath,32768))return;
 auto path=std::filesystem::path(modulePath).parent_path()/L"config"/L"left_elbow_inset.txt";
 std::error_code ec;auto size=DriverSettingSize(path,ec);if(ec || size>128)return;
 auto file=OpenDriverSetting(path);float value;std::string extra;
 if(!(file>>value) || (file>>extra) || !std::isfinite(value) || value<0 || value>.15f)return;
 if(value!=leftElbowInset.load()){leftElbowInset.store(value);char text[128];std::snprintf(text,sizeof(text),"ELBOW_RELOAD accepted inset_m=%.4f",value);Log(SPF_LOG_INFO,text);}
}
void PollRightElbowInset() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t modulePath[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollRightElbowInset),&module) || !GetModuleFileNameW(module,modulePath,32768))return;
 auto path=std::filesystem::path(modulePath).parent_path()/L"config"/L"right_elbow_inset.txt";
 std::error_code ec;auto size=DriverSettingSize(path,ec);if(ec || size>128)return;
 auto file=OpenDriverSetting(path);float value;std::string extra;
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
 auto path=std::filesystem::path(name).parent_path()/L"config"/L"head_tracking_limits.txt";std::error_code ec;auto size=DriverSettingSize(path,ec);if(ec || size>256)return;
 auto file=OpenDriverSetting(path,std::ios::binary);if(!file)return;std::string text(static_cast<size_t>(size),'\0');file.read(text.data(),static_cast<std::streamsize>(size));if(!file)return;
 HeadTrackingLimits limits;if(!ParseHeadTrackingLimits(text,limits))return;
 const bool changed=limits.forward!=headForwardLimit.load() || limits.degrees!=headLeanLimitDegrees.load() || limits.lateral!=headLateralLimit.load();headForwardLimit.store(limits.forward);headLeanLimitDegrees.store(limits.degrees);headLateralLimit.store(limits.lateral);
 if(changed){char message[160];std::snprintf(message,sizeof(message),"HEAD_LIMITS_RELOAD forward_m=%.3f max_lean_degrees=%.1f lateral_m=%.3f",limits.forward,limits.degrees,limits.lateral);Log(SPF_LOG_INFO,message);}
}
void PollInteriorHeadBackshift() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t name[32768]{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollInteriorHeadBackshift),&module) || !GetModuleFileNameW(module,name,32768))return;
 auto modePath=std::filesystem::path(name).parent_path()/L"config"/L"interior_head_mode.txt";
 std::error_code modeError;auto modeSize=DriverSettingSize(modePath,modeError);
 if(!modeError && modeSize<=64){auto modeFile=OpenDriverSetting(modePath);std::string mode,extra;if((modeFile>>mode) && !(modeFile>>extra) && (mode=="visible" || mode=="collapse")){bool collapse=mode=="collapse";if(collapseInteriorHead.exchange(collapse)!=collapse)Log(SPF_LOG_INFO,collapse?"INTERIOR_HEAD_MODE collapse":"INTERIOR_HEAD_MODE visible");}}
 auto path=std::filesystem::path(name).parent_path()/L"config"/L"interior_head_backshift.txt";std::error_code ec;auto size=DriverSettingSize(path,ec);if(ec || size>128)return;
 auto file=OpenDriverSetting(path);float value;std::string extra;if(!(file>>value) || (file>>extra) || !std::isfinite(value) )return;
 if(interiorHeadBackshift.exchange(value)!=value){char message[128];std::snprintf(message,sizeof(message),"INTERIOR_HEAD_RELOAD backshift_m=%.4f",value);Log(SPF_LOG_INFO,message);}
}
void PollHeadTrackingGain() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t name[32768]{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollHeadTrackingGain),&module) || !GetModuleFileNameW(module,name,32768))return;
 auto path=std::filesystem::path(name).parent_path()/L"config"/L"head_tracking_gain.txt";std::error_code ec;auto size=DriverSettingSize(path,ec);if(ec || size>128)return;
 auto file=OpenDriverSetting(path);float value;std::string extra;if(!(file>>value) || (file>>extra) || !std::isfinite(value) || value<0 || value>1)return;
 if(headTrackingGain.exchange(value)!=value)Log(SPF_LOG_INFO,"HEAD_TORSO live gain accepted.");
}
void PollGestureHeadFollowGain() {
 static uint64_t last=0;const auto now=GetTickCount64();if(now-last<500)return;last=now;
 HMODULE module=nullptr;wchar_t name[32768]{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PollGestureHeadFollowGain),&module) || !GetModuleFileNameW(module,name,32768))return;
 auto path=std::filesystem::path(name).parent_path()/L"config"/L"gesture_head_follow_gain.txt";std::error_code ec;auto size=DriverSettingSize(path,ec);if(ec || size>128)return;
 auto file=OpenDriverSetting(path);float value;std::string extra;if(!(file>>value) || (file>>extra) || !std::isfinite(value) || value<0 || value>1)return;
 if(gestureHeadFollowGain.exchange(value)!=value)Log(SPF_LOG_INFO,"GESTURE_HEAD_FOLLOW live gain accepted.");
}
HeadTorsoDrive SnapshotHeadDrive(){std::lock_guard lock(headDriveMutex);return headDrive;}
void UpdateHeadTorso() {
 static uint64_t previous=0;const auto now=GetTickCount64();float dt=previous?static_cast<float>(now-previous)/1000.f:0;previous=now;
 ObservedHead pose;{std::lock_guard lock(headMutex);pose=latestHead;}
 const auto controls=telemetryTick.load(),body=privateBodyTick.load();
 const bool interior=bodyCameraValid.load() && bodyCameraType.load()==2;
 const bool cameraKnown=bodyCameraValid.load() && bodyCameraType.load()>=0 && bodyCameraType.load()<=9;
 const bool fresh=cameraKnown && pose.tick && now-pose.tick<=250 && (pose.flags&15)==15 && controls && now-controls<=250 && body && now-body<=250;
 const bool headFresh=pose.tick&&now>=pose.tick&&now-pose.tick<=250&&(pose.flags&15)==15;
 recenterFeedbackGate.Observe(nativeRecenterSequence.load(),nativeRecenterTick.load());
 if(recenterFeedbackGate.Consume(now,pose.tick,headFresh)){PlayVRRecenterFeedback();Log(SPF_LOG_INFO,"HEAD_RECENTER spoken confirmation from fresh headset pose, independent of cabin.");}
 nativeRecenterGate.Observe(nativeRecenterSequence.load(),nativeRecenterTick.load());
 bool recenter=nativeRecenterGate.Consume(now,pose.tick,fresh);
 if(recenter){ResetControllerMarkerFrame();Log(SPF_LOG_INFO,"HEAD_RECENTER native action applied from fresh post-request pose.");}
 if(!cameraKnown){headCalibration.filtered={};}
 else if(headCalibration.Step(pose,worldGeneration.load(),now,fresh,recenter,dt,headTrackingGain.load(),headForwardLimit.load(),headLateralLimit.load(),interior))Log(SPF_LOG_INFO,"HEAD_TORSO seated calibration captured.");
{std::lock_guard lock(headDriveMutex);headDrive=headCalibration.filtered;}
}
void PollObservedHead() {
 static uint64_t last=0,generation=0;static ObservedHead baseline{};
 const auto now=GetTickCount64();if(now-last<500)return;last=now;
 ObservedHead pose;{std::lock_guard lock(headMutex);pose=latestHead;}
 if(!pose.tick || now-pose.tick>250 || !bodyCameraValid.load() || bodyCameraType.load()!=2)return;
 if(generation!=worldGeneration.load() || baseline.space!=pose.space){baseline=pose;generation=worldGeneration.load();Log(SPF_LOG_INFO,"HEAD_TRACKING baseline captured in interior camera; observation only.");}
 char text[384];std::snprintf(text,sizeof(text),"HEAD_TRACKING valid=1 samples=%llu rejected=%llu flags=%llu position_m=%.4f,%.4f,%.4f delta_m=%.4f,%.4f,%.4f quat_xyzw=%.5f,%.5f,%.5f,%.5f",static_cast<unsigned long long>(headAccepted.load()),static_cast<unsigned long long>(headRejected.load()),static_cast<unsigned long long>(pose.flags),pose.position[0],pose.position[1],pose.position[2],pose.position[0]-baseline.position[0],pose.position[1]-baseline.position[1],pose.position[2]-baseline.position[2],pose.orientation[0],pose.orientation[1],pose.orientation[2],pose.orientation[3]);Log(SPF_LOG_INFO,text);
}
void UpdateActivateSemanticInput(){
 if(!load||!load->input||!activateDevice||!activateDeviceReady.load())return;auto now=GetTickCount64();auto requested=radialActionTicks[6].exchange(0);DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);bool active=enabled.load()&&foreground==GetCurrentProcessId();int edge=activatePulse.Step(active,requested&&now>=requested&&now-requested<=500,now);
 if(edge>0){load->input->Virt_PressButton(activateDevice,"activate");Log(SPF_LOG_INFO,"RADIAL_ACTIVATE semantic activate pressed.");}else if(edge<0){load->input->Virt_ReleaseButton(activateDevice,"activate");Log(SPF_LOG_INFO,"RADIAL_ACTIVATE semantic activate released.");}
}
void ObserveInfotainmentUpdate(uintptr_t owner,int before){
 int after=-1;SafeRead(owner+0x15d4,&after,sizeof(after));float timer=0;int left=-1,right=-1;bool valid=SafeRead(owner+0x15e4,&timer,sizeof(timer))&&SafeRead(owner+0x15d8,&left,sizeof(left))&&SafeRead(owner+0x15dc,&right,sizeof(right));auto now=GetTickCount64();infotainmentCameraActive.store(valid&&timer<0&&(left>=0||right>=0));infotainmentStateTick.store(now);
 auto pending=infotainmentCyclePending.load();if(pending&&now>=pending){if(now-pending>500)infotainmentCyclePending.store(0);else if(before!=after){char text[128];std::snprintf(text,sizeof(text),"INFOTAINMENT_CYCLE mode before=%d after=%d camera=%d",before,after,infotainmentCameraActive.load()?1:0);Log(SPF_LOG_INFO,text);infotainmentCyclePending.store(0);if(before>=0&&after>=0&&after<before){parkingCameraRequest.store(now);Log(SPF_LOG_INFO,"INFOTAINMENT_CYCLE native mode wrapped; parking camera queued as extra step.");}}}
}
void UpdateDisplaySemanticInput(){
 if(!load||!load->input||!activateDevice||!activateDeviceReady.load())return;DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);bool active=enabled.load()&&foreground==GetCurrentProcessId();auto now=GetTickCount64();bool requests[4]{ConsumeControllerDisplayRequest(0),false,false,ConsumeControllerDisplayRequest(2)};bool infotainment=ConsumeControllerDisplayRequest(1);auto parking=parkingCameraRequest.exchange(0);requests[2]=parking&&now>=parking&&now-parking<=500;
 if(infotainment){auto state=infotainmentStateTick.load();if(state&&now>=state&&now-state<250&&infotainmentCameraActive.load())requests[2]=true;else{requests[1]=true;infotainmentCyclePending.store(now);}}
 const char* names[]{"display","infotainment","parking_cams","dashmapzoom"};for(unsigned i=0;i<4;++i){int edge=displayPulses[i].Step(active,requests[i],now);if(edge>0){load->input->Virt_PressButton(activateDevice,names[i]);Log(SPF_LOG_INFO,i==0?"DISPLAY_BOX dashboard pressed.":i==1?"DISPLAY_BOX infotainment pressed.":i==2?"DISPLAY_BOX parking cameras pressed.":"DISPLAY_BOX navigation zoom pressed.");}else if(edge<0)load->input->Virt_ReleaseButton(activateDevice,names[i]);}
}

void Update() {
 UpdateDisplaySemanticInput();
 UpdateActivateSemanticInput();
 PollIntegratedAdjustments();
 if(!enabled.load()) {
  const bool paused=core && core->gameworld && core->gameworld->GW_IsGamePaused && core->gameworld->GW_IsGamePaused();
  PollControllerMenuMouse(paused);
  static uint64_t menuLog=0;auto now=GetTickCount64();if(now-menuLog>=2000){menuLog=now;char text[128];std::snprintf(text,sizeof(text),"MENU_MOUSE unsupported-build mode paused=%d native_input_status=%d",paused?1:0,NativeControllerInputStatus());Log(SPF_LOG_INFO,text);}
  return;
 }
 static uint64_t vehiclePoll=0;auto vehicleNow=GetTickCount64();if(vehicleNow-vehiclePoll>=500&&telemetryTick.load()&&vehicleNow-telemetryTick.load()<250&&core&&core->telemetry&&vehicleTelemetry&&core->telemetry->Tel_GetTruckConstants){vehiclePoll=vehicleNow;SPF_TruckConstants constants{};core->telemetry->Tel_GetTruckConstants(vehicleTelemetry,&constants,sizeof(constants));VehicleConstants(&constants,nullptr);}
 PollBodyVisibilityMode();PollInteriorHeadBackshift();
 PollObservedHead();
 if(core && core->camera && core->camera->Cam_GetInteriorSeatPos){float x=0,y=0,z=0;if(core->camera->Cam_GetInteriorSeatPos(&x,&y,&z))SetControllerSceneCamera(x,y,z);}
 static uint64_t markerCameraLog=0;auto cameraStamp=GetTickCount64();if(cameraStamp-markerCameraLog>=1000 && core && core->camera && core->camera->Cam_GetInteriorSeatPos){markerCameraLog=cameraStamp;float x=0,y=0,z=0;if(core->camera->Cam_GetInteriorSeatPos(&x,&y,&z)){char text[128];std::snprintf(text,sizeof(text),"MARKER_CAMERA_FRAME seat_offset=%.4f,%.4f,%.4f",x,y,z);Log(SPF_LOG_INFO,text);}}
 SPF_CameraType cameraType{};const bool cameraValid=core && core->camera && core->camera->Cam_GetCurrentCamera && core->camera->Cam_GetCurrentCamera(&cameraType);
 bodyCameraValid.store(cameraValid);bodyCameraType.store(cameraValid?static_cast<int>(cameraType):-1);hideBody.store(bodyVisibilityMode.load()==0 && HideInteriorBody(cameraValid,static_cast<int>(cameraType)));hideStockDriver.store(!nativeDriverRenderReady && bodyVisibilityMode.load()==1 && HideInteriorBody(cameraValid,static_cast<int>(cameraType)));
 uintptr_t playerController=0,playerModel=0;ReadPlayerExteriorDriver(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),SafeRead,playerController,playerModel);expectedPlayerDriverController.store(playerController);expectedPlayerDriverModel.store(playerModel);
 const bool menuMouse=core && core->gameworld && core->gameworld->GW_IsGamePaused && (core->gameworld->GW_IsGamePaused() || !cameraValid || !telemetryTick.load() || GetTickCount64()-telemetryTick.load()>250);
 PollControllers(!menuMouse && cameraValid && telemetryTick.load() && GetTickCount64()-telemetryTick.load()<=250,worldGeneration.load());
 if(!menuMouse&&cameraValid&&static_cast<int>(cameraType)==2&&core&&core->camera&&core->camera->Cam_SetInteriorSeatPos){DriverIK::V seat;if(ConsumeControllerCameraPosition(seat)){core->camera->Cam_SetInteriorSeatPos(seat[0],seat[1],seat[2]);SetControllerSceneCamera(seat[0],seat[1],seat[2]);}}
 PollControllerMenuMouse(menuMouse);
 RefreshBorrowedDriverPose();
 {static uint64_t report=0,previous=0;auto now=GetTickCount64();if(now-report>=2000){report=now;auto calls=mapWidgetCalls.load();char msg[180];std::snprintf(msg,sizeof(msg),"MENU_MAP_PROBE menu=%d calls=%llu delta=%llu recent=%d widget_vtable_rva=%llx",menuMouse?1:0,calls,calls-previous,mapWidgetTick.load() && now-mapWidgetTick.load()<250?1:0,mapWidgetVtable.load()?mapWidgetVtable.load()-reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)):0);previous=calls;Log(SPF_LOG_INFO,msg);}}
 PollGesturePoseFile();PollGestureHeadFollowGain();PollHeadTrackingGain();PollHeadTrackingLimits();UpdateHeadTorso();PollSignalActivationDelay();PollSignalActuationSeconds();PollLeftElbowInset();PollRightElbowInset();
 PlayerEngineState engineState{};
 const bool engineValid=ReadPlayerEngine(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),SafeRead,engineState);
 if(engineValid){const int value=static_cast<int>(engineState);if(priorEngineSample>=0 && priorEngineSample!=1 && value==1){if(!skipNextEngineStartEvent.exchange(false))engineStartSequence.fetch_add(1);}priorEngineSample=value;}else priorEngineSample=-1;
 engineStarting.store(engineValid && engineState==PlayerEngineState::Starting);engineTick.store(engineValid?GetTickCount64():0);
 PollDeferredEngineStart(engineValid,engineState);
 if(ConsumeControllerIgnitionRequest() && engineValid && (engineState==PlayerEngineState::Off || engineState==PlayerEngineState::Running)){
  auto controller=expectedPlayerDriverController.load();auto fresh=telemetryTick.load();auto entry=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))+0x5e1c70;const unsigned char expectedCode[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0x48};unsigned char actualCode[sizeof(expectedCode)]{};
  if(controller && fresh && GetTickCount64()-fresh<250 && SafeRead(entry,actualCode,sizeof(actualCode)) && !std::memcmp(expectedCode,actualCode,sizeof(actualCode))){
   skipNextEngineStartEvent.store(true);controllerIgnitionCycle=true;using NativeIgnitionCycle=void(*)(uintptr_t);reinterpret_cast<NativeIgnitionCycle>(entry)(controller-0x1df0);controllerIgnitionCycle=false;
   uint32_t result=99;unsigned char electric=0;SafeRead(controller-0x1df0+0x1b8,&result,4);SafeRead(controller-0x1df0+0x1bc,&electric,1);if(result!=1)skipNextEngineStartEvent.store(false);char message[160];std::snprintf(message,sizeof(message),"IGNITION_INTERACTION native ignition cycle state_after=%u electricity=%u",result,electric);Log(SPF_LOG_INFO,message);
  }
 }

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
  ControllerStatus(text,sizeof(text));Log(SPF_LOG_INFO,text);
  std::snprintf(text,sizeof(text),"GESTURE_HEAD_FOLLOW updates=%llu rejected=%llu gain=%.3f",static_cast<unsigned long long>(gestureHeadFollowUpdates.load()),static_cast<unsigned long long>(gestureHeadFollowRejected.load()),gestureHeadFollowGain.load());Log(SPF_LOG_INFO,text);
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
void Unload() { SetIntegratedDisplayObserver(nullptr); if(activateDevice&&activateDeviceReady.load()&&load&&load->input)for(unsigned i=0;i<4;++i){if(displayPulses[i].down)load->input->Virt_ReleaseButton(activateDevice,i==0?"display":i==1?"infotainment":i==2?"parking_cams":"dashmapzoom");displayPulses[i]={};} if(activateDevice&&activateDeviceReady.load()&&load&&load->input&&activatePulse.down)load->input->Virt_ReleaseButton(activateDevice,"activate");activatePulse={};activateDeviceReady.store(false);activateDevice=nullptr;radialActionTicks[6].store(0);CloseIntegratedAdjustments(); earlyMenuStopped.store(true); nativeCursorEmit=nullptr; enabled.store(false); PollControllerMenuMouse(false); Reset(); CloseControllers(); }
}
// Blend only requested private arm rotations before the original hierarchy assembly.
// The current steering pose is freshly computed by the engine on every call.
void UpdateIgnitionInteractionTarget(const DriverIK::Pose& rotations,uintptr_t positionPointer,uint64_t generation) {
  {
   static uint64_t keyGeneration=UINT64_MAX;static DriverIK::V keyPoint{};DriverIK::Positions keyPositions{};
   if(keyGeneration!=generation && SafeRead(positionPointer,keyPositions.data(),sizeof(keyPositions))){auto keyPose=rotations;WindowGestureMotion arrival{1,1,0};BlendWindowGesture(keyPose,arrival,*liveIgnitionPose.Snapshot());auto keyWorld=DriverIK::FK(keyPose,keyPositions);keyPoint=DriverIK::Add(keyWorld.p[43],DriverIK::V{0,0,-.1524f});keyGeneration=generation;}
   if(keyGeneration==generation)SetIgnitionInteractionTarget(keyPoint);
   static uint64_t hornGeneration=UINT64_MAX;static DriverIK::V hornPoint{};
   if(hornGeneration!=generation && SafeRead(positionPointer,keyPositions.data(),sizeof(keyPositions))){auto hornPose=rotations;HornMotion press;press.reach=press.press=1;BlendHornPose(hornPose,press);hornPoint=DriverIK::FK(hornPose,keyPositions).p[7];hornGeneration=generation;}
   if(hornGeneration==generation)SetHornInteractionTarget(hornPoint);
  }
}
extern "C" void CaptureBone(uintptr_t output,uintptr_t bounds,uintptr_t arrays,uintptr_t bones,uint32_t count) noexcept {
 if(!enabled.load() || count!=61 || bounds<0x1e0)return;
 const uintptr_t model=bounds-0x1e0;char path[192]{};
 if(!ReadModelPath(model,path) || !UseAnimatedBody(model,path))return;
 uintptr_t expected=0,pointers[4]{};
 if(!SafeRead(model+0x1c0,&expected,8) || output!=expected || !SafeRead(arrays,pointers,sizeof(pointers)))return;
 privateBodyTick.store(GetTickCount64());
 if(IsBorrowedPlayerBody(model,path)){static std::atomic<uint64_t> frames{0};auto n=++frames;if(n==1||n%3000==0){char text[128];std::snprintf(text,sizeof(text),"NATIVE_BODY native player rig receives hand/pose updates frames=%llu",n);Log(SPF_LOG_INFO,text);}}
 float arm[4]{},hand[4]{},handPosition[3]{};
 if(!SafeRead(pointers[2]+5*16,arm,sizeof(arm)) || !SafeRead(pointers[2]+7*16,hand,sizeof(hand)) || !SafeRead(pointers[3]+7*12,handPosition,sizeof(handPosition)))return;
 for(float value:arm)if(!std::isfinite(value))return;
 for(float value:hand)if(!std::isfinite(value))return;
 static thread_local PedalMotion nativePedals;
 static thread_local float nativeClutch=0;
 static thread_local LeftArmGestures gestures;
 static thread_local SignalMotion signal;static thread_local uint64_t seenSignal=0,seenSignalCancels=0;
 static thread_local WindowGestureMotion rightMotion;
 static thread_local uint64_t observedDelayCancels=0;
 static thread_local IgnitionMotion ignition;static thread_local uint64_t seenEngineStart=0;
 static thread_local uintptr_t priorModel=0;
 static thread_local uint64_t priorGeneration=0,priorTick=0;
 const uint64_t now=GetTickCount64(),generation=worldGeneration.load();
 if(priorModel!=model || priorGeneration!=generation){nativePedals={};nativeClutch=0;gestures={};rightMotion={};ignition={};signal={};seenSignal=signalSequence.load();seenEngineStart=engineStartSequence.load();priorTick=now;priorModel=model;priorGeneration=generation;}
 const float dt=std::clamp(static_cast<float>(now-priorTick)/1000.f,0.f,.05f);priorTick=now;
 const auto inputTick=hornTick.load(),telTick=telemetryTick.load();
 const bool held=hornHeld.load() && !ControllerHornBoxHeld() && inputTick && now-inputTick<=250 && telTick && now-telTick<=250;
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
  const bool exteriorHeadActive=bodyCameraValid.load() && bodyCameraType.load()>=0 && bodyCameraType.load()<=9 && trackedHead.tick && now-trackedHead.tick<=250;
 const bool elbowFresh=telTick && now-telTick<=250;
 const bool elbowActive=leftElbowInset.load()>0 && elbowFresh,rightElbowActive=rightElbowInset.load()>0 && elbowFresh;
 if((leftActive || rightActive || elbowActive || rightElbowActive || headTorsoActive || exteriorHeadActive || elbowFresh) && !hornFaults.load()) {
  std::array<std::array<float,4>,61> rotations{};
  if(!SafeRead(pointers[2],rotations.data(),sizeof(rotations)))return;
  for(const auto& q:rotations){float norm=0;for(float v:q){if(!std::isfinite(v))return;norm+=v*v;}if(norm<.98f || norm>1.02f)return;}
  const bool nativePedalApplied=IsBorrowedPlayerBody(model,path);
  if(nativePedalApplied){
   const bool fresh=telTick && now>=telTick && now-telTick<=250;
   const float gas=fresh?PedalValue(userThrottle.load()):0,brake=fresh?PedalValue(userBrake.load()):0,clutch=fresh?PedalValue(userClutch.load()):0;
   nativePedals.Step(gas,brake,dt);nativeClutch=Approach(nativeClutch,clutch,dt/.1f);
   ApplyNativePedals(rotations,nativePedals,nativeClutch);
   lastThrottle.store(gas);lastBrake.store(brake);lastClutch.store(clutch);lastClutchPress.store(nativeClutch);lastTransfer.store(nativePedals.transfer);lastAnklePress.store(nativePedals.press);
  }
  unsigned gripMask=0,footMask=0;DriverIK::Positions gripPositions{};if(SafeRead(pointers[3],gripPositions.data(),sizeof(gripPositions))){auto vehiclePose=SnapshotVehicleSteeringPose();gripMask=ApplyVehicleSteeringGrips(rotations,gripPositions,vehiclePose);footMask=ApplyVehicleFootOffsets(rotations,gripPositions,vehiclePose);}
  BlendHornPose(rotations,gestures.horn);
  BlendLiveWindowGesture(rotations,gestures.window);
  if(signal.reach>0)BlendLiveSignal(rotations,signal);
  BlendLiveRightGesture(rotations,rightMotion);
  if(exteriorHeadActive && (gestures.window.reach>0 || rightMotion.reach>0)){
   DriverIK::Positions positions{};std::array<std::array<float,3>,61> scales{};bool valid=SafeRead(pointers[1],scales.data(),sizeof(scales));
   if(valid)for(const auto& scale:scales)for(float x:scale)if(!std::isfinite(x) || std::abs(x-1.f)>.001f)valid=false;
   valid=valid && SafeRead(pointers[3],positions.data(),sizeof(positions));
   if(valid){
    const float yaw=GestureLookYaw(trackedHead.headRotation),pitch=GestureLookPitch(trackedHead.headRotation);bool accepted=true;
    if(gestures.window.reach>0)accepted=FollowGestureHead(rotations,positions,false,gestures.window.reach,yaw,gestureHeadFollowGain.load(),pitch) && accepted;
    if(rightMotion.reach>0)accepted=FollowGestureHead(rotations,positions,true,rightMotion.reach,yaw,gestureHeadFollowGain.load(),pitch) && accepted;
    if(accepted)gestureHeadFollowUpdates.fetch_add(1);else gestureHeadFollowRejected.fetch_add(1);
   }else gestureHeadFollowRejected.fetch_add(1);
  }
  UpdateIgnitionInteractionTarget(rotations,pointers[3],generation);
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
  unsigned controllerMask=0;
  if(elbowFresh && bodyCameraValid.load()){
   DriverIK::Positions positions{};std::array<std::array<float,3>,61> scales{};bool valid=SafeRead(pointers[1],scales.data(),sizeof(scales));
   if(valid)for(const auto& scale:scales)for(float x:scale)if(!std::isfinite(x) || std::abs(x-1.f)>.001f)valid=false;
   if(valid && SafeRead(pointers[3],positions.data(),sizeof(positions)))controllerMask=ApplyControllers(rotations,positions,!leftActive,!rightActive);
  }
  __try {
   if(nativePedalApplied){for(unsigned bone:NativePedalBones)std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);throttleOverrides.fetch_add(1);brakeOverrides.fetch_add(1);clutchOverrides.fetch_add(1);}
   if((footMask&1)||(controllerMask&4))for(unsigned bone:{48u,49u,51u})std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);
   if((footMask&2)||(controllerMask&8))for(unsigned bone:{54u,55u,57u})std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);
   if(gripMask&1)for(unsigned bone:{5u,6u,7u})std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);
   if(gripMask&2)for(unsigned bone:{26u,27u,28u})std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);
   if(controllerMask&1){for(unsigned bone:{5u,6u,7u})std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);for(unsigned bone=8;bone<=22;++bone)std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);}
   if(controllerMask&2){for(unsigned bone:{26u,27u,28u})std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);for(unsigned bone=29;bone<=43;++bone)std::memcpy(reinterpret_cast<void*>(pointers[2]+bone*16),rotations[bone].data(),16);}
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
 if(IsBorrowedPlayerBody(model,path)){DriverIK::Pose auditPose{};DriverIK::Positions auditPositions{};if(SafeRead(pointers[2],auditPose.data(),sizeof(auditPose))&&SafeRead(pointers[3],auditPositions.data(),sizeof(auditPositions))){auto frame=DriverIK::FK(auditPose,auditPositions);nativeRigAudit={model,{frame.p[7],frame.p[28]}};}}
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
 if(!std::strcmp(path,"/model/spf_controller_markers/driver.pmd")){DriverPalette palette{};ApplyControllerMarkerPalette(palette);static std::atomic<uint64_t> markerFrames{0};auto sample=++markerFrames;if(sample==1 || sample%3000==0)Log(SPF_LOG_INFO,"CONTROLLER_SCENE_MARKER private scene model palette updated.");__try{std::memcpy(reinterpret_cast<void*>(output),palette.data(),sizeof(palette));}__except(EXCEPTION_EXECUTE_HANDLER){visibilityFaults.fetch_add(1);}return;}
 if(!std::strcmp(path,"/model/spf_stock_driver/driver.pmd")){
  static uint64_t logged=0;auto stamp=GetTickCount64();if(stamp-logged>=1000){logged=stamp;DriverPalette observed{};if(SafeRead(output,observed.data(),sizeof(observed))){char text[256];std::snprintf(text,sizeof(text),"MARKER_DRIVER_FRAME root=%.4f,%.4f,%.4f head=%.4f,%.4f,%.4f wrist_left=%.4f,%.4f,%.4f",observed[0][3],observed[0][7],observed[0][11],observed[47][3],observed[47][7],observed[47][11],observed[7][3],observed[7][7],observed[7][11]);Log(SPF_LOG_INFO,text);}}
 }
 const bool borrowed=IsBorrowedPlayerBody(model,path);
 if(borrowed&&nativeRigAudit.model==model){static thread_local uint64_t report=0;auto now=GetTickCount64();if(now-report>=1000){report=now;DriverPalette actual{};if(SafeRead(output,actual.data(),sizeof(actual))){auto left=DriverIK::V{actual[7][3],actual[7][7],actual[7][11]},right=DriverIK::V{actual[28][3],actual[28][7],actual[28][11]};char text[192];std::snprintf(text,sizeof(text),"NATIVE_RIG_MAPPING fk_to_palette_error_m=%.6f,%.6f",DriverIK::Length(DriverIK::Sub(left,nativeRigAudit.wrists[0])),DriverIK::Length(DriverIK::Sub(right,nativeRigAudit.wrists[1])));Log(SPF_LOG_INFO,text);}}}

 const bool privateModel=!std::strcmp(path,"/model/spf_stock_driver/driver.pmd");
 const bool duplicateAccessory=privateModel && nativeDriverRenderReady && confirmedPlayerDriverModel.load() && confirmedPlayerDriverModel.load()==expectedPlayerDriverModel.load();
 const bool privateBody=(hideBody.load() && (privateModel||borrowed)) || duplicateAccessory || (borrowed&&!SnapshotVehicleSteeringPose().driverVisible);
 const bool privateHead=(privateModel||borrowed) && !privateBody && (!bodyCameraValid.load() || bodyCameraType.load()==2);
 const auto now=GetTickCount64(),tick=playerDriverTick.load();
 const bool playerStock=hideStockDriver.load() && model==expectedPlayerDriverModel.load() && model==confirmedPlayerDriverModel.load() && tick && now-tick<=750 && !std::strcmp(path,"/asset/character/human/driver/man/driver.pmd");
 if(!privateBody && !playerStock && !privateHead)return;
 DriverPalette palette{};if(!SafeRead(output,palette.data(),sizeof(palette)))return;
 const auto headBefore=palette[47];
 if(privateHead){auto settings=SnapshotVehicleSteeringPose();bool collapse=borrowed?!settings.headVisible:collapseInteriorHead.load();float shift=borrowed?settings.headBackshift:interiorHeadBackshift.load();if(collapse){if(!CollapseDriverHead(palette,shift))return;}else if(!OffsetDriverHead(palette,shift))return;}else if(!CollapseDriverPalette(palette))return;
 if(privateHead){
  static float lastLogged=std::numeric_limits<float>::quiet_NaN();const float offset=interiorHeadBackshift.load();
  if(offset!=lastLogged){lastLogged=offset;char message[192];std::snprintf(message,sizeof(message),"INTERIOR_HEAD_APPLIED collapse=%d offset_m=%.3f palette_delta=%.4f,%.4f,%.4f",collapseInteriorHead.load()?1:0,offset,palette[47][3]-headBefore[3],palette[47][7]-headBefore[7],palette[47][11]-headBefore[11]);Log(SPF_LOG_INFO,message);}
 }
 __try{std::memcpy(reinterpret_cast<void*>(output),palette.data(),sizeof(palette));if(privateBody)hiddenBodyFrames.fetch_add(1);if(playerStock)hiddenStockFrames.fetch_add(1);if(privateHead)hiddenHeadFrames.fetch_add(1);}
 __except(EXCEPTION_EXECUTE_HANDLER){visibilityFaults.fetch_add(1);}
}
bool EnsureRadioList(uintptr_t radio,uint64_t& count,uint64_t& current) {
 return EnsureControllerRadioList(count,[&](){if(!nativeRadioInitialize)return false;nativeRadioInitialize(radio);Log(SPF_LOG_INFO,"RADIO_BOX initialized station list on controller request.");return true;},[&](){return SafeRead(radio+0x18,&count,sizeof(count)) && SafeRead(radio+0xe0,&current,sizeof(current));});
}
extern "C" void CaptureRadioInput(uintptr_t context) noexcept {
 auto command=TakeControllerRadioCommand();if(!context || !enabled.load() || (!command.station && command.volume==0 && !command.toggle) || !nativeRadioStation || !nativeRadioVolume)return;
 DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);auto now=GetTickCount64(),fresh=telemetryTick.load();if(foreground!=GetCurrentProcessId() || !fresh || now-fresh>250 || !core || !core->gameworld || core->gameworld->GW_IsGamePaused())return;
 uintptr_t vtable=0,ready=0;if(!SafeRead(context,&vtable,sizeof(vtable)) || !SafeRead(vtable+0x90,&ready,sizeof(ready)) || !ready)return;
 using Ready=bool(__fastcall*)(uintptr_t);if(!reinterpret_cast<Ready>(ready)(context))return;
 if(command.station){
 auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));uintptr_t manager=0,radio=0,table=0;uint64_t count=0,current=0;
 bool valid=nativeRadioSelect && SafeRead(base+0x36baef8,&manager,sizeof(manager)) && manager && SafeRead(manager+8,&radio,sizeof(radio)) && radio && SafeRead(radio,&table,sizeof(table)) && table==base+0x22c9e08 && SafeRead(radio+0x18,&count,sizeof(count)) && SafeRead(radio+0xe0,&current,sizeof(current));
 if(valid)valid=EnsureRadioList(radio,count,current);
 int next=valid?NextControllerRadioStation(current,count,command.station):-1;
 if(next>=0){nativeRadioSelect(radio,static_cast<uint64_t>(next));uint64_t after=~uint64_t{};SafeRead(radio+0xe0,&after,sizeof(after));if(after==static_cast<uint64_t>(next))QueueControllerRadioTuneSound();char message[192];std::snprintf(message,sizeof(message),"RADIO_BOX select before=%llu requested=%d after=%llu stations=%llu",static_cast<unsigned long long>(current),next,static_cast<unsigned long long>(after),static_cast<unsigned long long>(count));Log(SPF_LOG_INFO,message);}else Log(SPF_LOG_WARN,"RADIO_BOX station list unavailable; selection deferred.");
 }
 if(command.toggle){
 auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));uintptr_t manager=0,radio=0,table=0,stream=0;uint64_t count=0,current=0;
 bool valid=nativeRadioPause && nativeRadioSelect && SafeRead(base+0x36baef8,&manager,sizeof(manager)) && manager && SafeRead(manager+8,&radio,sizeof(radio)) && radio && SafeRead(radio,&table,sizeof(table)) && table==base+0x22c9e08 && SafeRead(radio+0xe8,&stream,sizeof(stream)) && SafeRead(radio+0x18,&count,sizeof(count)) && SafeRead(radio+0xe0,&current,sizeof(current));
 if(valid && !stream)valid=EnsureRadioList(radio,count,current);
 int action=valid?ControllerRadioPlaybackAction(stream!=0,current,count):-1;
 if(action==-2){nativeRadioPause(radio);uint32_t flags=0;if(SafeRead(radio+0xf0,&flags,sizeof(flags)))QueueControllerRadioClickSound((flags&1)==0);Log(SPF_LOG_INFO,"RADIO_BOX play/pause toggled on current station.");}else if(action>=0){nativeRadioSelect(radio,static_cast<uint64_t>(action));uintptr_t started=0;if(SafeRead(radio+0xe8,&started,sizeof(started)) && started)QueueControllerRadioClickSound(true);Log(SPF_LOG_INFO,"RADIO_BOX playback started on selected station.");}else Log(SPF_LOG_WARN,"RADIO_BOX play/pause unavailable until station list loads.");
 }
 if(command.volume!=0)nativeRadioVolume(context,command.volume);
}
namespace {
bool __fastcall NativeRadialDialogAction(uintptr_t object,uintptr_t source,unsigned event,uintptr_t payload){
 auto original=originalRadialDialogAction;if(!original)return false;
 auto request=radialActionTicks[0].load(),now=GetTickCount64();uintptr_t table=0,predicate=0;auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 if(enabled.load()&&request&&now>=request&&now-request<=500&&IsWorldMapMenuActive()&&SafeRead(object,&table,sizeof(table))&&SafeRead(table+0x190,&predicate,sizeof(predicate))&&predicate==base+0xfacb40){
  if(radialActionTicks[0].compare_exchange_strong(request,0)){const uint64_t action[3]={0x7c3ccd92,0,0};Log(SPF_LOG_INFO,"RADIAL_ACTION navmap delivered to open map dialog.");return original(object,source,16,reinterpret_cast<uintptr_t>(action));}
 }
 return original(object,source,event,payload);
}
}
extern "C" bool RequestNativeRadialAction(unsigned action) noexcept {
 if(action>=7||!enabled.load()||(action==6?!activateDeviceReady.load():!radialActionsReady))return false;DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);if(foreground!=GetCurrentProcessId())return false;radialActionTicks[action].store(GetTickCount64());if(action==6)Log(SPF_LOG_INFO,"RADIAL_ACTIVATE request queued.");return true;
}
extern "C" void CaptureNativeRadialAction(uintptr_t owner,unsigned action) noexcept {
 if(action>=7||!enabled.load())return;if(action==0&&IsWorldMapMenuActive())return;auto request=radialActionTicks[action].load();if(!request)return;auto now=GetTickCount64();if(now<request||now-request>500){radialActionTicks[action].store(0);return;}
 constexpr uintptr_t offsets[]={0x1c8,0x288,0x278,0x1b8,0x1eba0,0,0x2e0};uintptr_t binding=0,table=0;unsigned original=0;auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));uintptr_t wrapper=0;
 if(action==5){uint64_t count=0;if(!SafeRead(owner+0x1ebb0,&wrapper,sizeof(wrapper))||!wrapper||!SafeRead(owner+0x1ebb8,&count,sizeof(count))||count<1||count>10)return;}else wrapper=owner+offsets[action]-8;
 if(!SafeRead(wrapper,&table,sizeof(table))||table!=base+0x2201430||!SafeRead(wrapper+8,&binding,sizeof(binding))||!binding||!SafeRead(binding+0xf8,&original,sizeof(original))||original>3){if(action==6){static uint64_t last=0;if(last!=request){last=request;char message[160];std::snprintf(message,sizeof(message),"RADIAL_ACTIVATE binding rejected table_rva=%llx binding_present=%d state=%u",static_cast<unsigned long long>(table-base),binding!=0,original);Log(SPF_LOG_WARN,message);}}return;}
 if(action==6){unsigned char mode=0,latched=0;SafeRead(owner+0x210,&mode,1);SafeRead(owner+0x2fa,&latched,1);char message[128];std::snprintf(message,sizeof(message),"RADIAL_ACTIVATE owner_mode=%u latch=%u",mode,latched);Log(SPF_LOG_INFO,message);}
 auto merged=MergeNativeRadialAction(original,request,now);SIZE_T written=0;if(original==merged||WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(binding+0xf8),&merged,sizeof(merged),&written)&&written==sizeof(merged)){radialActionTicks[action].store(0);char message[128];std::snprintf(message,sizeof(message),"RADIAL_ACTION native binding action=%u before=%u delivered=%u",action,original,merged);Log(SPF_LOG_INFO,message);}
}
extern "C" bool RequestControllerHeadReset() noexcept {
 if(!enabled.load()||!nativeRecenterHook){Log(SPF_LOG_WARN,"HEAD_RECENTER controller request unavailable before native hook activation.");return false;}
 controllerRecenterTick.store(GetTickCount64());Log(SPF_LOG_INFO,"HEAD_RECENTER right B double press queued for ATS native reset.");return true;
}
extern "C" uint32_t CaptureNativeRecenter(uintptr_t inputOwner,uint32_t requested) noexcept {
 if(!enabled.load()||!inputOwner)return requested;
 uintptr_t input=0;if(!SafeRead(inputOwner+0xbd8,&input,sizeof(input))||!input)return requested;
 requested=MergeControllerHeadReset(requested,controllerRecenterTick.exchange(0),GetTickCount64());
 if(requested&255u){nativeRecenterTick.store(GetTickCount64());nativeRecenterSequence.fetch_add(1);Log(SPF_LOG_INFO,"HEAD_RECENTER native ATS head-reset action observed.");}
 return requested;
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
 if(side==3 || side==4){bool pending=side==3?ConsumeControllerLightsRequest():ConsumeControllerWipersRequest();DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);bool allowed=fresh && foreground==GetCurrentProcessId() && core && core->gameworld && !core->gameworld->GW_IsGamePaused();if(pending && allowed){if(side==4){if(edge)return edge;uint32_t current=0;if(!nativeWiperCount || !nativeWiperSet || !SafeRead(vehicle+0x230,&current,sizeof(current)))return edge;auto count=nativeWiperCount(vehicle);if(ApplyControllerWiperCycle(vehicle,current,count,nativeWiperSet)){char message[160];std::snprintf(message,sizeof(message),"WIPERS_BOX cycle current=%u next=%d states=%llu",current,NextControllerWiperState(current,count),static_cast<unsigned long long>(count));Log(SPF_LOG_INFO,message);}return edge;}Log(SPF_LOG_INFO,"LIGHTS_BOX native cycle trigger released.");return 1;}return edge;}

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
 if(controllerIgnitionCycle)return false;
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
 if(!ReadModelPath(model,path) || !UseAnimatedBody(model,path))return;
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
namespace {
void RefreshBorrowedDriverPose() {
 if(!enabled.load() || !nativeDriverRenderReady || !gAdvanceOriginal)return;
 const auto now=GetTickCount64(),tick=telemetryTick.load();
 auto model=expectedPlayerDriverModel.load(),controller=expectedPlayerDriverController.load();
 if(!model||!controller||model!=confirmedPlayerDriverModel.load()||!tick||now<tick||now-tick>250)return;
 uintptr_t current=0;uint32_t index=~0u;
 if(!SafeRead(controller+8,&current,8)||current!=model||!SafeRead(controller+0x10,&index,4)||index>=4096)return;
 char path[192]{};if(!ReadModelPath(model,path)||!IsBorrowedPlayerBody(model,path))return;
 float requested=0;ApplyInteriorSteering(model,index,&requested);
 // No controller/runtime mutex is held across the engine call. Advancing the
 // selected steering sample invalidates ATS's cached rig even at rest, allowing
 // render-time head/hand edits and wrist attachment updates on every frame.
 using Advance=void(__fastcall*)(uintptr_t,uint32_t,float);
 reinterpret_cast<Advance>(gAdvanceOriginal)(model,index,requested);
}
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

extern "C" void ObserveMarkerXRFrame(uintptr_t owner) noexcept {
 static std::atomic<uint64_t> last{0};auto now=GetTickCount64();
 float reference[8]{},current[8]{};if(!SafeRead(owner+0x410,reference,sizeof(reference)) || !SafeRead(owner+0x438,current,sizeof(current)))return;
 for(float v:reference)if(!std::isfinite(v))return;for(float v:current)if(!std::isfinite(v))return;
 SetControllerSceneXRFrame(reference,current);if(now-last.load()<1000)return;last.store(now);
 char text[512];std::snprintf(text,sizeof(text),"MARKER_XR_FRAME neutral_pos=%.4f,%.4f,%.4f neutral_q=%.5f,%.5f,%.5f,%.5f current_pos=%.4f,%.4f,%.4f current_q=%.5f,%.5f,%.5f,%.5f",reference[0],reference[1],reference[2],reference[4],reference[5],reference[6],reference[7],current[0],current[1],current[2],current[4],current[5],current[6],current[7]);Log(SPF_LOG_INFO,text);
}

extern "C" unsigned char CaptureHornBox(uintptr_t object,uint32_t nativeHeld) noexcept {
 if(!enabled.load())return static_cast<unsigned char>(nativeHeld);auto controller=expectedPlayerDriverController.load();uintptr_t horn=0;
 if(controller && SafeRead(controller-0x1df0+0x1d38,&horn,8) && horn==object && ControllerHornBoxHeld())return 1;
 return static_cast<unsigned char>(nativeHeld);
}
