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
extern "C" void ReportMenuModel(unsigned side,const char* name,unsigned part,int status) noexcept;
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
namespace {
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
DriverIK::V hornTarget{},hornBoxOffset{},leftHandBoxOffset{.14f,0,0};uint64_t hornTargetTick=0,hornBoxTick=0;bool hornBoxHeld=false;float hornBoxScale=1.25f;InteractionBoxEditor boxEditor;std::string previousBoxPlacement;uint64_t placementPoll=0;IgnitionTriggerGate radioTriggers[2];IgnitionTriggerGate wiperTriggers[2];uint64_t wiperRequestTick=0;IgnitionTriggerGate lightTriggers[2];uint64_t lightRequestTick=0;IgnitionTriggerGate ignitionTriggers[2];DriverIK::V ignitionTarget{},ignitionBoxOffset{},handBoxOffset{-.08f,0,0};uint64_t ignitionTargetTick=0,interactionRequestTick=0;ControllerSceneFrame actualSceneFrame{};uint64_t sceneXRFrameTick=0,sceneCameraTick=0;DriverPalette sceneMarkers{};uint64_t sceneMarkerTick=0;bool markerCalibrated=false;DriverIK::V markerOrigin{},markerAnchor{};DriverIK::Q markerBasis{1,0,0,0};
vr::IVRRenderModels* controllerModels=nullptr;vr::IVROverlay* markerOverlay=nullptr;vr::VROverlayHandle_t markers[2][6]{};int markerError=0;
void DestroyMenuLasers();void DestroyMenuControllerModels();void HideMenuControllerModels();void UpdateCabinControllerModels(const vr::TrackedDevicePose_t* poses);
bool cabinModelsActive=false,cabinModelReady[2]{};Tracked controllerAim[2];float cabinLaserDistance[2]{2,2};bool cabinLaserHit[2]{},cabinLaserReady[2]{};uint64_t cabinLaserTick[2]{};
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
void PollFingerFiles(){
 static uint64_t lastPoll=0;static std::string previous[2];auto now=GetTickCount64();if(guideFile.empty() || now-lastPoll<500)return;lastPoll=now;
 for(unsigned side=0;side<2;++side){auto input=OpenDriverSetting(guideFile.parent_path()/(side?"controller_right_fingers.txt":"controller_left_fingers.txt"),std::ios::binary);if(!input)continue;std::array<char,16385> buffer{};input.read(buffer.data(),buffer.size());std::string text(buffer.data(),static_cast<size_t>(input.gcount()));if(text==previous[side])continue;previous[side]=text;std::string error;liveFingers[side].Replace(text,error);}
}
Tracked Convert(const vr::TrackedDevicePose_t& pose){
 Tracked out;if(!pose.bDeviceIsConnected || !pose.bPoseIsValid || pose.eTrackingResult!=vr::TrackingResult_Running_OK)return out;
 const auto& m=pose.mDeviceToAbsoluteTracking.m;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)if(!std::isfinite(m[r][c]))return out;
 out.point={m[0][3],m[1][3],m[2][3]};float trace=m[0][0]+m[1][1]+m[2][2];auto& q=out.rotation;
 if(trace>0){float s=std::sqrt(trace+1)*2;q={s/4,(m[2][1]-m[1][2])/s,(m[0][2]-m[2][0])/s,(m[1][0]-m[0][1])/s};}
 else {unsigned i=m[1][1]>m[0][0]?1:0;if(m[2][2]>m[i][i])i=2;unsigned j=(i+1)%3,k=(i+2)%3;float s=std::sqrt(std::max(0.f,1+m[i][i]-m[j][j]-m[k][k]))*2;if(s<1e-6f)return out;q[0]=(m[k][j]-m[j][k])/s;q[i+1]=s/4;q[j+1]=(m[j][i]+m[i][j])/s;q[k+1]=(m[k][i]+m[i][k])/s;}
 float n=0;for(float x:q)n+=x*x;if(n<.98f || n>1.02f)return out;for(float& x:q)x/=std::sqrt(n);out.valid=true;return out;
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
void PollControllers(bool active,uint64_t generation){
 PollFingerFiles();
 std::lock_guard lock(guard);cabinModelsActive=active;cabinModelReady[0]=cabinModelReady[1]=false;controllerAim[0]=controllerAim[1]={};uint64_t now=GetTickCount64();float dt=tick?std::min(.05f,(now-tick)*.001f):0;tick=now;
 if(generationSeen!=generation){generationSeen=generation;radioClickPending=0;radioTunePending=false;radioStick={};radioCommand={};radioTick=0;radioTriggers[0]=radioTriggers[1]={};boxEditor.Release();boxEditor.enabled=false;markerCalibrated=false;ignitionTriggers[0]=ignitionTriggers[1]={};lightTriggers[0]=lightTriggers[1]={};wiperTriggers[0]=wiperTriggers[1]={};wiperRequestTick=0;lightRequestTick=0;interactionRequestTick=ignitionTargetTick=0;for(unsigned i=0;i<2;++i){controls[i]={};fingerMode[i]={};weights[i]=0;fingerGrip[i]=fingerTrigger[i]=fingerThumb[i]=0;lastSolveTick[i]=0;}}
 if(!systemVR && now-attempt>=5000){attempt=now;Connect();}
 if(!guideFile.empty() && now-guidePoll>=500){guidePoll=now;auto input=OpenDriverSetting(guideFile);if(input){std::string text((std::istreambuf_iterator<char>(input)),{});ControllerElbowGuides parsed;if(ParseControllerElbowGuides(text,parsed))elbowGuides=parsed;}auto twistInput=OpenDriverSetting(twistFile);float value;std::string extra;if(twistInput>>value && !(twistInput>>extra) && std::isfinite(value) && value>=0 && value<=1)twistShare=value;auto boxInput=OpenDriverSetting(guideFile.parent_path()/"ignition_box_offset.txt");DriverIK::V boxOffset{};if(boxInput>>boxOffset[0]>>boxOffset[1]>>boxOffset[2] && !(boxInput>>extra) && std::isfinite(boxOffset[0]) && std::isfinite(boxOffset[1]) && std::isfinite(boxOffset[2]))ignitionBoxOffset=boxOffset;auto leftBoxInput=OpenDriverSetting(guideFile.parent_path()/"horn_left_hand_box_offset.txt");DriverIK::V leftOffset{};if(leftBoxInput>>leftOffset[0]>>leftOffset[1]>>leftOffset[2] && !(leftBoxInput>>extra) && std::isfinite(leftOffset[0]) && std::isfinite(leftOffset[1]) && std::isfinite(leftOffset[2]))leftHandBoxOffset=leftOffset;auto hornScaleInput=OpenDriverSetting(guideFile.parent_path()/"horn_box_scale.txt");float scaleValue=0;if(hornScaleInput>>scaleValue && !(hornScaleInput>>extra) && std::isfinite(scaleValue) && scaleValue>0)hornBoxScale=scaleValue;auto hornBoxInput=OpenDriverSetting(guideFile.parent_path()/"horn_box_offset.txt");DriverIK::V hornOffset{};if(hornBoxInput>>hornOffset[0]>>hornOffset[1]>>hornOffset[2] && !(hornBoxInput>>extra) && std::isfinite(hornOffset[0]) && std::isfinite(hornOffset[1]) && std::isfinite(hornOffset[2]))hornBoxOffset=hornOffset;auto handBoxInput=OpenDriverSetting(guideFile.parent_path()/"ignition_hand_box_offset.txt");DriverIK::V handOffset{};if(handBoxInput>>handOffset[0]>>handOffset[1]>>handOffset[2] && !(handBoxInput>>extra) && std::isfinite(handOffset[0]) && std::isfinite(handOffset[1]) && std::isfinite(handOffset[2]))handBoxOffset=handOffset;auto blendInput=OpenDriverSetting(modeBlendFile);if(blendInput>>value && !(blendInput>>extra) && std::isfinite(value) && value>0)modeBlendSeconds=value;}

 if(!guideFile.empty() && now-placementPoll>=500){placementPoll=now;auto input=OpenDriverSetting(guideFile.parent_path()/"interaction_boxes.txt");if(input && boxEditor.selected<0){std::string text((std::istreambuf_iterator<char>(input)),{});if(text!=previousBoxPlacement && text.size()<4096){if(ParseInteractionBoxes(text,boxEditor))previousBoxPlacement=text;}}}
 if(radioClickPending && !guideFile.empty()){auto sound=radioClickPending>0?"controller_radio_on.wav":"controller_radio_off.wav";radioClickPending=0;PlaySoundW((guideFile.parent_path().parent_path()/"sounds"/sound).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}
 if(radioTunePending && !guideFile.empty()){radioTunePending=false;PlaySoundW((guideFile.parent_path().parent_path()/"sounds"/"controller_radio_tune.wav").c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}
 bool nativePressed[2]{},modePressed[2]{},thumbPressed[2]{};float grips[2]{},triggers[2]{},inward[2]{};bool nativeValid=active && PollNativeControllerButtons(nativePressed,grips,triggers,modePressed,thumbPressed,inward);float sticks[4]{};bool sticksValid=nativeValid && ReadControllerStickAxes(sticks);bool mirrorFresh=active && mirrorContactTick && now-mirrorContactTick<150 && !boxEditor.enabled;windowRequests[0]=windowRequests[1]=0;windowRequestTick=now;int radioOwner=-1;int mirrorOwner=-1;for(unsigned side=0;side<2;++side)if(mirrorFresh && mirrorContact[side]>=0){inward[side]=0;if(mirrorContact[side]==9 && sticksValid && (radioOwner<0 || std::hypot(sticks[side*2],sticks[side*2+1])>std::hypot(sticks[radioOwner*2],sticks[radioOwner*2+1])))radioOwner=side;if(mirrorContact[side]>=7)continue;if(mirrorContact[side]>=4 && mirrorContact[side]<=5){if(sticksValid){auto& value=windowRequests[mirrorContact[side]-4];if(std::abs(sticks[side*2+1])>std::abs(value))value=sticks[side*2+1];}continue;}if(sticksValid && (mirrorOwner<0 || std::hypot(sticks[side*2],sticks[side*2+1])>std::hypot(sticks[mirrorOwner*2],sticks[mirrorOwner*2+1])))mirrorOwner=side;}if(radioOwner<0)radioCommand.station=0;auto radioInput=radioStick.Step(radioOwner<0?0:sticks[radioOwner*2],radioOwner<0?0:sticks[radioOwner*2+1],radioOwner>=0);if(radioInput.station)radioCommand.station=radioInput.station;radioCommand.volume=radioInput.volume;radioTick=now;SendMirrorInput(mirrorOwner<0?-1:mirrorContact[mirrorOwner],mirrorOwner<0?0:sticks[mirrorOwner*2],mirrorOwner<0?0:sticks[mirrorOwner*2+1]);
 if(!active)HideMarkers();head.valid=false;for(auto& h:hands)h.valid=false;
 if(systemVR && active){vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount]{};systemVR->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding,0,poses,vr::k_unMaxTrackedDeviceCount);auto observedHead=Convert(poses[0]);if(observedHead.valid)head=observedHead;
  for(unsigned i=0;i<2;++i){auto index=systemVR->GetTrackedDeviceIndexForControllerRole(i?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand);bool pressed=false;
   if(index<vr::k_unMaxTrackedDeviceCount){auto observedHand=Convert(poses[index]);if(observedHand.valid)hands[i]=observedHand;vr::VRControllerState_t state{};if(systemVR->GetControllerState(index,&state,sizeof(state)))pressed=(state.ulButtonPressed&vr::ButtonMaskFromId(vr::k_EButton_Axis0))!=0;}
   bool valid=hands[i].valid && head.valid;bool wasEnabled=controls[i].enabled;controls[i].Button(nativeValid?nativePressed[i]:pressed,now,valid);if(controls[i].enabled!=wasEnabled){ControllerToggleFeedback(i);PlaySoundW((guideFile.parent_path().parent_path()/"sounds"/(controls[i].enabled?"controller_hand_engaged.wav":"controller_hand_disengaged.wav")).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}auto fingers=liveFingers[i].Snapshot();auto priorSwitches=fingerMode[i].switches;fingerMode[i].Step(modePressed[i],valid && nativeValid,liveFingers[i].Revision(),*fingers);if(fingerMode[i].switches!=priorSwitches){ControllerToggleFeedback(i);auto name=std::string("controller_")+(i?"right_":"left_")+fingerMode[i].SpokenFinger()+".wav";PlaySoundW((guideFile.parent_path().parent_path()/"sounds"/name).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}if(fingerMode[i].Toggle(inward[i],valid && nativeValid)){ControllerToggleFeedback(i);PlaySoundW((guideFile.parent_path().parent_path()/"sounds"/(fingerMode[i].Selected()?"controller_finger_added.wav":"controller_finger_removed.wav")).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}if(valid)++samples[i];
   if(markerOverlay)for(auto handle:markers[i])if(handle)markerOverlay->HideOverlay(handle);

  }
  UpdateCabinControllerModels(poses);
 }
 for(unsigned i=0;i<2;++i){auto currentFingers=liveFingers[i].Snapshot();fingerMode[i].Advance(*currentFingers,dt,modeBlendSeconds);float desired=active && hands[i].valid && head.valid && controls[i].enabled?1.f:0.f;weights[i]+=std::clamp(desired-weights[i],-dt/.18f,dt/.18f);float alpha=1-std::exp(-dt/.04f);fingerGrip[i]+=(grips[i]-fingerGrip[i])*alpha;fingerTrigger[i]+=(triggers[i]-fingerTrigger[i])*alpha;fingerThumb[i]+=((thumbPressed[i]?1.f:0.f)-fingerThumb[i])*alpha;}
 BoxGrabInput boxInputs[2]{};bool sceneFresh=active && head.valid && sceneXRFrameTick && now-sceneXRFrameTick<250 && sceneCameraTick && now-sceneCameraTick<250;
 for(unsigned i=0;i<2;++i)if(sceneFresh && hands[i].valid){boxInputs[i]={actualSceneFrame.Position(hands[i].point,head.point,head.rotation),actualSceneFrame.Orientation(hands[i].rotation,head.rotation),true,grips[i]};}
 boxEditor.Step(boxInputs);
 if(boxEditor.dirty && !guideFile.empty()){
  auto text=SerializeInteractionBoxes(boxEditor);
  if(WriteDriverSetting(guideFile.parent_path(),"interaction_boxes",text)){boxEditor.dirty=false;previousBoxPlacement=text;}

 }

}
unsigned ApplyControllers(DriverIK::Pose& pose,const DriverIK::Positions& positions,bool allowLeft,bool allowRight){
 std::lock_guard lock(guard);if(!tick || GetTickCount64()-tick>250)return 0;unsigned mask=0;

 for(unsigned i=0;i<2;++i){if(!(i?allowRight:allowLeft) || weights[i]<=0 || (controls[i].calibrate && (!hands[i].valid || !head.valid)))continue;
  if(!sceneXRFrameTick || GetTickCount64()-sceneXRFrameTick>250 || !sceneCameraTick || GetTickCount64()-sceneCameraTick>250)continue;
  auto temporary=controls[i];temporary.enabled=true;bool calibration=temporary.calibrate;auto now=GetTickCount64();float seconds=lastSolveTick[i]?std::min(.05f,(now-lastSolveTick[i])*.001f):1.f/90.f;
  auto controllerPoint=actualSceneFrame.Position(hands[i].point,head.point,head.rotation);auto controllerRotation=actualSceneFrame.Orientation(hands[i].rotation,head.rotation);
  if(ApplyControllerHand(pose,positions,i!=0,temporary,controllerPoint,controllerRotation,{1,0,0,0},weights[i],seconds,elbowGuides.values[i],twistShare,true)){auto fingers=liveFingers[i].Snapshot();auto selected=fingerMode[i].Targets(*fingers);BlendControllerFingers(pose,i!=0,fingerGrip[i],fingerTrigger[i],weights[i],&selected,&fingerMode[i].weights,fingerThumb[i],static_cast<int>(fingerMode[i].finger),std::sin(3.14159265f*std::clamp(fingerMode[i].previewSeconds/.35f,0.f,1.f)));controls[i].origin=temporary.origin;controls[i].originRotation=temporary.originRotation;controls[i].anchor=temporary.anchor;controls[i].anchorRotation=temporary.anchorRotation;controls[i].basis=temporary.basis;controls[i].calibrate=temporary.calibrate;controls[i].bodyOrientation=temporary.bodyOrientation;controls[i].priorDirection=temporary.priorDirection;controls[i].priorBend=temporary.priorBend;controls[i].haveBend=temporary.haveBend;controls[i].neutralWrist=temporary.neutralWrist;controls[i].previousTwist=temporary.previousTwist;controls[i].haveTwist=temporary.haveTwist;lastSolveTick[i]=now;mask|=1u<<i;++solves[i];if(calibration)++calibrations[i];}else ++rejects[i];
 }
 sceneMarkers={};sceneMarkerTick=GetTickCount64();mirrorContact[0]=mirrorContact[1]=-1;mirrorContactTick=sceneMarkerTick;hornBoxHeld=false;hornBoxTick=sceneMarkerTick;
 if(!boxEditor.authored[0]){boxEditor.boxes[0].center=ignitionTarget;boxEditor.boxes[0].scale=1;}
 if(!boxEditor.authored[1]){boxEditor.boxes[1].center=hornTarget;boxEditor.boxes[1].scale=hornBoxScale;}
 if(!boxEditor.authored[9]){boxEditor.boxes[9].center=boxEditor.boxes[0].center;boxEditor.boxes[9].center[0]-=.12f;}
 if(!boxEditor.authored[10]){boxEditor.boxes[10].center=boxEditor.boxes[9].center;boxEditor.boxes[10].center[1]+=.10f;}
 auto keyBox=boxEditor.boxes[0],hornBox=boxEditor.boxes[1];
 auto drawBox=[&](unsigned bone,const InteractionBox& box){auto& m=sceneMarkers[bone];for(unsigned col=0;col<3;++col){DriverIK::V axis{};axis[col]=box.scale*box.stretch[col];auto v=DriverIK::Rotate(box.rotation,axis);for(unsigned row=0;row<3;++row)m[row*4+col]=v[row];}m[3]=box.center[0];m[7]=box.center[1];m[11]=box.center[2];m[15]=1;};
 if(boxEditor.enabled){drawBox(44,keyBox);for(unsigned j=2;j<InteractionBoxCount;++j)drawBox(j==11?46:j>=5?50+j:48+j,boxEditor.boxes[j]);}
 auto handWorld=DriverIK::FK(pose,positions);bool keyFresh=ignitionTargetTick && sceneMarkerTick-ignitionTargetTick<250;
 bool sceneFresh=head.valid && sceneXRFrameTick && sceneMarkerTick-sceneXRFrameTick<250 && sceneCameraTick && sceneMarkerTick-sceneCameraTick<250;
 bool hornFresh=hornTargetTick && sceneMarkerTick-hornTargetTick<250;
 if(boxEditor.enabled)drawBox(49,hornBox);
 ControllerContactVolume volumes[2]{};InteractionRayHit rayHits[2]{};
 for(unsigned side=0;side<2;++side){unsigned wrist=side?28:7;auto offset=side?handBoxOffset:leftHandBoxOffset;auto palm=DriverIK::Add(handWorld.p[wrist],DriverIK::Rotate(handWorld.q[wrist],offset));
  auto raw=actualSceneFrame.Position(hands[side].point,head.point,head.rotation);auto rawQ=actualSceneFrame.Orientation(hands[side].rotation,head.rotation);
  auto& volume=volumes[side];volume=ControllerInteractionVolume(controls[side].enabled,hands[side].valid,sceneFresh,(mask&(1u<<side))!=0,palm,handWorld.q[wrist],raw,rawQ);
  if(!volume.valid){lightTriggers[side].Step(0,false,false);wiperTriggers[side].Step(0,false,false);radioTriggers[side].Step(0,false,false);continue;}
  if(boxEditor.enabled){InteractionBox display{volume.center,{.04f,.035f,.055f},volume.rotation};drawBox(side?48:45,display);}
  if(!controls[side].enabled){
   auto aim=controllerAim[side].valid?controllerAim[side]:hands[side];auto aimOrigin=actualSceneFrame.Position(aim.point,head.point,head.rotation);auto aimQ=actualSceneFrame.Orientation(aim.rotation,head.rotation);auto direction=DriverIK::Rotate(aimQ,{0,0,-1});rayHits[side]=RayInteractionBoxes(boxEditor,aimOrigin,direction);
   cabinLaserDistance[side]=rayHits[side].distance;cabinLaserHit[side]=rayHits[side].box>=0;cabinLaserTick[side]=GetTickCount64();
   InteractionBox laser{DriverIK::Add(aimOrigin,DriverIK::Mul(direction,rayHits[side].distance*.5f)),{.0015f,.0015f,.5f},aimQ,1,{1,1,std::max(.001f,rayHits[side].distance)}};if(!cabinLaserReady[side])drawBox(53+side,laser);
  }
  const auto& lightsBox=boxEditor.boxes[9];bool lightsHit=controls[side].enabled?IgnitionBoxesOverlap(lightsBox.center,DriverIK::V{lightsBox.half[0]*lightsBox.scale*lightsBox.stretch[0],lightsBox.half[1]*lightsBox.scale*lightsBox.stretch[1],lightsBox.half[2]*lightsBox.scale*lightsBox.stretch[2]},volume.center,volume.rotation,{.04f,.035f,.055f},lightsBox.rotation):rayHits[side].box==9;
  if(lightTriggers[side].Step(fingerTrigger[side],!boxEditor.enabled && volume.valid,lightsHit)){lightRequestTick=sceneMarkerTick;ControllerToggleFeedback(side);}
  const auto& wipersBox=boxEditor.boxes[10];bool wipersHit=controls[side].enabled?IgnitionBoxesOverlap(wipersBox.center,DriverIK::V{wipersBox.half[0]*wipersBox.scale*wipersBox.stretch[0],wipersBox.half[1]*wipersBox.scale*wipersBox.stretch[1],wipersBox.half[2]*wipersBox.scale*wipersBox.stretch[2]},volume.center,volume.rotation,{.04f,.035f,.055f},wipersBox.rotation):rayHits[side].box==10;
  if(wiperTriggers[side].Step(fingerTrigger[side],!boxEditor.enabled && volume.valid,wipersHit)){wiperRequestTick=sceneMarkerTick;ControllerToggleFeedback(side);}
  const auto& radioBox=boxEditor.boxes[11];bool radioHit=controls[side].enabled?IgnitionBoxesOverlap(radioBox.center,DriverIK::V{radioBox.half[0]*radioBox.scale*radioBox.stretch[0],radioBox.half[1]*radioBox.scale*radioBox.stretch[1],radioBox.half[2]*radioBox.scale*radioBox.stretch[2]},volume.center,volume.rotation,{.04f,.035f,.055f},radioBox.rotation):rayHits[side].box==11;
  if(radioTriggers[side].Step(fingerTrigger[side],!boxEditor.enabled && volume.valid,radioHit)){radioCommand.toggle=true;radioTick=sceneMarkerTick;ControllerToggleFeedback(side);}
  if(!boxEditor.enabled)mirrorContact[side]=controls[side].enabled?MirrorContact(boxEditor,volume.center,volume.rotation):(rayHits[side].box>=2?rayHits[side].box-2:-1);
  if(ControllerHornPressed(!boxEditor.enabled && hornFresh && (controls[side].enabled?IgnitionBoxesOverlap(hornBox.center,DriverIK::V{hornBox.half[0]*hornBox.scale*hornBox.stretch[0],hornBox.half[1]*hornBox.scale*hornBox.stretch[1],hornBox.half[2]*hornBox.scale*hornBox.stretch[2]},volume.center,volume.rotation,{.04f,.035f,.055f},hornBox.rotation):rayHits[side].box==1),volume.valid,fingerTrigger[side]))hornBoxHeld=true;
 }
 for(unsigned side=0;side<2;++side){
 bool overlap=volumes[side].valid && keyFresh && (controls[side].enabled?IgnitionBoxesOverlap(keyBox.center,DriverIK::V{keyBox.half[0]*keyBox.scale*keyBox.stretch[0],keyBox.half[1]*keyBox.scale*keyBox.stretch[1],keyBox.half[2]*keyBox.scale*keyBox.stretch[2]},volumes[side].center,volumes[side].rotation,{.04f,.035f,.055f},keyBox.rotation):rayHits[side].box==0);
 if(ignitionTriggers[side].Step(fingerTrigger[side],volumes[side].valid && keyFresh && !boxEditor.enabled,overlap)){interactionRequestTick=sceneMarkerTick;ControllerToggleFeedback(side);}
 }

 if(head.valid && sceneXRFrameTick && sceneMarkerTick-sceneXRFrameTick<250 && sceneCameraTick && sceneMarkerTick-sceneCameraTick<250){
 for(unsigned side=0;side<2;++side)if(hands[side].valid && !controls[side].enabled && !cabinModelReady[side]){auto position=actualSceneFrame.Position(hands[side].point,head.point,head.rotation);auto rotation=actualSceneFrame.Orientation(hands[side].rotation,head.rotation);
  auto& m=sceneMarkers[side?28:7];for(unsigned col=0;col<3;++col){DriverIK::V axis{};axis[col]=1;auto v=DriverIK::Rotate(rotation,axis);for(unsigned row=0;row<3;++row)m[row*4+col]=v[row];}m[3]=position[0];m[7]=position[1];m[11]=position[2];m[15]=1;}
 }

 return mask;
}
void ControllerStatus(char* buffer,size_t length){std::lock_guard lock(guard);std::snprintf(buffer,length,"CONTROLLER_IK mirror_bridge=%d connected=%d error=%d marker_error=%d native_input=%d twist_share=%.3f grip=%.3f,%.3f trigger=%.3f,%.3f left_valid=%d right_valid=%d left_enabled=%d right_enabled=%d samples=%llu,%llu solves=%llu,%llu rejected=%llu,%llu calibrations=%llu,%llu finger_mode=%s,%s mode_switches=%llu,%llu finger_pose_revision=%llu,%llu finger_pose_rejected=%llu,%llu",mirrorBridgeStatus,systemVR?1:0,errorCode,markerError,NativeControllerInputStatus(),twistShare,fingerGrip[0],fingerGrip[1],fingerTrigger[0],fingerTrigger[1],hands[0].valid?1:0,hands[1].valid?1:0,controls[0].enabled?1:0,controls[1].enabled?1:0,samples[0],samples[1],solves[0],solves[1],rejects[0],rejects[1],calibrations[0],calibrations[1],fingerMode[0].Name(),fingerMode[1].Name(),fingerMode[0].switches,fingerMode[1].switches,liveFingers[0].Revision(),liveFingers[1].Revision(),liveFingers[0].Rejected(),liveFingers[1].Rejected());}
void CloseControllers(){
 std::lock_guard lock(guard);HideMarkers();DestroyMenuLasers();
 cabinModelsActive=false;cabinModelReady[0]=cabinModelReady[1]=false;
 // OpenVR and the game's OpenXR runtime share vrclient in this process.
 // A framework plugin toggle must not shut that client down under xrEndFrame.
 // Keep the connection and helper loaded until process exit; reactivation reuses them.
}

bool ApplyControllerMarkerPalette(DriverPalette& palette){std::lock_guard lock(guard);palette={};if(sceneMarkerTick && GetTickCount64()-sceneMarkerTick<=250)palette=sceneMarkers;return true;}

void ResetControllerMarkerFrame(){std::lock_guard lock(guard);markerCalibrated=false;}

void SetControllerSceneXRFrame(const float* reference,const float* current){std::lock_guard lock(guard);actualSceneFrame.neutral={reference[0],reference[1],reference[2]};actualSceneFrame.neutralQ={reference[4],reference[5],reference[6],reference[7]};actualSceneFrame.xrHead={current[0],current[1],current[2]};actualSceneFrame.xrHeadQ={current[4],current[5],current[6],current[7]};sceneXRFrameTick=GetTickCount64();}
void SetControllerSceneCamera(float x,float y,float z){if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))return;std::lock_guard lock(guard);actualSceneFrame.camera={-.3847368658f+x,1.066045642f+y,.1469421685f+z};sceneCameraTick=GetTickCount64();}

void SetIgnitionInteractionTarget(DriverIK::V point){for(float v:point)if(!std::isfinite(v))return;std::lock_guard lock(guard);ignitionTarget=DriverIK::Add(point,ignitionBoxOffset);ignitionTargetTick=GetTickCount64();}
bool ConsumeControllerIgnitionRequest(){std::lock_guard lock(guard);auto event=interactionRequestTick;interactionRequestTick=0;return event && GetTickCount64()-event<250;}

void SetHornInteractionTarget(DriverIK::V point){for(float v:point)if(!std::isfinite(v))return;std::lock_guard lock(guard);hornTarget=DriverIK::Add(point,hornBoxOffset);hornTargetTick=GetTickCount64();}
bool ControllerHornBoxHeld(){std::lock_guard lock(guard);return hornBoxHeld && hornBoxTick && GetTickCount64()-hornBoxTick<250;}

int ControllerBoxSetupKey(bool held){std::lock_guard lock(guard);return boxEditor.Key(held);}

float ControllerWindowDirection(unsigned side){std::lock_guard lock(guard);return side<2 && windowRequestTick && GetTickCount64()-windowRequestTick<150 && !boxEditor.enabled?windowRequests[side]:0;}

bool ConsumeControllerLightsRequest(){std::lock_guard lock(guard);auto event=lightRequestTick;lightRequestTick=0;return event && GetTickCount64()-event<250;}

bool ConsumeControllerWipersRequest(){std::lock_guard lock(guard);auto event=wiperRequestTick;wiperRequestTick=0;return event && GetTickCount64()-event<250;}


ControllerRadioCommand TakeControllerRadioCommand(){std::lock_guard lock(guard);auto out=radioCommand;radioCommand.station=0;radioCommand.toggle=false;if(!radioTick || GetTickCount64()-radioTick>150 || boxEditor.enabled)return {};return out;}


void QueueControllerRadioTuneSound(){std::lock_guard lock(guard);radioTunePending=true;}

void QueueControllerRadioClickSound(bool playing){std::lock_guard lock(guard);radioClickPending=playing?1:-1;}


namespace {MenuLaserSurface menuSurface;uint64_t menuSurfaceTick=0;vr::VROverlayHandle_t menuBeams[2]{},menuDots[2]{};
void DestroyMenuLasers(){DestroyMenuControllerModels();if(markerOverlay)for(unsigned i=0;i<2;++i){if(menuBeams[i])markerOverlay->DestroyOverlay(menuBeams[i]);if(menuDots[i])markerOverlay->DestroyOverlay(menuDots[i]);menuBeams[i]=menuDots[i]=0;}menuSurfaceTick=0;}
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
bool PollMenuLaser(bool menu){
 using namespace DriverIK;static ControllerMenuMouse click;static MenuCursorMotion cursor;static IgnitionTriggerGate choose[2];static int owner=-1;static uint64_t last=0;auto now=GetTickCount64();float dt=last?(now-last)*.001f:0;last=now;
 DWORD pid=0;auto window=GetForegroundWindow();GetWindowThreadProcessId(window,&pid);menu=menu && pid==GetCurrentProcessId();
 bool surfaceFresh=menuSurfaceTick && now-menuSurfaceTick<250;
 if(!menu || !surfaceFresh){if(!cabinModelsActive)HideMenuLasers();if(!cabinModelsActive)HideMenuControllerModels();auto out=click.Step(false,false,0,0,0,0,0);if(out.button<0){INPUT e{};e.type=INPUT_MOUSE;e.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&e,sizeof(e));}owner=-1;cursor.Step(false,-1,0,0,0,0,0);choose[0]=choose[1]={};return false;}
 if(!systemVR && now-attempt>=5000){attempt=now;Connect();}
 bool pressed[2]{},mode[2]{},thumb[2]{};float grips[2]{},triggers[2]{},inward[2]{},sticks[4]{};bool valid=systemVR && PollNativeControllerButtons(pressed,grips,triggers,mode,thumb,inward);if(valid)ReadControllerStickAxes(sticks);
 vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount]{};if(valid)systemVR->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding,0,poses,vr::k_unMaxTrackedDeviceCount);
 auto h=Convert(poses[0]);valid=valid && h.valid;MenuLaserHit hits[2]{};std::array<ControllerMeshPose,64> modelPoses{};EnsureMenuLasers();
 for(unsigned side=0;side<2;++side){auto index=valid?systemVR->GetTrackedDeviceIndexForControllerRole(side?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand):vr::k_unMaxTrackedDeviceCount;auto hand=index<vr::k_unMaxTrackedDeviceCount?MenuAimPose(index,poses[index]):Tracked{};
  if(!valid || !hand.valid){if(markerOverlay){for(auto handle:markers[side])if(handle)markerOverlay->HideOverlay(handle);if(menuBeams[side])markerOverlay->HideOverlay(menuBeams[side]);if(menuDots[side])markerOverlay->HideOverlay(menuDots[side]);}continue;}
  auto ray=MenuRayInHeadSpace(hand.point,hand.rotation,h.point,h.rotation);auto start=ray.origin;auto direction=ray.direction;hits[side]=IntersectMenuLaser(menuSurface,start,direction);float length=hits[side].hit?hits[side].distance:2.f;if(ShowMenuControllerModel(side,index,poses[index],h,modelPoses)){if(markerOverlay)for(auto handle:markers[side])if(handle)markerOverlay->HideOverlay(handle);}else ShowAxes(side,index);
  ShowControllerPointer(side,hand,h,length,hits[side].hit);
 }
 SubmitMenuModelPoses(modelPoses);
 bool select[2]{};for(unsigned side=0;side<2;++side)select[side]=choose[side].Step(triggers[side],valid,hits[side].hit);
 bool hitFlags[2]{hits[0].hit,hits[1].hit};if(!click.down && (select[0]||select[1]))click.Step(true,true,0,0,0,0,0);owner=SelectMenuLaserOwner(owner,click.down,hitFlags,select);
 bool hit=owner>=0 && hits[owner].hit;auto output=click.Step(hit,hit,0,0,hit?triggers[owner]:0,hit?sticks[owner*2+1]:0,dt);INPUT events[3]{};UINT count=0;
 static float gainX=1.5f,gainY=1.5f;static uint64_t gainPoll=0;if(now-gainPoll>=500 && !guideFile.empty()){gainPoll=now;auto gainFile=OpenDriverSetting(guideFile.parent_path()/"controller_menu_cursor_gain.txt");if(gainFile){std::string text((std::istreambuf_iterator<char>(gainFile)),{});ParseMenuCursorGain(text,gainX,gainY);}}
 RECT rect{};GetClientRect(window,&rect);auto motion=hit?cursor.Step(true,owner,hits[owner].u,hits[owner].v,rect.right,rect.bottom,dt,gainX,gainY):MenuCursorMotion::Output{};
 bool nativePositioned=hit && ApplyNativeMenuCursor(hits[owner].u,hits[owner].v);
 if(!nativePositioned && motion.anchor){POINT point{motion.x,motion.y};if(ClientToScreen(window,&point))SetCursorPos(point.x,point.y);}

