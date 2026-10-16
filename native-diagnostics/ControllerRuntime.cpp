#include "CameraSeatDrag.hpp"
#include "SetupHistory.hpp"
#include <map>
#include "ResetConfirmation.hpp"
#include "SteeringHandDrag.hpp"
#include "ExclusiveEditingModes.hpp"
#include "ControllerStickNudge.hpp"
#include "VehicleBoxProfiles.hpp"
#include "IndependentBoxRender.hpp"
#include "ControllerRadial.hpp"
#include "HandGrab.hpp"
#include "MenuKeyPulse.hpp"
#include "MenuDoublePress.hpp"
#include "MenuButtonRoute.hpp"
#include "DriverSettings.hpp"
#include "IntegratedAdjustments.hpp"
#include "ControllerModelData.hpp"
#include "ControllerModelInput.hpp"
#include <Windows.h>
#include "ControllerRuntime.hpp"
#include "ControllerFingers.hpp"
#include "LiveControllerFingers.hpp"
#include "ControllerFingerMode.hpp"
#include "ControllerSceneFrame.hpp"
#include "IgnitionInteraction.hpp"
#include "InteractionBoxEditor.hpp"
#include "MirrorInteraction.hpp"
#include "NativeControllerInput.hpp"
#include "ControllerMenuMouse.hpp"
#include "MenuLaser.hpp"
extern "C" void ReportControllerSetupGraphics(int state,unsigned boxes) noexcept;
extern "C" void ReportControllerVehicleBoxes(const char* model,const char* file,int status) noexcept;
extern "C" void ReportMenuModel(unsigned side,const char* name,unsigned part,int status) noexcept;
extern "C" void ReportMenuShortcut(int x,int y,int enter,int escape,int key,int edge,unsigned sent) noexcept;
extern "C" bool ApplyNativeMenuCursor(float u,float v) noexcept;
extern "C" void ReportMenuLaser(int owner,int leftHit,int rightHit,float leftTrigger,float rightTrigger,int button,unsigned sent,unsigned requested) noexcept;
#include "vendor/openvr.h"
#include <mutex>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"gdi32.lib")
namespace {
extern "C" bool RequestNativeRadialAction(unsigned action) noexcept;
extern "C" bool RequestControllerHeadReset() noexcept;
ControllerFingerSetupMode fingerSetup;
void PollControllerShortcuts(bool permitted);
void PlaySetupFeedback(bool enabled){HMODULE module=nullptr;wchar_t path[32768]{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PlaySetupFeedback),&module)||!GetModuleFileNameW(module,path,32768))return;auto sound=std::filesystem::path(path).parent_path()/"sounds"/(enabled?"controller_setup_enabled.wav":"controller_setup_disabled.wav");PlaySoundW(sound.c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}
int radioClickPending=0;bool radioTunePending=false;ControllerRadioStick radioStick;ControllerRadioCommand radioCommand;uint64_t radioTick=0;float windowRequests[2]{};uint64_t windowRequestTick=0;
int mirrorContact[2]{-1,-1};uint64_t mirrorContactTick=0;int mirrorBridgeStatus=5;
void SendMirrorInput(int mirror,float x,float y){mirrorBridgeStatus=SetIntegratedAdjustmentInput(MirrorPhysicalSlot(mirror),x,y);}
std::mutex guard;HMODULE library=nullptr;vr::IVRSystem* systemVR=nullptr;bool initialized=false;
using Init=uint32_t (__cdecl*)(vr::EVRInitError*,vr::EVRApplicationType,const char*);
using Interface=void* (__cdecl*)(const char*,vr::EVRInitError*);
using Shutdown=void (__cdecl*)();Shutdown shutdownVR=nullptr;
struct Tracked {DriverIK::V point{};DriverIK::Q rotation{1,0,0,0};bool valid=false;};
LiveControllerFingerPose liveFingers[2]{LiveControllerFingerPose(false),LiveControllerFingerPose(true)};
ControllerFingerMode fingerMode[2];
DriverIK::V hornTarget{},hornBoxOffset{},leftHandBoxOffset{.14f,0,0};uint64_t hornTargetTick=0,hornBoxTick=0;bool hornBoxHeld=false;float hornBoxScale=1.25f;InteractionBoxEditor boxEditor;HandBoxAttachments handBoxes;HandGrabGesture handGrabGestures[2];int handFeedback[2]{};bool suppressGrabTrigger[2]{};MenuDoublePress setupAPress;std::string previousBoxPlacement;uint64_t placementPoll=0;IgnitionTriggerGate displayTriggers[2][2];uint64_t displayRequestTicks[3]{};ControllerUpNudge navigationZoomNudge[2];IgnitionTriggerGate radioTriggers[2];IgnitionTriggerGate wiperTriggers[2];uint64_t wiperRequestTick=0;IgnitionTriggerGate lightTriggers[2];uint64_t lightRequestTick=0;IgnitionTriggerGate ignitionTriggers[2];DriverIK::V ignitionTarget{},ignitionBoxOffset{},handBoxOffset{-.08f,0,0};uint64_t ignitionTargetTick=0,interactionRequestTick=0;ControllerSceneFrame actualSceneFrame{};uint64_t sceneXRFrameTick=0,sceneCameraTick=0;DriverPalette sceneMarkers{};uint64_t sceneMarkerTick=0;bool markerCalibrated=false;DriverIK::V markerOrigin{},markerAnchor{};DriverIK::Q markerBasis{1,0,0,0};
LiveVehicleSteeringPose vehicleSteeringPose;SteeringHandDrag steeringHandDrag;bool steeringPoseDirty=false;CameraSeatDrag cameraSeatDrag;bool cameraPositionPending=false;DriverIK::V pendingCameraPosition{};
extern "C" void ReportControllerVehiclePose(const char*,int);
VehicleBoxIdentity desiredBoxVehicle,activeBoxVehicle;std::filesystem::path activeBoxFile;bool vehicleBoxesReady=false;
bool SaveActiveVehicleBoxes(){if(!activeBoxVehicle.Valid()||activeBoxFile.empty())return false;auto text=SerializeInteractionBoxes(handBoxes.Saved(boxEditor));if(!WriteVehicleBoxes(activeBoxFile,activeBoxVehicle,text))return false;boxEditor.dirty=false;previousBoxPlacement=text;return true;}
vr::IVRRenderModels* controllerModels=nullptr;vr::IVROverlay* markerOverlay=nullptr;vr::VROverlayHandle_t markers[2][6]{};int markerError=0;
void DestroyMenuLasers();void DestroyMenuControllerModels();void HideMenuControllerModels();void UpdateCabinControllerModels(const vr::TrackedDevicePose_t* poses);
bool cabinModelsActive=false,cabinModelReady[2]{};Tracked controllerAim[2];float cabinLaserDistance[2]{2,2};bool cabinLaserHit[2]{},cabinLaserReady[2]{};uint64_t cabinLaserTick[2]{};
#include "ControllerSetupMeshes.inc"
void HideMarkers(){if(markerOverlay)for(auto& side:markers)for(auto handle:side)if(handle)markerOverlay->HideOverlay(handle);}
void CreateMarkers(){
 if(!markerOverlay)return;
 std::array<unsigned char,64*64*4> pixels{};for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x){unsigned n=(y*64+x)*4;pixels[n]=pixels[n+1]=pixels[n+2]=255;pixels[n+3]=(y>=30 && y<=33)?255:0;}
 for(unsigned side=0;side<2;++side)for(unsigned plane=0;plane<6;++plane){auto key=std::string("spf.driver.calibration.")+std::to_string(side)+"."+std::to_string(plane);auto& handle=markers[side][plane];markerError=markerOverlay->CreateOverlay(key.c_str(),"Controller calibration axis",&handle);if(markerError)continue;markerError=markerOverlay->SetOverlayRaw(handle,pixels.data(),64,64,4);markerOverlay->SetOverlayWidthInMeters(handle,.12f);unsigned axis=plane/2;markerOverlay->SetOverlayColor(handle,axis==0?1.f:.1f,axis==1?1.f:.1f,axis==2?1.f:.1f);}
}
void ShowAxes(unsigned side,vr::TrackedDeviceIndex_t device){
 if(!markerOverlay)return;
 for(unsigned plane=0;plane<6;++plane){auto handle=markers[side][plane];if(!handle)continue;unsigned axis=plane/2,variant=plane%2;vr::HmdMatrix34_t m{};
  unsigned tangent=(axis+1+variant)%3;m.m[axis][0]=1;m.m[tangent][1]=1;
  // Complete a right-handed basis so each crossed plane faces consistently.
  float x[3]{},y[3]{};x[axis]=1;y[tangent]=1;m.m[0][2]=x[1]*y[2]-x[2]*y[1];m.m[1][2]=x[2]*y[0]-x[0]*y[2];m.m[2][2]=x[0]*y[1]-x[1]*y[0];m.m[axis][3]=.06f;
  markerError=markerOverlay->SetOverlayTransformTrackedDeviceRelative(handle,device,&m);if(!markerError)markerError=markerOverlay->ShowOverlay(handle);
 }
}
ControllerElbowGuides elbowGuides;float twistShare=.5f,modeBlendSeconds=.18f;std::filesystem::path guideFile,twistFile,modeBlendFile;uint64_t guidePoll=0,lastSolveTick[2]{};
Tracked head,hands[2];ControllerHand controls[2];float weights[2]{},fingerGrip[2]{},fingerTrigger[2]{},fingerThumb[2]{};uint64_t tick=0,generationSeen=0,attempt=0,samples[2]{},solves[2]{},rejects[2]{},calibrations[2]{};int errorCode=0;
std::map<std::string,SetupHistory> setupHistories;bool applyingSetupHistory=false;
SetupHistoryState SnapshotSetupHistory(){return {SerializeInteractionBoxes(handBoxes.Saved(boxEditor)),{vehicleSteeringPose.settings.offset[0],vehicleSteeringPose.settings.offset[1]},vehicleSteeringPose.settings.cameraConfigured?vehicleSteeringPose.settings.cameraSeat:cameraPositionPending?pendingCameraPosition:actualSceneFrame.seatOffset,vehicleSteeringPose.settings.cameraConfigured,vehicleSteeringPose.settings.headVisible,vehicleSteeringPose.settings.driverVisible};}
SetupHistory& CurrentSetupHistory(){return setupHistories[activeBoxVehicle.Filename()];}
void RecordSetupHistory(){if(!applyingSetupHistory&&vehicleBoxesReady&&activeBoxVehicle.Valid()&&vehicleSteeringPose.identity==activeBoxVehicle&&boxEditor.selected<0&&steeringHandDrag.owner<0&&!cameraSeatDrag.active&&!boxEditor.dirty&&!steeringPoseDirty)CurrentSetupHistory().Record(SnapshotSetupHistory());}
bool SaveSteeringPoseEdit(){if(!steeringPoseDirty)return true;if(guideFile.empty()||!vehicleSteeringPose.Save(guideFile.parent_path())){ReportControllerVehiclePose(VehiclePosePath(guideFile.parent_path(),vehicleSteeringPose.identity).string().c_str(),-2);return false;}steeringPoseDirty=false;return true;}
void PollFingerFiles(){
 static uint64_t lastPoll=0;static std::string previous[2];auto now=GetTickCount64();if(guideFile.empty() || now-lastPoll<500)return;lastPoll=now;
 for(unsigned side=0;side<2;++side){auto input=OpenDriverSetting(guideFile.parent_path()/(side?"controller_right_fingers.txt":"controller_left_fingers.txt"),std::ios::binary);if(!input)continue;std::array<char,16385> buffer{};input.read(buffer.data(),buffer.size());std::string text(buffer.data(),static_cast<size_t>(input.gcount()));if(text==previous[side])continue;previous[side]=text;std::string error;liveFingers[side].Replace(text,error);}
}
bool ResetCurrentSetup(unsigned action){
 if(!boxEditor.enabled||!vehicleBoxesReady||action>1)return false;steeringHandDrag.Release();if(!SaveSteeringPoseEdit())return false;
 if(action==0){ResetVehicleBoxLayout(boxEditor);handBoxes.Load(boxEditor);return SaveActiveVehicleBoxes();}
 boxEditor.Release();if(boxEditor.dirty&&!SaveActiveVehicleBoxes())return false;vehicleSteeringPose.settings.offset[0]={};vehicleSteeringPose.settings.offset[1]={};steeringPoseDirty=true;for(unsigned side=0;side<2;++side){controls[side].enabled=false;controls[side].calibrate=false;weights[side]=0;}return SaveSteeringPoseEdit();
}
bool ApplySetupHistory(bool forward){
 if(!boxEditor.enabled||!vehicleBoxesReady)return false;steeringHandDrag.Release();boxEditor.Release();if((boxEditor.dirty&&!SaveActiveVehicleBoxes())||!SaveSteeringPoseEdit())return false;RecordSetupHistory();auto& history=CurrentSetupHistory();auto target=history.Target(forward);if(!target)return false;auto desired=*target;auto before=SnapshotSetupHistory();auto priorEditor=boxEditor;auto priorHandBoxes=handBoxes;InteractionBoxEditor restored;if(!ParseInteractionBoxes(desired.boxes,restored))return false;applyingSetupHistory=true;cameraSeatDrag.Cancel();vehicleSteeringPose.settings.cameraSeat=desired.camera;vehicleSteeringPose.settings.cameraConfigured=desired.cameraConfigured;vehicleSteeringPose.settings.headVisible=desired.headVisible;vehicleSteeringPose.settings.driverVisible=desired.driverVisible;
 for(unsigned side=0;side<2;++side){vehicleSteeringPose.settings.offset[side]=desired.hands[side];controls[side].enabled=false;controls[side].calibrate=false;weights[side]=0;}steeringPoseDirty=true;bool saved=SaveSteeringPoseEdit();if(saved){restored.enabled=true;restored.dirty=true;boxEditor=restored;handBoxes.Load(boxEditor);saved=SaveActiveVehicleBoxes();}
 if(saved){pendingCameraPosition=desired.camera;cameraPositionPending=true;history.Commit(forward);history.current=SnapshotSetupHistory();}else{boxEditor=priorEditor;handBoxes=priorHandBoxes;vehicleSteeringPose.settings.cameraSeat=before.camera;vehicleSteeringPose.settings.cameraConfigured=before.cameraConfigured;vehicleSteeringPose.settings.headVisible=before.headVisible;vehicleSteeringPose.settings.driverVisible=before.driverVisible;for(unsigned side=0;side<2;++side)vehicleSteeringPose.settings.offset[side]=before.hands[side];steeringPoseDirty=true;SaveSteeringPoseEdit();}applyingSetupHistory=false;return saved;
}
bool ToggleSetupVisibility(bool driver){if(!boxEditor.enabled||!vehicleBoxesReady)return false;cameraSeatDrag.Cancel();steeringHandDrag.Release();boxEditor.Release();if((boxEditor.dirty&&!SaveActiveVehicleBoxes())||!SaveSteeringPoseEdit())return false;RecordSetupHistory();auto& value=driver?vehicleSteeringPose.settings.driverVisible:vehicleSteeringPose.settings.headVisible;value=!value;steeringPoseDirty=true;if(!SaveSteeringPoseEdit())return false;RecordSetupHistory();return true;}
Tracked Convert(const vr::TrackedDevicePose_t& pose){
 Tracked out;if(!pose.bDeviceIsConnected || !pose.bPoseIsValid || pose.eTrackingResult!=vr::TrackingResult_Running_OK)return out;
 const auto& m=pose.mDeviceToAbsoluteTracking.m;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)if(!std::isfinite(m[r][c]))return out;
 out.point={m[0][3],m[1][3],m[2][3]};float trace=m[0][0]+m[1][1]+m[2][2];auto& q=out.rotation;
 if(trace>0){float s=std::sqrt(trace+1)*2;q={s/4,(m[2][1]-m[1][2])/s,(m[0][2]-m[2][0])/s,(m[1][0]-m[0][1])/s};}
 else {unsigned i=m[1][1]>m[0][0]?1:0;if(m[2][2]>m[i][i])i=2;unsigned j=(i+1)%3,k=(i+2)%3;float s=std::sqrt(std::max(0.f,1+m[i][i]-m[j][j]-m[k][k]))*2;if(s<1e-6f)return out;q[0]=(m[k][j]-m[j][k])/s;q[i+1]=s/4;q[j+1]=(m[j][i]+m[i][j])/s;q[k+1]=(m[k][i]+m[i][k])/s;}
 float n=0;for(float x:q)n+=x*x;if(n<.98f || n>1.02f)return out;for(float& x:q)x/=std::sqrt(n);out.valid=true;return out;
}
bool ShowControllerPointer(unsigned,const Tracked&,const Tracked&,float,bool);
#include "ResetDialogOverlay.inc"
vr::VROverlayHandle_t radialMenus[2]{};int radialSelection[2]{-2,-2};ControllerRadialConfirm radialConfirm[2];ControllerRadialNote radialNotes[2];
void HideRadialMenus(){if(markerOverlay)for(auto handle:radialMenus)if(handle)markerOverlay->HideOverlay(handle);}
void DrawFingerSetupLabel(std::vector<unsigned char>& pixels,int size,bool enabled,bool dark,int label=0){
 constexpr int width=512,height=320;BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 void* bits=nullptr;auto dc=CreateCompatibleDC(nullptr);if(!dc)return;auto bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(!bitmap){DeleteDC(dc);return;}auto oldBitmap=SelectObject(dc,bitmap);memset(bits,0,width*height*4);auto font=CreateFontW(-58,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");auto oldFont=font?SelectObject(dc,font):nullptr;SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,255,255));const wchar_t* labels[16][4]={{L"FINGER",L"EDITING",L"MODE",enabled?L"ON":L"OFF"},{L"",L"MAP",L"",L""},{L"",L"QUICK",L"INFO",L""},{L"SERVICES",L"AND",L"ADJUSTMENTS",L""},{L"",L"HUD",L"WIDGETS",L""},{L"",L"NEXT",L"CAMERA",L""},{L"",L"INTERIOR",L"CAMERA",L""},{L"",L"ACTIVATE",L"",L""},{enabled?L"EXIT":L"SETUP",enabled?L"SETUP":L"MODE",enabled?L"":L"OFF",L""},{L"",L"VR",L"RECENTER",L""},{L"",L"RESET",L"BOXES",L""},{L"",L"RESET",L"HANDS",L""},{L"",L"UNDO",enabled?L"":L"EMPTY",L""},{L"",L"REDO",enabled?L"":L"EMPTY",L""},{L"",enabled?L"HIDE":L"SHOW",L"HEAD",L""},{L"",enabled?L"HIDE":L"SHOW",L"DRIVER",L""}};auto lines=labels[label];for(int line=0;line<4;++line){RECT rect{0,16+line*72,width,88+line*72};DrawTextW(dc,lines[line],-1,&rect,DT_CENTER|DT_SINGLELINE|DT_NOPREFIX);}
 GdiFlush();auto source=static_cast<const unsigned char*>(bits);const int octants[]={2,0,6,0,2,4,5,1,3,5,6,4,7,1,0,2};float angle=octants[label]*.78539816339f;int startX=size/2+static_cast<int>(std::sin(angle)*size*.30f)-width/2,startY=size/2-static_cast<int>(std::cos(angle)*size*.30f)-height/2;for(int y=0;y<height;++y)for(int x=0;x<width;++x){unsigned alpha=source[(y*width+x)*4];if(!alpha)continue;int px=startX+x,py=startY+y;if(px<0||py<0||px>=size||py>=size)continue;auto n=(py*size+px)*4;unsigned color=dark?15:255;for(int c=0;c<3;++c)pixels[n+c]=static_cast<unsigned char>((pixels[n+c]*(255-alpha)+color*alpha)/255);pixels[n+3]=static_cast<unsigned char>(alpha+pixels[n+3]*(255-alpha)/255);}
 if(font){SelectObject(dc,oldFont);DeleteObject(font);}SelectObject(dc,oldBitmap);DeleteObject(bitmap);DeleteDC(dc);
}
void UpdateRadialMenus(const vr::TrackedDevicePose_t* poses,const float* sticks,const bool* clicks,bool valid){
 using namespace DriverIK;if(!markerOverlay||!systemVR)return;auto h=Convert(poses[0]);
 for(unsigned side=0;side<2;++side){if(boxEditor.enabled&&side==1){radialConfirm[side].Step(false,false,-1,GetTickCount64());radialNotes[side].Step(false,-1);if(radialMenus[side])markerOverlay->HideOverlay(radialMenus[side]);continue;}auto index=systemVR->GetTrackedDeviceIndexForControllerRole(side?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand);auto hand=index<vr::k_unMaxTrackedDeviceCount?Convert(poses[index]):Tracked{};int selection=valid&&h.valid&&hand.valid?ControllerRadialDirection(sticks[side*2],sticks[side*2+1]):-1;if(!ControllerRadialSegmentVisible(side,selection,boxEditor.enabled))selection=-1;if(boxEditor.enabled&&side==0&&((selection==7&&!CurrentSetupHistory().Target(false))||(selection==1&&!CurrentSetupHistory().Target(true))))selection=-1;
 if(radialConfirm[side].Step(valid && resetConfirmation.action<0 && h.valid && hand.valid,clicks[side],selection,GetTickCount64())){ControllerToggleFeedback(side);bool setupSelection=boxEditor.enabled;if(setupSelection&&side==0&&(selection==0||selection==2))ToggleSetupVisibility(selection==2);if(boxEditor.enabled&&side==0&&(selection==7||selection==1))ApplySetupHistory(selection==1);int reset=ControllerRadialResetSelection(side,selection,boxEditor.enabled);if(reset>=0)OpenResetDialog(static_cast<unsigned>(reset));if(!setupSelection&&ControllerRadialRecenterSelection(side,selection))RequestControllerHeadReset();if(ControllerRadialSetupSelection(side,selection)&&vehicleBoxesReady){ToggleExclusiveBoxSetup(boxEditor,fingerSetup);PlaySetupFeedback(boxEditor.enabled);}int action=setupSelection?-1:ControllerRadialAction(side,selection);if(action>=0)RequestNativeRadialAction(static_cast<unsigned>(action));if(!setupSelection&&SelectExclusiveFingerSetup(boxEditor,fingerSetup,side,selection)){HMODULE module=nullptr;wchar_t path[32768]{};if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&UpdateRadialMenus),&module)&&GetModuleFileNameW(module,path,32768))PlaySoundW((std::filesystem::path(path).parent_path()/"sounds"/(fingerSetup.enabled?"controller_finger_editing_enabled.wav":"controller_finger_editing_disabled.wav")).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}}if(radialNotes[side].Step(radialConfirm[side].open,selection)){HMODULE module=nullptr;wchar_t path[32768]{};if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&UpdateRadialMenus),&module)&&GetModuleFileNameW(module,path,32768)){auto name=std::string("controller_radial_")+(side?"right_note_":"left_note_")+std::to_string(selection)+".wav";PlaySoundW((std::filesystem::path(path).parent_path()/"sounds"/name).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}}bool flash=radialConfirm[side].Flash(selection,GetTickCount64());int visual=selection+(flash?8:0)+(side==0 && fingerSetup.enabled?16:0)+(side==0 && boxEditor.enabled?32:0)+(side==0&&boxEditor.enabled&&CurrentSetupHistory().Target(false)?64:0)+(side==0&&boxEditor.enabled&&CurrentSetupHistory().Target(true)?128:0)+(side==0&&boxEditor.enabled&&vehicleSteeringPose.settings.headVisible?256:0)+(side==0&&boxEditor.enabled&&vehicleSteeringPose.settings.driverVisible?512:0);
 auto& handle=radialMenus[side];if(!radialConfirm[side].open){if(handle)markerOverlay->HideOverlay(handle);continue;}
 if(!handle){auto key=std::string("spf.driver.radial.")+std::to_string(side);if(markerOverlay->CreateOverlay(key.c_str(),"Controller eight-way menu",&handle)!=vr::VROverlayError_None)continue;markerOverlay->SetOverlayWidthInMeters(handle,.22f);radialSelection[side]=-2;}
 if(radialSelection[side]!=visual){constexpr int size=2048;constexpr float pi=3.14159265359f,step=pi/4;std::vector<unsigned char> pixels(size*size*4);for(int py=0;py<size;++py)for(int px=0;px<size;++px){float x=px-(size-1)*.5f,y=(size-1)*.5f-py,r=std::hypot(x,y),angle=std::atan2(x,y);int sector=(static_cast<int>(std::floor(angle/step+.5f))+8)%8;if(!ControllerRadialSegmentVisible(side,sector,boxEditor.enabled))continue;float fromCenter=std::abs(std::remainder(angle-sector*step,2*pi));float edge=std::min({r-384.f,976.f-r,(step*.5f-fromCenter-.018f)*r});float coverage=std::clamp(edge+.5f,0.f,1.f);if(coverage<=0)continue;bool selected=sector==selection;unsigned n=(py*size+px)*4;pixels[n]=selected?(flash?255:70):(side==0&&sector==2&&fingerSetup.enabled?70:210);pixels[n+1]=selected?(flash?255:220):210;pixels[n+2]=255;pixels[n+3]=static_cast<unsigned char>((selected?240:125)*coverage);}if(side==0){DrawFingerSetupLabel(pixels,size,boxEditor.enabled,flash && selection==3,8);if(!boxEditor.enabled){DrawFingerSetupLabel(pixels,size,fingerSetup.enabled,flash&&selection==2);DrawFingerSetupLabel(pixels,size,false,flash&&selection==0,1);DrawFingerSetupLabel(pixels,size,false,flash&&selection==5,9);}if(boxEditor.enabled){DrawFingerSetupLabel(pixels,size,false,flash&&selection==6,10);DrawFingerSetupLabel(pixels,size,false,flash&&selection==4,11);DrawFingerSetupLabel(pixels,size,CurrentSetupHistory().Target(false)!=nullptr,flash&&selection==7,12);DrawFingerSetupLabel(pixels,size,CurrentSetupHistory().Target(true)!=nullptr,flash&&selection==1,13);DrawFingerSetupLabel(pixels,size,vehicleSteeringPose.settings.headVisible,flash&&selection==0,14);DrawFingerSetupLabel(pixels,size,vehicleSteeringPose.settings.driverVisible,flash&&selection==2,15);}}else{DrawFingerSetupLabel(pixels,size,false,flash && selection==6,2);DrawFingerSetupLabel(pixels,size,false,flash && selection==0,3);DrawFingerSetupLabel(pixels,size,false,flash && selection==2,4);DrawFingerSetupLabel(pixels,size,false,flash && selection==4,5);DrawFingerSetupLabel(pixels,size,false,flash && selection==5,6);DrawFingerSetupLabel(pixels,size,false,flash && selection==1,7);}markerOverlay->SetOverlayRaw(handle,pixels.data(),size,size,4);radialSelection[side]=visual;}

 auto point=Add(hand.point,Rotate(h.rotation,{0,.07f,0}));auto x=Rotate(h.rotation,{1,0,0}),y=Rotate(h.rotation,{0,1,0}),z=Rotate(h.rotation,{0,0,1});vr::HmdMatrix34_t m{};for(unsigned row=0;row<3;++row){m.m[row][0]=x[row];m.m[row][1]=y[row];m.m[row][2]=z[row];m.m[row][3]=point[row];}markerOverlay->SetOverlayTransformAbsolute(handle,vr::TrackingUniverseStanding,&m);markerOverlay->ShowOverlay(handle);
 }
}
bool Connect(){
 wchar_t path[MAX_PATH]{};HMODULE self=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&Connect),&self))return false;
 GetModuleFileNameW(self,path,MAX_PATH);guideFile=std::filesystem::path(path).parent_path()/"config"/"controller_elbow_guides.txt";twistFile=guideFile.parent_path()/"controller_forearm_twist_share.txt";modeBlendFile=guideFile.parent_path()/"controller_finger_mode_blend_seconds.txt";wchar_t* slash=wcsrchr(path,L'\\');if(!slash)return false;wcscpy_s(slash+1,MAX_PATH-(slash+1-path),L"spf_controller_openvr.dll");
 library=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if(!library){errorCode=static_cast<int>(GetLastError());return false;}
 auto init=reinterpret_cast<Init>(GetProcAddress(library,"VR_InitInternal2"));auto get=reinterpret_cast<Interface>(GetProcAddress(library,"VR_GetGenericInterface"));shutdownVR=reinterpret_cast<Shutdown>(GetProcAddress(library,"VR_ShutdownInternal"));
 vr::EVRInitError error=vr::VRInitError_None;
 if(init && get && shutdownVR){init(&error,vr::VRApplication_Background,nullptr);initialized=error==vr::VRInitError_None;if(initialized)systemVR=static_cast<vr::IVRSystem*>(get(vr::IVRSystem_Version,&error));}
 errorCode=static_cast<int>(error);if(systemVR){controllerModels=static_cast<vr::IVRRenderModels*>(get(vr::IVRRenderModels_Version,&error));markerOverlay=static_cast<vr::IVROverlay*>(get(vr::IVROverlay_Version,&error));CreateMarkers();return true;}
 if(initialized && shutdownVR)shutdownVR();initialized=false;FreeLibrary(library);library=nullptr;return false;
}
}
static uint64_t lastBodyInteractionTick=0;
void UpdateCabinInteractionsLocked(const DriverIK::World&,unsigned,const float*,bool);
void PollControllers(bool active,uint64_t generation){
 PollFingerFiles();
 std::lock_guard lock(guard);cabinModelsActive=active;cabinModelReady[0]=cabinModelReady[1]=false;controllerAim[0]=controllerAim[1]={};uint64_t now=GetTickCount64();float dt=tick?std::min(.05f,(now-tick)*.001f):0;tick=now;
 if(generationSeen!=generation){cameraSeatDrag.Cancel();CancelResetDialog();steeringHandDrag.Release();if(!SaveSteeringPoseEdit())return;lastBodyInteractionTick=0;generationSeen=generation;radioClickPending=0;radioTunePending=false;radioStick={};radioCommand={};radioTick=0;radioTriggers[0]=radioTriggers[1]={};boxEditor.Release();boxEditor.enabled=false;handBoxes.valid=false;handGrabGestures[0]=handGrabGestures[1]={};suppressGrabTrigger[0]=suppressGrabTrigger[1]=false;markerCalibrated=false;ignitionTriggers[0]=ignitionTriggers[1]={};lightTriggers[0]=lightTriggers[1]={};wiperTriggers[0]=wiperTriggers[1]={};wiperRequestTick=0;lightRequestTick=0;interactionRequestTick=ignitionTargetTick=0;for(unsigned i=0;i<2;++i){controls[i]={};fingerMode[i]={};weights[i]=0;fingerGrip[i]=fingerTrigger[i]=fingerThumb[i]=0;lastSolveTick[i]=0;}}
 if(!systemVR && now-attempt>=5000){attempt=now;Connect();}
 if(!guideFile.empty() && now-guidePoll>=500){guidePoll=now;auto input=OpenDriverSetting(guideFile);if(input){std::string text((std::istreambuf_iterator<char>(input)),{});ControllerElbowGuides parsed;if(ParseControllerElbowGuides(text,parsed))elbowGuides=parsed;}auto twistInput=OpenDriverSetting(twistFile);float value;std::string extra;if(twistInput>>value && !(twistInput>>extra) && std::isfinite(value) && value>=0 && value<=1)twistShare=value;auto boxInput=OpenDriverSetting(guideFile.parent_path()/"ignition_box_offset.txt");DriverIK::V boxOffset{};if(boxInput>>boxOffset[0]>>boxOffset[1]>>boxOffset[2] && !(boxInput>>extra) && std::isfinite(boxOffset[0]) && std::isfinite(boxOffset[1]) && std::isfinite(boxOffset[2]))ignitionBoxOffset=boxOffset;auto leftBoxInput=OpenDriverSetting(guideFile.parent_path()/"horn_left_hand_box_offset.txt");DriverIK::V leftOffset{};if(leftBoxInput>>leftOffset[0]>>leftOffset[1]>>leftOffset[2] && !(leftBoxInput>>extra) && std::isfinite(leftOffset[0]) && std::isfinite(leftOffset[1]) && std::isfinite(leftOffset[2]))leftHandBoxOffset=leftOffset;auto hornScaleInput=OpenDriverSetting(guideFile.parent_path()/"horn_box_scale.txt");float scaleValue=0;if(hornScaleInput>>scaleValue && !(hornScaleInput>>extra) && std::isfinite(scaleValue) && scaleValue>0)hornBoxScale=scaleValue;auto hornBoxInput=OpenDriverSetting(guideFile.parent_path()/"horn_box_offset.txt");DriverIK::V hornOffset{};if(hornBoxInput>>hornOffset[0]>>hornOffset[1]>>hornOffset[2] && !(hornBoxInput>>extra) && std::isfinite(hornOffset[0]) && std::isfinite(hornOffset[1]) && std::isfinite(hornOffset[2]))hornBoxOffset=hornOffset;auto handBoxInput=OpenDriverSetting(guideFile.parent_path()/"ignition_hand_box_offset.txt");DriverIK::V handOffset{};if(handBoxInput>>handOffset[0]>>handOffset[1]>>handOffset[2] && !(handBoxInput>>extra) && std::isfinite(handOffset[0]) && std::isfinite(handOffset[1]) && std::isfinite(handOffset[2]))handBoxOffset=handOffset;auto blendInput=OpenDriverSetting(modeBlendFile);if(blendInput>>value && !(blendInput>>extra) && std::isfinite(value) && value>0)modeBlendSeconds=value;}

 if(!guideFile.empty() && (activeBoxVehicle!=desiredBoxVehicle || now-placementPoll>=500)){
  placementPoll=now;
  if(activeBoxVehicle!=desiredBoxVehicle){cameraSeatDrag.Cancel();cameraPositionPending=false;CancelResetDialog();steeringHandDrag.Release();if(!SaveSteeringPoseEdit())return;
   if(activeBoxVehicle.Valid()){if(boxEditor.selected>=0)handBoxes.CaptureEdit(boxEditor);boxEditor.Release();if(boxEditor.dirty&&!SaveActiveVehicleBoxes()){vehicleBoxesReady=false;ReportControllerVehicleBoxes(activeBoxVehicle.model.c_str(),activeBoxFile.string().c_str(),-2);return;}}
   activeBoxVehicle=desiredBoxVehicle;activeBoxFile=VehicleBoxPath(guideFile.parent_path(),activeBoxVehicle);vehicleBoxesReady=false;boxEditor={};handBoxes={};previousBoxPlacement.clear();ignitionTargetTick=hornTargetTick=0;mirrorContactTick=0;radioCommand={};windowRequests[0]=windowRequests[1]=0;hornBoxHeld=false;interactionRequestTick=lightRequestTick=wiperRequestTick=windowRequestTick=radioTick=0;radioStick={};radioClickPending=0;radioTunePending=false;
   for(unsigned side=0;side<2;++side){controls[side]={};weights[side]=0;handGrabGestures[side]={};suppressGrabTrigger[side]=false;ignitionTriggers[side]={};radioTriggers[side]={};lightTriggers[side]={};wiperTriggers[side]={};}
  }
  if(activeBoxVehicle.Valid()&&!vehicleBoxesReady){bool migrated=false;std::string text;if(InitializeVehicleBoxes(guideFile.parent_path(),activeBoxVehicle,boxEditor,text,migrated)){handBoxes.Load(boxEditor);previousBoxPlacement=text;vehicleBoxesReady=true;ReportControllerVehicleBoxes(activeBoxVehicle.model.c_str(),activeBoxFile.string().c_str(),migrated?2:1);}else ReportControllerVehicleBoxes(activeBoxVehicle.model.c_str(),activeBoxFile.string().c_str(),-1);}
  else if(vehicleBoxesReady&&boxEditor.selected<0&&!boxEditor.dirty){auto candidate=boxEditor;std::string text;if(ReadVehicleBoxes(activeBoxFile,activeBoxVehicle,candidate,text)&&text!=previousBoxPlacement){boxEditor=candidate;handBoxes.Load(boxEditor);previousBoxPlacement=text;}}
  if(vehicleBoxesReady&&!steeringPoseDirty&&steeringHandDrag.owner<0&&!cameraSeatDrag.active){auto previous=vehicleSteeringPose.prior;bool valid=vehicleSteeringPose.Poll(guideFile.parent_path(),activeBoxVehicle);if(valid&&previous!=vehicleSteeringPose.prior&&vehicleSteeringPose.settings.cameraConfigured){pendingCameraPosition=vehicleSteeringPose.settings.cameraSeat;cameraPositionPending=true;}static int priorPoseStatus=0;int status=valid?1:-1;if(status!=priorPoseStatus||previous!=vehicleSteeringPose.prior){priorPoseStatus=status;ReportControllerVehiclePose(VehiclePosePath(guideFile.parent_path(),activeBoxVehicle).string().c_str(),status);}}

 }
 RecordSetupHistory();
 if(radioClickPending && !guideFile.empty()){auto sound=radioClickPending>0?"controller_radio_on.wav":"controller_radio_off.wav";radioClickPending=0;PlaySoundW((guideFile.parent_path().parent_path()/"sounds"/sound).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}
 if(radioTunePending && !guideFile.empty()){radioTunePending=false;PlaySoundW((guideFile.parent_path().parent_path()/"sounds"/"controller_radio_tune.wav").c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}
 bool nativePressed[2]{},modePressed[2]{},thumbPressed[2]{};float grips[2]{},triggers[2]{},inward[2]{};bool nativeValid=active && PollNativeControllerButtons(nativePressed,grips,triggers,modePressed,thumbPressed,inward);float sticks[4]{};bool sticksValid=nativeValid && ReadControllerStickAxes(sticks);bool mirrorFresh=active && vehicleBoxesReady && mirrorContactTick && now-mirrorContactTick<150 && !boxEditor.enabled;windowRequests[0]=windowRequests[1]=0;windowRequestTick=now;int radioOwner=-1;int mirrorOwner=-1;for(unsigned side=0;side<2;++side)if(mirrorFresh && mirrorContact[side]>=0){inward[side]=0;if(mirrorContact[side]==9 && sticksValid && (radioOwner<0 || std::hypot(sticks[side*2],sticks[side*2+1])>std::hypot(sticks[radioOwner*2],sticks[radioOwner*2+1])))radioOwner=side;if(mirrorContact[side]>=7)continue;if(mirrorContact[side]>=4 && mirrorContact[side]<=5){if(sticksValid){auto& value=windowRequests[mirrorContact[side]-4];if(std::abs(sticks[side*2+1])>std::abs(value))value=sticks[side*2+1];}continue;}if(sticksValid && (mirrorOwner<0 || std::hypot(sticks[side*2],sticks[side*2+1])>std::hypot(sticks[mirrorOwner*2],sticks[mirrorOwner*2+1])))mirrorOwner=side;}for(unsigned side=0;side<2;++side)if(navigationZoomNudge[side].Step(sticks[side*2+1],mirrorFresh&&sticksValid&&mirrorContact[side]==13&&!radialConfirm[side].open&&!nativePressed[side])){displayRequestTicks[2]=now;ControllerToggleFeedback(side);}
 if(radioOwner<0)radioCommand.station=0;auto radioInput=radioStick.Step(radioOwner<0?0:sticks[radioOwner*2],radioOwner<0?0:sticks[radioOwner*2+1],radioOwner>=0);if(radioInput.station)radioCommand.station=radioInput.station;radioCommand.volume=radioInput.volume;radioTick=now;SendMirrorInput(mirrorOwner<0?-1:mirrorContact[mirrorOwner],mirrorOwner<0?0:sticks[mirrorOwner*2],mirrorOwner<0?0:sticks[mirrorOwner*2+1]);
 if(!active){HideMarkers();HideRadialMenus();}head.valid=false;for(auto& h:hands)h.valid=false;
 if(systemVR && active){vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount]{};systemVR->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding,0,poses,vr::k_unMaxTrackedDeviceCount);auto observedHead=Convert(poses[0]);if(observedHead.valid)head=observedHead;
  for(unsigned i=0;i<2;++i){auto index=systemVR->GetTrackedDeviceIndexForControllerRole(i?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand);
   if(index<vr::k_unMaxTrackedDeviceCount){auto observedHand=Convert(poses[index]);if(observedHand.valid)hands[i]=observedHand;}
   bool valid=hands[i].valid && head.valid;if(handFeedback[i]){ControllerToggleFeedback(i);PlaySoundW((guideFile.parent_path().parent_path()/"sounds"/(handFeedback[i]>0?"controller_hand_engaged.wav":"controller_hand_disengaged.wav")).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);handFeedback[i]=0;}auto fingers=liveFingers[i].Snapshot();auto priorSwitches=fingerMode[i].switches;fingerMode[i].Step(modePressed[i],valid && nativeValid && fingerSetup.enabled,liveFingers[i].Revision(),*fingers);if(fingerMode[i].switches!=priorSwitches){ControllerToggleFeedback(i);auto name=std::string("controller_")+(i?"right_":"left_")+fingerMode[i].SpokenFinger()+".wav";PlaySoundW((guideFile.parent_path().parent_path()/"sounds"/name).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}if(fingerMode[i].ToggleButton(thumbPressed[i],valid && nativeValid && fingerSetup.enabled)){ControllerToggleFeedback(i);PlaySoundW((guideFile.parent_path().parent_path()/"sounds"/(fingerMode[i].Selected()?"controller_finger_added.wav":"controller_finger_removed.wav")).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}if(valid)++samples[i];
   if(markerOverlay)for(auto handle:markers[i])if(handle)markerOverlay->HideOverlay(handle);

  }
  UpdateCabinControllerModels(poses);UpdateRadialMenus(poses,sticks,nativePressed,nativeValid && sticksValid);
 }
 for(unsigned i=0;i<2;++i){auto currentFingers=liveFingers[i].Snapshot();fingerMode[i].Advance(*currentFingers,dt,modeBlendSeconds);float desired=active && hands[i].valid && head.valid && controls[i].enabled?1.f:0.f;weights[i]+=std::clamp(desired-weights[i],-dt/.18f,dt/.18f);float alpha=1-std::exp(-dt/.04f);fingerGrip[i]+=(grips[i]-fingerGrip[i])*alpha;fingerTrigger[i]+=(triggers[i]-fingerTrigger[i])*alpha;fingerThumb[i]+=((!fingerSetup.enabled && thumbPressed[i]?1.f:0.f)-fingerThumb[i])*alpha;}
 if(setupAPress.Step(active && nativeValid && vehicleBoxesReady && !fingerSetup.enabled,modePressed[1],now)){ToggleExclusiveBoxSetup(boxEditor,fingerSetup);ControllerToggleFeedback(1);PlaySetupFeedback(boxEditor.enabled);}
 BoxGrabInput boxInputs[2]{};bool sceneFresh=active && head.valid && sceneXRFrameTick && now-sceneXRFrameTick<250 && sceneCameraTick && now-sceneCameraTick<250;
 for(unsigned i=0;i<2;++i)if(sceneFresh && hands[i].valid){boxInputs[i]={actualSceneFrame.Position(hands[i].point,head.point,head.rotation),actualSceneFrame.Orientation(hands[i].rotation,head.rotation),true,grips[i]};if(boxEditor.enabled){auto aim=controllerAim[i].valid?controllerAim[i]:hands[i];auto origin=actualSceneFrame.Position(aim.point,head.point,head.rotation);auto orientation=actualSceneFrame.Orientation(aim.rotation,head.rotation);auto hit=RayInteractionBoxes(boxEditor,origin,DriverIK::Rotate(orientation,{0,0,-1}),2.f,handBoxes.valid);boxInputs[i].rayBox=hit.box;boxInputs[i].rayDistance=hit.distance;}}
 bool resetDialogActive=UpdateResetDialog(triggers,active&&nativeValid&&sceneFresh);if(resetDialogActive)for(auto& input:boxInputs)input.valid=false;
 bool cameraTracked[2]{boxInputs[0].valid,boxInputs[1].valid};auto cameraMid=DriverIK::Sub(DriverIK::Mul(DriverIK::Add(boxInputs[0].position,boxInputs[1].position),.5f),actualSceneFrame.camera);bool emptySpace=true;for(unsigned side=0;side<2;++side){if(boxInputs[side].rayBox>=0)emptySpace=false;for(unsigned box=0;box<InteractionBoxCount;++box){if((box==12||box==13)&&!handBoxes.valid)continue;const auto& b=boxEditor.boxes[box];auto local=DriverIK::Rotate(DriverIK::Inverse(b.rotation),DriverIK::Sub(boxInputs[side].position,b.center));bool cameraNear=true;for(unsigned axis=0;axis<3;++axis)cameraNear=cameraNear&&std::abs(local[axis])<=b.half[axis]*b.scale*b.stretch[axis]+.08f;if(cameraNear)emptySpace=false;}}
 bool cameraPermitted=boxEditor.enabled&&vehicleBoxesReady&&sceneFresh&&nativeValid&&!resetDialogActive&&boxEditor.selected<0&&steeringHandDrag.owner<0&&(cameraSeatDrag.active||emptySpace)&&triggers[0]<.2f&&triggers[1]<.2f;DriverIK::V movedCamera{};if(cameraSeatDrag.Step(cameraPermitted,cameraTracked,grips,cameraMid,actualSceneFrame.seatOffset,movedCamera)){vehicleSteeringPose.settings.cameraConfigured=true;vehicleSteeringPose.settings.cameraSeat=movedCamera;pendingCameraPosition=movedCamera;cameraPositionPending=true;steeringPoseDirty=true;}bool cameraDragActive=cameraSeatDrag.active;if(cameraDragActive)for(auto& input:boxInputs)input.valid=false;
 SteeringHandDragInput poseInputs[2]{};bool poseEditing=!cameraDragActive&&!resetDialogActive&&boxEditor.enabled&&vehicleBoxesReady&&handBoxes.valid&&sceneFresh&&nativeValid&&boxEditor.selected<0;
 for(unsigned side=0;side<2;++side){auto& input=poseInputs[side];input.position=boxInputs[side].position;input.valid=boxInputs[side].valid&&grips[side]<.3f;input.trigger=triggers[side];if(input.valid){int contact=-1;float distance=1e9f;for(unsigned hand=0;hand<2;++hand){const auto& box=boxEditor.boxes[12+hand];DriverIK::V half{};for(unsigned axis=0;axis<3;++axis)half[axis]=box.half[axis]*box.scale*box.stretch[axis];if(IgnitionBoxesOverlap(box.center,half,input.position,boxInputs[side].rotation,{.025f,.025f,.04f},box.rotation)){float d=DriverIK::Length(DriverIK::Sub(box.center,input.position));if(d<distance){contact=hand;distance=d;}}}input.target=contact>=0?contact:boxInputs[side].rayBox==12?0:boxInputs[side].rayBox==13?1:-1;}}
 int priorPoseOwner=steeringHandDrag.owner;if(steeringHandDrag.Step(poseInputs,poseEditing,vehicleSteeringPose.settings.offset))steeringPoseDirty=true;if(steeringHandDrag.owner>=0){unsigned hand=steeringHandDrag.hand;controls[hand].enabled=false;controls[hand].calibrate=false;weights[hand]=0;for(auto& input:boxInputs)input.valid=false;if(priorPoseOwner<0)ControllerToggleFeedback(steeringHandDrag.owner);}else if(steeringPoseDirty&&!cameraDragActive)SaveSteeringPoseEdit();
 boxEditor.Step(boxInputs,InteractionBoxCount,handBoxes.valid);if(boxEditor.enabled)handBoxes.CaptureEdit(boxEditor);
 if(boxEditor.dirty && vehicleBoxesReady && !guideFile.empty()){
  SaveActiveVehicleBoxes();

 }

 RecordSetupHistory();
 if(active&&sceneFresh&&vehicleBoxesReady&&now-lastBodyInteractionTick>150){handBoxes.valid=false;for(unsigned side=0;side<2;++side){controls[side].enabled=false;weights[side]=0;}float actionTrigger[2]{fingerTrigger[0],fingerTrigger[1]};DriverIK::World empty{};UpdateCabinInteractionsLocked(empty,0,actionTrigger,false);}
 SubmitSetupBoxes(active&&sceneFresh&&vehicleBoxesReady&&boxEditor.enabled);

}
unsigned ApplyControllers(DriverIK::Pose& pose,const DriverIK::Positions& positions,bool allowLeft,bool allowRight){
 std::lock_guard lock(guard);if(!tick || GetTickCount64()-tick>250)return 0;unsigned mask=0;
 const auto grabNow=GetTickCount64();bool grabFresh=head.valid&&sceneXRFrameTick&&grabNow-sceneXRFrameTick<250&&sceneCameraTick&&grabNow-sceneCameraTick<250;
 auto initialWorld=DriverIK::FK(pose,positions);
 for(unsigned side=0;side<2;++side){unsigned wrist=side?28:7;handBoxes.wrist[side]=initialWorld.p[wrist];handBoxes.rotation[side]=initialWorld.q[wrist];boxEditor.boxes[12+side]=AttachHandBox(handBoxes.local[side],handBoxes.wrist[side],handBoxes.rotation[side]);}handBoxes.valid=true;
 for(unsigned side=0;side<2;++side){
  auto raw=actualSceneFrame.Position(hands[side].point,head.point,head.rotation);auto rawQ=actualSceneFrame.Orientation(hands[side].rotation,head.rotation);auto& grabBox=boxEditor.boxes[12+side];DriverIK::V half{};for(unsigned j=0;j<3;++j)half[j]=grabBox.half[j]*grabBox.scale*grabBox.stretch[j];
  bool contact=IgnitionBoxesOverlap(grabBox.center,half,raw,rawQ,{.025f,.025f,.04f},grabBox.rotation);
  auto aim=controllerAim[side].valid?controllerAim[side]:hands[side];auto rayOrigin=actualSceneFrame.Position(aim.point,head.point,head.rotation);auto rayQ=actualSceneFrame.Orientation(aim.rotation,head.rotation);auto hit=RayInteractionBoxes(boxEditor,rayOrigin,DriverIK::Rotate(rayQ,{0,0,-1}));
  bool permitted=vehicleBoxesReady && (side?allowRight:allowLeft)&&grabFresh&&hands[side].valid&&NativeControllerInputStatus()==0;
  if(fingerTrigger[side]<.2f)suppressGrabTrigger[side]=false;
  int action=handGrabGestures[side].Step(controls[side].enabled,fingerTrigger[side],permitted,boxEditor.enabled,contact||hit.box==static_cast<int>(12+side),grabNow);
  if(action){controls[side].enabled=action>0;controls[side].calibrate=false;handFeedback[side]=action;suppressGrabTrigger[side]=true;
   if(action>0){CalibrateControllerHand(controls[side],pose,positions,side!=0,raw,rawQ,{1,0,0,0});++calibrations[side];}
  }
 }
 float actionTrigger[2]{suppressGrabTrigger[0]?0.f:fingerTrigger[0],suppressGrabTrigger[1]?0.f:fingerTrigger[1]};


 for(unsigned i=0;i<2;++i){if(!(i?allowRight:allowLeft) || weights[i]<=0 || (controls[i].calibrate && (!hands[i].valid || !head.valid)))continue;
  if(!sceneXRFrameTick || GetTickCount64()-sceneXRFrameTick>250 || !sceneCameraTick || GetTickCount64()-sceneCameraTick>250)continue;
  auto temporary=controls[i];temporary.enabled=true;bool calibration=temporary.calibrate;auto now=GetTickCount64();float seconds=lastSolveTick[i]?std::min(.05f,(now-lastSolveTick[i])*.001f):1.f/90.f;
  auto controllerPoint=actualSceneFrame.Position(hands[i].point,head.point,head.rotation);auto controllerRotation=actualSceneFrame.Orientation(hands[i].rotation,head.rotation);
  if(ApplyControllerHand(pose,positions,i!=0,temporary,controllerPoint,controllerRotation,{1,0,0,0},weights[i],seconds,elbowGuides.values[i],twistShare,true)){auto fingers=liveFingers[i].Snapshot();auto selected=fingerMode[i].Targets(*fingers);BlendControllerFingers(pose,i!=0,fingerGrip[i],fingerTrigger[i],weights[i],&selected,&fingerMode[i].weights,fingerThumb[i],static_cast<int>(fingerMode[i].finger),std::sin(3.14159265f*std::clamp(fingerMode[i].previewSeconds/.35f,0.f,1.f)));controls[i].origin=temporary.origin;controls[i].originRotation=temporary.originRotation;controls[i].anchor=temporary.anchor;controls[i].anchorRotation=temporary.anchorRotation;controls[i].basis=temporary.basis;controls[i].calibrate=temporary.calibrate;controls[i].bodyOrientation=temporary.bodyOrientation;controls[i].priorDirection=temporary.priorDirection;controls[i].priorBend=temporary.priorBend;controls[i].haveBend=temporary.haveBend;controls[i].neutralWrist=temporary.neutralWrist;controls[i].previousTwist=temporary.previousTwist;controls[i].haveTwist=temporary.haveTwist;lastSolveTick[i]=now;mask|=1u<<i;++solves[i];if(calibration)++calibrations[i];}else ++rejects[i];
 }
 lastBodyInteractionTick=GetTickCount64();
 UpdateCabinInteractionsLocked(DriverIK::FK(pose,positions),mask,actionTrigger,true);
 return mask;
}
void UpdateCabinInteractionsLocked(const DriverIK::World& handWorld,unsigned mask,const float* actionTrigger,bool bodyPresent){
 sceneMarkers={};sceneMarkerTick=GetTickCount64();mirrorContact[0]=mirrorContact[1]=-1;mirrorContactTick=sceneMarkerTick;hornBoxHeld=false;hornBoxTick=sceneMarkerTick;
 if(!boxEditor.authored[0]){boxEditor.boxes[0].center=ignitionTarget;boxEditor.boxes[0].scale=1;}
 if(!boxEditor.authored[1]){boxEditor.boxes[1].center=hornTarget;boxEditor.boxes[1].scale=hornBoxScale;}
 if(!boxEditor.authored[9]){boxEditor.boxes[9].center=boxEditor.boxes[0].center;boxEditor.boxes[9].center[0]-=.12f;}
 if(!boxEditor.authored[10]){boxEditor.boxes[10].center=boxEditor.boxes[9].center;boxEditor.boxes[10].center[1]+=.10f;}
 auto keyBox=boxEditor.boxes[0],hornBox=boxEditor.boxes[1];
 auto drawBox=[&](unsigned bone,const InteractionBox& box){auto& m=sceneMarkers[bone];for(unsigned col=0;col<3;++col){DriverIK::V axis{};axis[col]=box.scale*box.stretch[col];auto v=DriverIK::Rotate(box.rotation,axis);for(unsigned row=0;row<3;++row)m[row*4+col]=v[row];}m[3]=box.center[0];m[7]=box.center[1];m[11]=box.center[2];m[15]=1;};
 if(boxEditor.enabled){drawBox(44,keyBox);for(unsigned j=2;j<12;++j)drawBox(j==11?46:j>=5?50+j:48+j,boxEditor.boxes[j]);}
 if(bodyPresent)for(unsigned side=0;side<2;++side){unsigned wrist=side?28:7;handBoxes.wrist[side]=handWorld.p[wrist];handBoxes.rotation[side]=handWorld.q[wrist];boxEditor.boxes[12+side]=AttachHandBox(handBoxes.local[side],handBoxes.wrist[side],handBoxes.rotation[side]);if(boxEditor.enabled)drawBox(side?29:8,boxEditor.boxes[12+side]);}bool keyFresh=vehicleBoxesReady && (boxEditor.authored[0]||(ignitionTargetTick && sceneMarkerTick-ignitionTargetTick<250));
 bool sceneFresh=head.valid && sceneXRFrameTick && sceneMarkerTick-sceneXRFrameTick<250 && sceneCameraTick && sceneMarkerTick-sceneCameraTick<250;
 bool hornFresh=vehicleBoxesReady && (boxEditor.authored[1]||(hornTargetTick && sceneMarkerTick-hornTargetTick<250));
 if(boxEditor.enabled)drawBox(49,hornBox);
 ControllerContactVolume volumes[2]{};InteractionRayHit rayHits[2]{};
 for(unsigned side=0;side<2;++side){unsigned wrist=side?28:7;auto offset=side?handBoxOffset:leftHandBoxOffset;auto palm=DriverIK::Add(handWorld.p[wrist],DriverIK::Rotate(handWorld.q[wrist],offset));
  auto raw=actualSceneFrame.Position(hands[side].point,head.point,head.rotation);auto rawQ=actualSceneFrame.Orientation(hands[side].rotation,head.rotation);
  auto& volume=volumes[side];volume=ControllerInteractionVolume(controls[side].enabled,hands[side].valid,sceneFresh,(mask&(1u<<side))!=0,palm,handWorld.q[wrist],raw,rawQ);
  if(!volume.valid){lightTriggers[side].Step(0,false,false);wiperTriggers[side].Step(0,false,false);radioTriggers[side].Step(0,false,false);continue;}
  // Hand-attached grab boxes provide the setup hand markers.
  if(!controls[side].enabled){
   auto aim=controllerAim[side].valid?controllerAim[side]:hands[side];auto aimOrigin=actualSceneFrame.Position(aim.point,head.point,head.rotation);auto aimQ=actualSceneFrame.Orientation(aim.rotation,head.rotation);auto direction=DriverIK::Rotate(aimQ,{0,0,-1});rayHits[side]=RayInteractionBoxes(boxEditor,aimOrigin,direction);
   cabinLaserDistance[side]=rayHits[side].distance;cabinLaserHit[side]=rayHits[side].box>=0;cabinLaserTick[side]=GetTickCount64();
   InteractionBox laser{DriverIK::Add(aimOrigin,DriverIK::Mul(direction,rayHits[side].distance*.5f)),{.0015f,.0015f,.5f},aimQ,1,{1,1,std::max(.001f,rayHits[side].distance)}};if(!cabinLaserReady[side])drawBox(53+side,laser);
  }
  const auto& lightsBox=boxEditor.boxes[9];bool lightsHit=controls[side].enabled?IgnitionBoxesOverlap(lightsBox.center,DriverIK::V{lightsBox.half[0]*lightsBox.scale*lightsBox.stretch[0],lightsBox.half[1]*lightsBox.scale*lightsBox.stretch[1],lightsBox.half[2]*lightsBox.scale*lightsBox.stretch[2]},volume.center,volume.rotation,{.04f,.035f,.055f},lightsBox.rotation):rayHits[side].box==9;
  if(lightTriggers[side].Step(actionTrigger[side],vehicleBoxesReady && !boxEditor.enabled && volume.valid,lightsHit)){lightRequestTick=sceneMarkerTick;ControllerToggleFeedback(side);}
  const auto& wipersBox=boxEditor.boxes[10];bool wipersHit=controls[side].enabled?IgnitionBoxesOverlap(wipersBox.center,DriverIK::V{wipersBox.half[0]*wipersBox.scale*wipersBox.stretch[0],wipersBox.half[1]*wipersBox.scale*wipersBox.stretch[1],wipersBox.half[2]*wipersBox.scale*wipersBox.stretch[2]},volume.center,volume.rotation,{.04f,.035f,.055f},wipersBox.rotation):rayHits[side].box==10;
  if(wiperTriggers[side].Step(actionTrigger[side],vehicleBoxesReady && !boxEditor.enabled && volume.valid,wipersHit)){wiperRequestTick=sceneMarkerTick;ControllerToggleFeedback(side);}
  for(unsigned action=0;action<2;++action){const auto& displayBox=boxEditor.boxes[14+action];DriverIK::V half{};for(unsigned axis=0;axis<3;++axis)half[axis]=displayBox.half[axis]*displayBox.scale*displayBox.stretch[axis];bool hit=controls[side].enabled?IgnitionBoxesOverlap(displayBox.center,half,volume.center,volume.rotation,{.04f,.035f,.055f},displayBox.rotation):rayHits[side].box==static_cast<int>(14+action);if(displayTriggers[action][side].Step(actionTrigger[side],vehicleBoxesReady&&!boxEditor.enabled&&volume.valid,hit)){displayRequestTicks[action]=sceneMarkerTick;ControllerToggleFeedback(side);}}
  const auto& radioBox=boxEditor.boxes[11];bool radioHit=controls[side].enabled?IgnitionBoxesOverlap(radioBox.center,DriverIK::V{radioBox.half[0]*radioBox.scale*radioBox.stretch[0],radioBox.half[1]*radioBox.scale*radioBox.stretch[1],radioBox.half[2]*radioBox.scale*radioBox.stretch[2]},volume.center,volume.rotation,{.04f,.035f,.055f},radioBox.rotation):rayHits[side].box==11;
  if(radioTriggers[side].Step(actionTrigger[side],vehicleBoxesReady && !boxEditor.enabled && volume.valid,radioHit)){radioCommand.toggle=true;radioTick=sceneMarkerTick;ControllerToggleFeedback(side);}
  if(vehicleBoxesReady && !boxEditor.enabled)mirrorContact[side]=controls[side].enabled?MirrorContact(boxEditor,volume.center,volume.rotation):(rayHits[side].box>=2?rayHits[side].box-2:-1);
  if(ControllerHornPressed(!boxEditor.enabled && hornFresh && (controls[side].enabled?IgnitionBoxesOverlap(hornBox.center,DriverIK::V{hornBox.half[0]*hornBox.scale*hornBox.stretch[0],hornBox.half[1]*hornBox.scale*hornBox.stretch[1],hornBox.half[2]*hornBox.scale*hornBox.stretch[2]},volume.center,volume.rotation,{.04f,.035f,.055f},hornBox.rotation):rayHits[side].box==1),vehicleBoxesReady && volume.valid,actionTrigger[side]))hornBoxHeld=true;
 }
 for(unsigned side=0;side<2;++side){
 bool overlap=vehicleBoxesReady && volumes[side].valid && keyFresh && (controls[side].enabled?IgnitionBoxesOverlap(keyBox.center,DriverIK::V{keyBox.half[0]*keyBox.scale*keyBox.stretch[0],keyBox.half[1]*keyBox.scale*keyBox.stretch[1],keyBox.half[2]*keyBox.scale*keyBox.stretch[2]},volumes[side].center,volumes[side].rotation,{.04f,.035f,.055f},keyBox.rotation):rayHits[side].box==0);
 if(ignitionTriggers[side].Step(actionTrigger[side],vehicleBoxesReady && volumes[side].valid && keyFresh && !boxEditor.enabled,overlap)){interactionRequestTick=sceneMarkerTick;ControllerToggleFeedback(side);}
 }

 if(head.valid && sceneXRFrameTick && sceneMarkerTick-sceneXRFrameTick<250 && sceneCameraTick && sceneMarkerTick-sceneCameraTick<250){
 for(unsigned side=0;side<2;++side)if(hands[side].valid && !controls[side].enabled && !cabinModelReady[side]){auto position=actualSceneFrame.Position(hands[side].point,head.point,head.rotation);auto rotation=actualSceneFrame.Orientation(hands[side].rotation,head.rotation);
  auto& m=sceneMarkers[side?28:7];for(unsigned col=0;col<3;++col){DriverIK::V axis{};axis[col]=1;auto v=DriverIK::Rotate(rotation,axis);for(unsigned row=0;row<3;++row)m[row*4+col]=v[row];}m[3]=position[0];m[7]=position[1];m[11]=position[2];m[15]=1;}
 }

}
void ControllerStatus(char* buffer,size_t length){std::lock_guard lock(guard);std::snprintf(buffer,length,"CONTROLLER_IK mirror_bridge=%d connected=%d error=%d marker_error=%d native_input=%d twist_share=%.3f grip=%.3f,%.3f trigger=%.3f,%.3f left_valid=%d right_valid=%d left_enabled=%d right_enabled=%d samples=%llu,%llu solves=%llu,%llu rejected=%llu,%llu calibrations=%llu,%llu finger_mode=%s,%s mode_switches=%llu,%llu finger_pose_revision=%llu,%llu finger_pose_rejected=%llu,%llu",mirrorBridgeStatus,systemVR?1:0,errorCode,markerError,NativeControllerInputStatus(),twistShare,fingerGrip[0],fingerGrip[1],fingerTrigger[0],fingerTrigger[1],hands[0].valid?1:0,hands[1].valid?1:0,controls[0].enabled?1:0,controls[1].enabled?1:0,samples[0],samples[1],solves[0],solves[1],rejects[0],rejects[1],calibrations[0],calibrations[1],fingerMode[0].Name(),fingerMode[1].Name(),fingerMode[0].switches,fingerMode[1].switches,liveFingers[0].Revision(),liveFingers[1].Revision(),liveFingers[0].Rejected(),liveFingers[1].Rejected());}
void CloseControllers(){
 std::lock_guard lock(guard);cameraSeatDrag.Cancel();CancelResetDialog();steeringHandDrag.Release();SaveSteeringPoseEdit();SubmitSetupBoxes(false);PollControllerShortcuts(false);HideMarkers();DestroyMenuLasers();
 cabinModelsActive=false;cabinModelReady[0]=cabinModelReady[1]=false;
 // OpenVR and the game's OpenXR runtime share vrclient in this process.
 // A framework plugin toggle must not shut that client down under xrEndFrame.
 // Keep the connection and helper loaded until process exit; reactivation reuses them.
}

bool ApplyControllerMarkerPalette(DriverPalette& palette){std::lock_guard lock(guard);palette={};if(sceneMarkerTick && GetTickCount64()-sceneMarkerTick<=250)palette=sceneMarkers;if(setupMeshesReady)for(unsigned bone:{44u,49u,50u,51u,52u,55u,56u,57u,58u,59u,60u,46u,8u,29u})palette[bone]={};return true;}

void ResetControllerMarkerFrame(){std::lock_guard lock(guard);markerCalibrated=false;}

void SetControllerSceneXRFrame(const float* reference,const float* current){std::lock_guard lock(guard);actualSceneFrame.neutral={reference[0],reference[1],reference[2]};actualSceneFrame.neutralQ={reference[4],reference[5],reference[6],reference[7]};actualSceneFrame.xrHead={current[0],current[1],current[2]};actualSceneFrame.xrHeadQ={current[4],current[5],current[6],current[7]};sceneXRFrameTick=GetTickCount64();}
bool SetControllerVehicleCamera(DriverIK::V cabin,DriverIK::V position){std::lock_guard lock(guard);return actualSceneFrame.SetVehicleCamera(cabin,position);}
void SetControllerSceneCamera(float x,float y,float z){if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))return;std::lock_guard lock(guard);actualSceneFrame.SetSeatOffset({x,y,z});sceneCameraTick=GetTickCount64();}