 if(!nativePositioned && (motion.dx||motion.dy)){auto& e=events[count++];e.type=INPUT_MOUSE;e.mi.dx=motion.dx;e.mi.dy=motion.dy;e.mi.dwFlags=MOUSEEVENTF_MOVE;}
 if(output.button){auto& e=events[count++];e.type=INPUT_MOUSE;e.mi.dwFlags=output.button>0?MOUSEEVENTF_LEFTDOWN:MOUSEEVENTF_LEFTUP;}if(output.wheel){auto& e=events[count++];e.type=INPUT_MOUSE;e.mi.dwFlags=MOUSEEVENTF_WHEEL;e.mi.mouseData=static_cast<DWORD>(output.wheel*WHEEL_DELTA);}unsigned sent=count?SendInput(count,events,sizeof(INPUT)):0;static uint64_t lastReport=0;if(output.button || now-lastReport>=2000){lastReport=now;ReportMenuLaser(owner,hits[0].hit?1:0,hits[1].hit?1:0,triggers[0],triggers[1],output.button,sent,count);}return true;
}
}
void PublishControllerMenuSurface(const float* transform,const float* size,float radius){for(unsigned i=0;i<8;++i)if(!std::isfinite(transform[i]))return;float n=0;for(unsigned i=4;i<8;++i)n+=transform[i]*transform[i];if(n<.98f||n>1.02f||!std::isfinite(radius)||!std::isfinite(size[0])||!std::isfinite(size[1])||size[0]<=0||size[1]<=0||size[0]>20||size[1]>20)return;std::unique_lock lock(guard,std::try_to_lock);if(!lock.owns_lock())return;menuSurface={{transform[0],transform[1],transform[2]},{transform[4],transform[5],transform[6],transform[7]},size[0],size[1],radius};menuSurfaceTick=GetTickCount64();}

// Menu cursor prototype: controller rotation acts as a relative air mouse.
void PollControllerMenuMouse(bool menu){
 static ControllerMenuMouse mouse;static int owner=-1;static uint64_t last=0;static float remainderX=0,remainderY=0;
 std::lock_guard lock(guard);if(PollMenuLaser(menu)){auto release=mouse.Step(false,false,0,0,0,0,0);if(release.button<0){INPUT e{};e.type=INPUT_MOUSE;e.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&e,sizeof(e));}owner=-1;return;}auto now=GetTickCount64();float dt=last?(now-last)*.001f:0;last=now;
 DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);menu=menu && pid==GetCurrentProcessId();
 bool pressed[2]{},mode[2]{},thumb[2]{};float grip[2]{},trigger[2]{},inward[2]{},axes[4]{};
 bool valid=menu && PollNativeControllerButtons(pressed,grip,trigger,mode,thumb,inward);
 if(valid)ReadControllerStickAxes(axes);
 if(!systemVR && menu && now-attempt>=5000){attempt=now;Connect();}
 vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount]{};if(systemVR && valid)systemVR->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding,0,poses,vr::k_unMaxTrackedDeviceCount);
 if(owner<0 && valid){if(grip[1]>.6f)owner=1;else if(grip[0]>.6f)owner=0;}
 bool held=owner>=0 && valid && grip[owner]>.4f;float yaw=0,pitch=0;
 if(held && systemVR){auto index=systemVR->GetTrackedDeviceIndexForControllerRole(owner?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand);if(index>=vr::k_unMaxTrackedDeviceCount)held=false;else{auto pose=Convert(poses[index]);held=pose.valid;if(held){auto d=DriverIK::Rotate(pose.rotation,{0,0,-1});yaw=std::atan2(d[0],-d[2]);pitch=std::atan2(d[1],std::hypot(d[0],d[2]));}}}else held=false;
 auto output=mouse.Step(held,held,yaw,pitch,owner<0?0:trigger[owner],owner<0?0:axes[owner*2+1],dt);
 INPUT events[4]{};UINT count=0;
 if(held){remainderX+=output.dx;remainderY+=output.dy;LONG dx=static_cast<LONG>(remainderX),dy=static_cast<LONG>(remainderY);remainderX-=dx;remainderY-=dy;if(dx||dy){auto& e=events[count++];e.type=INPUT_MOUSE;e.mi.dx=dx;e.mi.dy=dy;e.mi.dwFlags=MOUSEEVENTF_MOVE;}}
 if(output.button){auto& e=events[count++];e.type=INPUT_MOUSE;e.mi.dwFlags=output.button>0?MOUSEEVENTF_LEFTDOWN:MOUSEEVENTF_LEFTUP;}
 if(output.wheel){auto& e=events[count++];e.type=INPUT_MOUSE;e.mi.dwFlags=MOUSEEVENTF_WHEEL;e.mi.mouseData=static_cast<DWORD>(output.wheel*WHEEL_DELTA);}
 if(count)SendInput(count,events,sizeof(INPUT));
 if(!held){owner=-1;remainderX=remainderY=0;}
}