void SetIgnitionInteractionTarget(DriverIK::V point){for(float v:point)if(!std::isfinite(v))return;std::lock_guard lock(guard);ignitionTarget=DriverIK::Add(point,ignitionBoxOffset);ignitionTargetTick=GetTickCount64();}
bool ConsumeControllerIgnitionRequest(){std::lock_guard lock(guard);auto event=interactionRequestTick;interactionRequestTick=0;return event && GetTickCount64()-event<250;}

void SetHornInteractionTarget(DriverIK::V point){for(float v:point)if(!std::isfinite(v))return;std::lock_guard lock(guard);hornTarget=DriverIK::Add(point,hornBoxOffset);hornTargetTick=GetTickCount64();}
bool ControllerHornBoxHeld(){std::lock_guard lock(guard);return hornBoxHeld && hornBoxTick && GetTickCount64()-hornBoxTick<250;}

int ControllerBoxSetupKey(bool held){std::lock_guard lock(guard);if(!vehicleBoxesReady)return 0;auto result=KeyExclusiveBoxSetup(boxEditor,fingerSetup,held);if(result)PlaySetupFeedback(boxEditor.enabled);return result;}

float ControllerWindowDirection(unsigned side){std::lock_guard lock(guard);return side<2 && windowRequestTick && GetTickCount64()-windowRequestTick<150 && !boxEditor.enabled?windowRequests[side]:0;}

bool ConsumeControllerLightsRequest(){std::lock_guard lock(guard);auto event=lightRequestTick;lightRequestTick=0;return event && GetTickCount64()-event<250;}

bool ConsumeControllerWipersRequest(){std::lock_guard lock(guard);auto event=wiperRequestTick;wiperRequestTick=0;return event && GetTickCount64()-event<250;}


ControllerRadioCommand TakeControllerRadioCommand(){std::lock_guard lock(guard);auto out=radioCommand;radioCommand.station=0;radioCommand.toggle=false;if(!radioTick || GetTickCount64()-radioTick>150 || boxEditor.enabled)return {};return out;}


void QueueControllerRadioTuneSound(){std::lock_guard lock(guard);radioTunePending=true;}

void QueueControllerRadioClickSound(bool playing){std::lock_guard lock(guard);radioClickPending=playing?1:-1;}


namespace {MenuLaserSurface menuSurface;uint64_t menuSurfaceTick=0;vr::VROverlayHandle_t menuBeams[2]{},menuDots[2]{};
void DestroyMenuLasers(){CancelResetDialog();if(markerOverlay&&resetDialogOverlay){markerOverlay->DestroyOverlay(resetDialogOverlay);resetDialogOverlay=0;}HideRadialMenus();if(markerOverlay)for(auto& handle:radialMenus){if(handle)markerOverlay->DestroyOverlay(handle);handle=0;}radialSelection[0]=radialSelection[1]=-2;DestroyMenuControllerModels();if(markerOverlay)for(unsigned i=0;i<2;++i){if(menuBeams[i])markerOverlay->DestroyOverlay(menuBeams[i]);if(menuDots[i])markerOverlay->DestroyOverlay(menuDots[i]);menuBeams[i]=menuDots[i]=0;}menuSurfaceTick=0;}
void HideMenuLasers(){if(markerOverlay)for(unsigned i=0;i<2;++i){if(menuBeams[i])markerOverlay->HideOverlay(menuBeams[i]);if(menuDots[i])markerOverlay->HideOverlay(menuDots[i]);}}
void EnsureMenuLasers(){if(!markerOverlay)return;for(unsigned i=0;i<2;++i){if(!menuBeams[i]){auto key=std::string("spf.driver.menu.beam.")+std::to_string(i);if(markerOverlay->CreateOverlay(key.c_str(),"Menu controller laser",&menuBeams[i])==vr::VROverlayError_None){std::vector<unsigned char> pixels(4*1024*4,255);markerOverlay->SetOverlayRaw(menuBeams[i],pixels.data(),4,1024,4);}}if(!menuDots[i]){auto key=std::string("spf.driver.menu.dot.")+std::to_string(i);if(markerOverlay->CreateOverlay(key.c_str(),"Menu laser hit",&menuDots[i])==vr::VROverlayError_None){std::array<unsigned char,32*32*4> pixels{};for(int y=0;y<32;++y)for(int x=0;x<32;++x){auto n=(y*32+x)*4;pixels[n]=pixels[n+1]=pixels[n+2]=255;pixels[n+3]=(x-16)*(x-16)+(y-16)*(y-16)<200?255:0;}markerOverlay->SetOverlayRaw(menuDots[i],pixels.data(),32,32,4);markerOverlay->SetOverlayWidthInMeters(menuDots[i],.012f);}}for(auto h:{menuBeams[i],menuDots[i]})if(h)markerOverlay->SetOverlayColor(h,1.f,1.f,1.f);}}
vr::HmdMatrix34_t MenuOverlayMatrix(DriverIK::V x,DriverIK::V y,DriverIK::V z,DriverIK::V p){vr::HmdMatrix34_t m{};for(unsigned r=0;r<3;++r){m.m[r][0]=x[r];m.m[r][1]=y[r];m.m[r][2]=z[r];m.m[r][3]=p[r];}return m;}
void HideControllerPointer(unsigned side){if(markerOverlay){if(menuBeams[side])markerOverlay->HideOverlay(menuBeams[side]);if(menuDots[side])markerOverlay->HideOverlay(menuDots[side]);}}
bool ShowControllerPointer(unsigned side,const Tracked& hand,const Tracked& h,float length,bool hit){
 using namespace DriverIK;EnsureMenuLasers();if(!markerOverlay || !menuBeams[side])return false;
 auto y=Rotate(hand.rotation,{0,0,-1});auto middle=Add(hand.point,Mul(y,length*.5f));auto z=Unit(Sub(h.point,middle));auto x=Unit(Cross(y,z));z=Unit(Cross(x,y));auto m=MenuOverlayMatrix(x,y,z,middle);
 // Preserve a 1.5 mm beam width while stretching its height to the hit distance.
 markerOverlay->SetOverlayWidthInMeters(menuBeams[side],.0015f);markerOverlay->SetOverlayTexelAspect(menuBeams[side],256.f*.0015f/std::max(.001f,length));markerOverlay->SetOverlayTransformAbsolute(menuBeams[side],vr::TrackingUniverseStanding,&m);markerOverlay->ShowOverlay(menuBeams[side]);
 if(menuDots[side]){if(hit){auto point=Add(hand.point,Mul(y,length));auto q=h.rotation;auto dot=MenuOverlayMatrix(Rotate(q,{1,0,0}),Rotate(q,{0,1,0}),Rotate(q,{0,0,1}),point);markerOverlay->SetOverlayTransformAbsolute(menuDots[side],vr::TrackingUniverseStanding,&dot);markerOverlay->ShowOverlay(menuDots[side]);}else markerOverlay->HideOverlay(menuDots[side]);}return true;
}
Tracked MenuAimPose(vr::TrackedDeviceIndex_t device,const vr::TrackedDevicePose_t& raw){
 if(!controllerModels || !systemVR)return Convert(raw);char model[256]{};vr::ETrackedPropertyError error{};systemVR->GetStringTrackedDeviceProperty(device,vr::Prop_RenderModelName_String,model,sizeof(model),&error);vr::VRControllerState_t buttons{};vr::RenderModel_ControllerMode_State_t mode{};vr::RenderModel_ComponentState_t component{};
 if(error!=vr::TrackedProp_Success)return Convert(raw);systemVR->GetControllerState(device,&buttons,sizeof(buttons));if(!controllerModels->GetComponentState(model,"tip",&buttons,&mode,&component))return Convert(raw);
 MenuPoseMatrix deviceMatrix{},tipMatrix{};for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c){deviceMatrix[r][c]=raw.mDeviceToAbsoluteTracking.m[r][c];tipMatrix[r][c]=component.mTrackingToComponentLocal.m[r][c];}auto composed=ComposeMenuAim(deviceMatrix,tipMatrix);auto pose=raw;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)pose.mDeviceToAbsoluteTracking.m[r][c]=composed[r][c];return Convert(pose);
}
#include "ControllerMenuModels.inc"
void UpdateCabinControllerModels(const vr::TrackedDevicePose_t* poses){
 std::array<ControllerMeshPose,64> modelPoses{};for(unsigned side=0;side<2;++side){cabinLaserReady[side]=false;auto index=systemVR?systemVR->GetTrackedDeviceIndexForControllerRole(side?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand):vr::k_unMaxTrackedDeviceCount;if(!head.valid || index>=vr::k_unMaxTrackedDeviceCount || !hands[side].valid || controls[side].enabled){HideControllerPointer(side);continue;}controllerAim[side]=MenuAimPose(index,poses[index]);cabinModelReady[side]=ShowMenuControllerModel(side,index,poses[index],head,modelPoses);bool fresh=GetTickCount64()-cabinLaserTick[side]<250;cabinLaserReady[side]=ShowControllerPointer(side,controllerAim[side],head,fresh?cabinLaserDistance[side]:2.f,fresh && cabinLaserHit[side]);}SubmitMenuModelPoses(modelPoses);
}
extern "C" bool RequestControllerHeadReset() noexcept;
extern "C" bool IsWorldMapMenuActive() noexcept;
void PlayKeyboardShortcutSound(bool enter){
 static const auto folder=[]{HMODULE module=nullptr;wchar_t path[32768]{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PlayKeyboardShortcutSound),&module)||!GetModuleFileNameW(module,path,32768))return std::filesystem::path{};return std::filesystem::path(path).parent_path()/"sounds";}();
 if(!folder.empty())PlaySoundW((folder/(enter?"controller_key_enter.wav":"controller_key_escape.wav")).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);
}
void UpdateMenuKeys(bool active,bool x,bool y,bool enter,bool escape,uint64_t now){
 static MenuKeyPulse pulses[2];static bool previousX=false,previousY=false;
 bool inputChanged=x!=previousX||y!=previousY;previousX=x;previousY=y;
 if(inputChanged||enter||escape)ReportMenuShortcut(x,y,enter,escape,0,0,0);
 const bool fires[2]{enter,escape};const WORD scans[2]{0x1c,0x01};
 for(unsigned i=0;i<2;++i){int edge=pulses[i].Step(active,fires[i],now);if(!edge)continue;INPUT event{};event.type=INPUT_KEYBOARD;event.ki.wScan=scans[i];event.ki.dwFlags=KEYEVENTF_SCANCODE|(edge<0?KEYEVENTF_KEYUP:0);auto sent=SendInput(1,&event,sizeof(INPUT));if(sent==1&&edge>0)PlayKeyboardShortcutSound(i==0);ReportMenuShortcut(x,y,enter,escape,scans[i],edge,sent);}
}
// Keyboard shortcuts apply to the focused game, independent of menu surface or hand IK.
void PollControllerShortcuts(bool permitted){
 static MenuDoublePress enterPress,escapePress,recenterPress;
 DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);
 bool pressed[2]{},mode[2]{},thumb[2]{};float grips[2]{},triggers[2]{},inward[2]{};
 bool valid=permitted&&process==GetCurrentProcessId()&&PollNativeControllerButtons(pressed,grips,triggers,mode,thumb,inward);
 auto now=GetTickCount64();bool enter=enterPress.Step(valid && !fingerSetup.enabled,mode[0],now),escape=escapePress.Step(valid && !fingerSetup.enabled,thumb[0],now);
 UpdateMenuKeys(valid,mode[0],thumb[0],enter,escape,now);
 if(recenterPress.Step(valid && !fingerSetup.enabled,thumb[1],now))RequestControllerHeadReset();
}
bool PollMenuLaser(bool menu){
 using namespace DriverIK;static ControllerMenuMouse click,rightClick;static MenuButtonRoute buttons;static MenuCursorMotion cursor;static IgnitionTriggerGate choose[2],chooseGrip[2];static int owner=-1;static uint64_t last=0;auto now=GetTickCount64();float dt=last?(now-last)*.001f:0;last=now;
 DWORD pid=0;auto window=GetForegroundWindow();GetWindowThreadProcessId(window,&pid);menu=menu && pid==GetCurrentProcessId();
 bool surfaceFresh=menuSurfaceTick && now-menuSurfaceTick<250;
 if(!menu || !surfaceFresh){if(!cabinModelsActive){HideMenuLasers();HideRadialMenus();}if(!cabinModelsActive)HideMenuControllerModels();auto out=click.Step(false,false,0,0,0,0,0);auto rightOut=rightClick.Step(false,false,0,0,0,0,0);auto released=buttons.Cancel();INPUT release[2]{};unsigned count=0;for(auto button:{(released.released&1)?MOUSEEVENTF_LEFTUP:0,(released.released&2)?MOUSEEVENTF_RIGHTUP:0})if(button){release[count].type=INPUT_MOUSE;release[count++].mi.dwFlags=button;}if(count)SendInput(count,release,sizeof(INPUT));owner=-1;cursor.Step(false,-1,0,0,0,0,0);choose[0]=choose[1]={};chooseGrip[0]=chooseGrip[1]={};return false;}
 if(!systemVR && now-attempt>=5000){attempt=now;Connect();}
 bool pressed[2]{},mode[2]{},thumb[2]{};float grips[2]{},triggers[2]{},inward[2]{},sticks[4]{};bool valid=systemVR && PollNativeControllerButtons(pressed,grips,triggers,mode,thumb,inward);if(valid)ReadControllerStickAxes(sticks);
 vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount]{};if(valid)systemVR->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding,0,poses,vr::k_unMaxTrackedDeviceCount);
 auto h=Convert(poses[0]);valid=valid && h.valid;UpdateRadialMenus(poses,sticks,pressed,valid);

 MenuLaserHit hits[2]{};std::array<ControllerMeshPose,64> modelPoses{};EnsureMenuLasers();
 for(unsigned side=0;side<2;++side){auto index=valid?systemVR->GetTrackedDeviceIndexForControllerRole(side?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand):vr::k_unMaxTrackedDeviceCount;auto hand=index<vr::k_unMaxTrackedDeviceCount?MenuAimPose(index,poses[index]):Tracked{};
  if(!valid || !hand.valid){if(markerOverlay){for(auto handle:markers[side])if(handle)markerOverlay->HideOverlay(handle);if(menuBeams[side])markerOverlay->HideOverlay(menuBeams[side]);if(menuDots[side])markerOverlay->HideOverlay(menuDots[side]);}continue;}
  auto ray=MenuRayInHeadSpace(hand.point,hand.rotation,h.point,h.rotation);auto start=ray.origin;auto direction=ray.direction;hits[side]=IntersectMenuLaser(menuSurface,start,direction);float length=hits[side].hit?hits[side].distance:2.f;ShowMenuControllerModel(side,index,poses[index],h,modelPoses);if(markerOverlay)for(auto handle:markers[side])if(handle)markerOverlay->HideOverlay(handle);
  ShowControllerPointer(side,hand,h,length,hits[side].hit);
 }
 SubmitMenuModelPoses(modelPoses);
 bool select[2]{};for(unsigned side=0;side<2;++side){bool triggerPress=choose[side].Step(triggers[side],valid,hits[side].hit);bool gripPress=chooseGrip[side].Step(grips[side],valid,hits[side].hit);select[side]=triggerPress||gripPress;}
 bool hitFlags[2]{hits[0].hit,hits[1].hit};if(!click.down && !rightClick.down && (select[0]||select[1])){click.Step(true,true,0,0,0,0,0);rightClick.Step(true,true,0,0,0,0,0);}owner=SelectMenuLaserOwner(owner,click.down||rightClick.down,hitFlags,select);
 bool hit=owner>=0 && hits[owner].hit;auto output=click.Step(hit,hit,0,0,hit?triggers[owner]:0,hit?sticks[owner*2+1]:0,dt);auto rightOutput=rightClick.Step(hit,hit,0,0,hit?grips[owner]:0,0,dt);INPUT events[5]{};UINT count=0;
 static float gainX=1.5f,gainY=1.5f;static uint64_t gainPoll=0;if(now-gainPoll>=500 && !guideFile.empty()){gainPoll=now;auto gainFile=OpenDriverSetting(guideFile.parent_path()/"controller_menu_cursor_gain.txt");if(gainFile){std::string text((std::istreambuf_iterator<char>(gainFile)),{});ParseMenuCursorGain(text,gainX,gainY);}}
 RECT rect{};GetClientRect(window,&rect);auto motion=hit?cursor.Step(true,owner,hits[owner].u,hits[owner].v,rect.right,rect.bottom,dt,gainX,gainY):MenuCursorMotion::Output{};
 auto routed=hit?buttons.Step(output.button,rightOutput.button,IsWorldMapMenuActive(),static_cast<float>(motion.dx),static_cast<float>(motion.dy),dt):buttons.Cancel();bool dragging=buttons.heldMask!=0;bool buttonPress=routed.pressed!=0;
 // Accessory preview orbit consumes relative mouse movement while captured.
 // Anchor on press, then avoid absolute warps until release.
 bool nativePositioned=hit && (!dragging || buttonPress) && ApplyNativeMenuCursor(hits[owner].u,hits[owner].v);
 if(!nativePositioned && motion.anchor && !dragging){POINT point{motion.x,motion.y};if(ClientToScreen(window,&point))SetCursorPos(point.x,point.y);}

 if(!nativePositioned && !buttonPress && (motion.dx||motion.dy)){auto& e=events[count++];e.type=INPUT_MOUSE;e.mi.dx=motion.dx;e.mi.dy=motion.dy;e.mi.dwFlags=MOUSEEVENTF_MOVE;}
 if(routed.pressed||routed.released){auto& e=events[count++];e.type=INPUT_MOUSE;e.mi.dwFlags=((routed.pressed&1)?MOUSEEVENTF_LEFTDOWN:0)|((routed.pressed&2)?MOUSEEVENTF_RIGHTDOWN:0)|((routed.released&1)?MOUSEEVENTF_LEFTUP:0)|((routed.released&2)?MOUSEEVENTF_RIGHTUP:0);}if(routed.tapped&1){auto& down=events[count++];down.type=INPUT_MOUSE;down.mi.dwFlags=MOUSEEVENTF_LEFTDOWN;auto& up=events[count++];up.type=INPUT_MOUSE;up.mi.dwFlags=MOUSEEVENTF_LEFTUP;}if(output.wheel){auto& e=events[count++];e.type=INPUT_MOUSE;e.mi.dwFlags=MOUSEEVENTF_WHEEL;e.mi.mouseData=static_cast<DWORD>(output.wheel*WHEEL_DELTA);}unsigned sent=count?SendInput(count,events,sizeof(INPUT)):0;static uint64_t lastReport=0;if(output.button || now-lastReport>=2000){lastReport=now;ReportMenuLaser(owner,hits[0].hit?1:0,hits[1].hit?1:0,triggers[0],triggers[1],output.button,sent,count);}return true;
}
}
void PublishControllerMenuSurface(const float* transform,const float* size,float radius){for(unsigned i=0;i<8;++i)if(!std::isfinite(transform[i]))return;float n=0;for(unsigned i=4;i<8;++i)n+=transform[i]*transform[i];if(n<.98f||n>1.02f||!std::isfinite(radius)||!std::isfinite(size[0])||!std::isfinite(size[1])||size[0]<=0||size[1]<=0||size[0]>20||size[1]>20)return;std::unique_lock lock(guard,std::try_to_lock);if(!lock.owns_lock())return;menuSurface={{transform[0],transform[1],transform[2]},{transform[4],transform[5],transform[6],transform[7]},size[0],size[1],radius};menuSurfaceTick=GetTickCount64();}

// Menu input is exclusively laser-based; grip is reserved for right click/drag.
void PollControllerMenuMouse(bool menu){
 if(menu){std::lock_guard setupLock(guard);CancelResetDialog();SubmitSetupBoxes(false);}std::lock_guard lock(guard);PollControllerShortcuts(true);PollMenuLaser(menu);}

void PlayVRRecenterFeedback(){
 HMODULE module=nullptr;wchar_t path[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&PlayVRRecenterFeedback),&module)||!GetModuleFileNameW(module,path,32768))return;
 auto sound=std::filesystem::path(path).parent_path()/"sounds"/"controller_vr_recenter.wav";
 PlaySoundW(sound.c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);
}

void SetControllerVehicleIdentity(const char* brand,const char* model){std::lock_guard lock(guard);VehicleBoxIdentity identity{brand?brand:"",model?model:""};if(identity!=desiredBoxVehicle){cameraSeatDrag.Cancel();cameraPositionPending=false;CancelResetDialog();steeringHandDrag.Release();if(!SaveSteeringPoseEdit())return;actualSceneFrame.ResetVehicleCamera();sceneCameraTick=0;vehicleSteeringPose={};}desiredBoxVehicle=identity.Valid()?identity:VehicleBoxIdentity{};}

VehicleSteeringPose SnapshotVehicleSteeringPose(){std::lock_guard lock(guard);return vehicleSteeringPose.settings;}

bool ConsumeControllerDisplayRequest(unsigned action){std::lock_guard lock(guard);if(action>=3)return false;auto event=displayRequestTicks[action];displayRequestTicks[action]=0;return event&&GetTickCount64()-event<250;}

bool ConsumeControllerCameraPosition(DriverIK::V& position){std::lock_guard lock(guard);if(!cameraPositionPending||activeBoxVehicle!=desiredBoxVehicle)return false;position=pendingCameraPosition;cameraPositionPending=false;return true;}
