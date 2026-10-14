#include <Windows.h>
#include "ControllerRuntime.hpp"
#include "ControllerFingers.hpp"
#include "LiveControllerFingers.hpp"
#include "ControllerFingerMode.hpp"
#include "ControllerSceneFrame.hpp"
#include "IgnitionInteraction.hpp"
#include "InteractionBoxEditor.hpp"
#include "NativeControllerInput.hpp"
#include "vendor/openvr.h"
#include <mutex>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
namespace {
std::mutex guard;HMODULE library=nullptr;vr::IVRSystem* systemVR=nullptr;bool initialized=false;
using Init=uint32_t (__cdecl*)(vr::EVRInitError*,vr::EVRApplicationType,const char*);
using Interface=void* (__cdecl*)(const char*,vr::EVRInitError*);
using Shutdown=void (__cdecl*)();Shutdown shutdownVR=nullptr;
struct Tracked {DriverIK::V point{};DriverIK::Q rotation{1,0,0,0};bool valid=false;};
LiveControllerFingerPose liveFingers[2]{LiveControllerFingerPose(false),LiveControllerFingerPose(true)};
ControllerFingerMode fingerMode[2];
DriverIK::V hornTarget{},hornBoxOffset{},leftHandBoxOffset{.14f,0,0};uint64_t hornTargetTick=0,hornBoxTick=0;bool hornBoxHeld=false;float hornBoxScale=1.25f;InteractionBoxEditor boxEditor;std::string previousBoxPlacement;uint64_t placementPoll=0;IgnitionTriggerGate ignitionTrigger;DriverIK::V ignitionTarget{},ignitionBoxOffset{},handBoxOffset{-.08f,0,0};uint64_t ignitionTargetTick=0,interactionRequestTick=0;ControllerSceneFrame actualSceneFrame{};uint64_t sceneXRFrameTick=0,sceneCameraTick=0;DriverPalette sceneMarkers{};uint64_t sceneMarkerTick=0;bool markerCalibrated=false;DriverIK::V markerOrigin{},markerAnchor{};DriverIK::Q markerBasis{1,0,0,0};
vr::IVROverlay* markerOverlay=nullptr;vr::VROverlayHandle_t markers[2][6]{};int markerError=0;
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
 for(unsigned side=0;side<2;++side){std::ifstream input(guideFile.parent_path()/(side?"controller_right_fingers.txt":"controller_left_fingers.txt"),std::ios::binary);if(!input)continue;std::array<char,16385> buffer{};input.read(buffer.data(),buffer.size());std::string text(buffer.data(),static_cast<size_t>(input.gcount()));if(text==previous[side])continue;previous[side]=text;std::string error;liveFingers[side].Replace(text,error);}
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
 errorCode=static_cast<int>(error);if(systemVR){markerOverlay=static_cast<vr::IVROverlay*>(get(vr::IVROverlay_Version,&error));CreateMarkers();return true;}
 if(initialized && shutdownVR)shutdownVR();initialized=false;FreeLibrary(library);library=nullptr;return false;
}
}
void PollControllers(bool active,uint64_t generation){
 PollFingerFiles();
 std::lock_guard lock(guard);uint64_t now=GetTickCount64();float dt=tick?std::min(.05f,(now-tick)*.001f):0;tick=now;
 if(generationSeen!=generation){generationSeen=generation;boxEditor.Release();boxEditor.enabled=false;markerCalibrated=false;ignitionTrigger={};interactionRequestTick=ignitionTargetTick=0;for(unsigned i=0;i<2;++i){controls[i]={};fingerMode[i]={};weights[i]=0;fingerGrip[i]=fingerTrigger[i]=fingerThumb[i]=0;lastSolveTick[i]=0;}}
 if(!systemVR && now-attempt>=5000){attempt=now;Connect();}
 if(!guideFile.empty() && now-guidePoll>=500){guidePoll=now;std::ifstream input(guideFile);if(input){std::string text((std::istreambuf_iterator<char>(input)),{});ControllerElbowGuides parsed;if(ParseControllerElbowGuides(text,parsed))elbowGuides=parsed;}std::ifstream twistInput(twistFile);float value;std::string extra;if(twistInput>>value && !(twistInput>>extra) && std::isfinite(value) && value>=0 && value<=1)twistShare=value;std::ifstream boxInput(guideFile.parent_path()/"ignition_box_offset.txt");DriverIK::V boxOffset{};if(boxInput>>boxOffset[0]>>boxOffset[1]>>boxOffset[2] && !(boxInput>>extra) && std::isfinite(boxOffset[0]) && std::isfinite(boxOffset[1]) && std::isfinite(boxOffset[2]))ignitionBoxOffset=boxOffset;std::ifstream leftBoxInput(guideFile.parent_path()/"horn_left_hand_box_offset.txt");DriverIK::V leftOffset{};if(leftBoxInput>>leftOffset[0]>>leftOffset[1]>>leftOffset[2] && !(leftBoxInput>>extra) && std::isfinite(leftOffset[0]) && std::isfinite(leftOffset[1]) && std::isfinite(leftOffset[2]))leftHandBoxOffset=leftOffset;std::ifstream hornScaleInput(guideFile.parent_path()/"horn_box_scale.txt");float scaleValue=0;if(hornScaleInput>>scaleValue && !(hornScaleInput>>extra) && std::isfinite(scaleValue) && scaleValue>0)hornBoxScale=scaleValue;std::ifstream hornBoxInput(guideFile.parent_path()/"horn_box_offset.txt");DriverIK::V hornOffset{};if(hornBoxInput>>hornOffset[0]>>hornOffset[1]>>hornOffset[2] && !(hornBoxInput>>extra) && std::isfinite(hornOffset[0]) && std::isfinite(hornOffset[1]) && std::isfinite(hornOffset[2]))hornBoxOffset=hornOffset;std::ifstream handBoxInput(guideFile.parent_path()/"ignition_hand_box_offset.txt");DriverIK::V handOffset{};if(handBoxInput>>handOffset[0]>>handOffset[1]>>handOffset[2] && !(handBoxInput>>extra) && std::isfinite(handOffset[0]) && std::isfinite(handOffset[1]) && std::isfinite(handOffset[2]))handBoxOffset=handOffset;std::ifstream blendInput(modeBlendFile);if(blendInput>>value && !(blendInput>>extra) && std::isfinite(value) && value>0)modeBlendSeconds=value;}

 if(!guideFile.empty() && now-placementPoll>=500){placementPoll=now;std::ifstream input(guideFile.parent_path()/"interaction_boxes.txt");if(input && boxEditor.selected<0){std::string text((std::istreambuf_iterator<char>(input)),{});if(text!=previousBoxPlacement && text.size()<4096){if(ParseInteractionBoxes(text,boxEditor))previousBoxPlacement=text;}}}
 bool nativePressed[2]{},modePressed[2]{},thumbPressed[2]{};float grips[2]{},triggers[2]{},inward[2]{};bool nativeValid=active && PollNativeControllerButtons(nativePressed,grips,triggers,modePressed,thumbPressed,inward);
 if(!active)HideMarkers();head.valid=false;for(auto& h:hands)h.valid=false;
 if(systemVR && active){vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount]{};systemVR->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding,0,poses,vr::k_unMaxTrackedDeviceCount);auto observedHead=Convert(poses[0]);if(observedHead.valid)head=observedHead;
  for(unsigned i=0;i<2;++i){auto index=systemVR->GetTrackedDeviceIndexForControllerRole(i?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand);bool pressed=false;
   if(index<vr::k_unMaxTrackedDeviceCount){auto observedHand=Convert(poses[index]);if(observedHand.valid)hands[i]=observedHand;vr::VRControllerState_t state{};if(systemVR->GetControllerState(index,&state,sizeof(state)))pressed=(state.ulButtonPressed&vr::ButtonMaskFromId(vr::k_EButton_Axis0))!=0;}
   bool valid=hands[i].valid && head.valid;bool wasEnabled=controls[i].enabled;controls[i].Button(nativeValid?nativePressed[i]:pressed,now,valid);if(controls[i].enabled!=wasEnabled){ControllerToggleFeedback(i);PlaySoundW((guideFile.parent_path()/(controls[i].enabled?"controller_hand_engaged.wav":"controller_hand_disengaged.wav")).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}auto fingers=liveFingers[i].Snapshot();auto priorSwitches=fingerMode[i].switches;fingerMode[i].Step(modePressed[i],valid && nativeValid,liveFingers[i].Revision(),*fingers);if(fingerMode[i].switches!=priorSwitches){ControllerToggleFeedback(i);auto name=std::string("controller_")+(i?"right_":"left_")+fingerMode[i].SpokenFinger()+".wav";PlaySoundW((guideFile.parent_path()/name).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}if(fingerMode[i].Toggle(inward[i],valid && nativeValid)){ControllerToggleFeedback(i);PlaySoundW((guideFile.parent_path()/(fingerMode[i].Selected()?"controller_finger_added.wav":"controller_finger_removed.wav")).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}if(valid)++samples[i];
   if(markerOverlay)for(auto handle:markers[i])if(handle)markerOverlay->HideOverlay(handle);

  }
 }
 for(unsigned i=0;i<2;++i){auto currentFingers=liveFingers[i].Snapshot();fingerMode[i].Advance(*currentFingers,dt,modeBlendSeconds);float desired=active && hands[i].valid && head.valid && controls[i].enabled?1.f:0.f;weights[i]+=std::clamp(desired-weights[i],-dt/.18f,dt/.18f);float alpha=1-std::exp(-dt/.04f);fingerGrip[i]+=(grips[i]-fingerGrip[i])*alpha;fingerTrigger[i]+=(triggers[i]-fingerTrigger[i])*alpha;fingerThumb[i]+=((thumbPressed[i]?1.f:0.f)-fingerThumb[i])*alpha;}
 BoxGrabInput boxInputs[2]{};bool sceneFresh=active && head.valid && sceneXRFrameTick && now-sceneXRFrameTick<250 && sceneCameraTick && now-sceneCameraTick<250;
 for(unsigned i=0;i<2;++i)if(sceneFresh && hands[i].valid){boxInputs[i]={actualSceneFrame.Position(hands[i].point,head.point,head.rotation),actualSceneFrame.Orientation(hands[i].rotation,head.rotation),true,grips[i]};}
 boxEditor.Step(boxInputs);
 if(boxEditor.dirty && !guideFile.empty()){
  auto text=SerializeInteractionBoxes(boxEditor);auto target=guideFile.parent_path()/"interaction_boxes.txt";auto temp=target;temp+=L".tmp";bool written=false;{std::ofstream output(temp,std::ios::binary|std::ios::trunc);if(output){output<<text;output.flush();written=output.good();}}
  if(written && MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){boxEditor.dirty=false;previousBoxPlacement=text;}
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
 sceneMarkers={};sceneMarkerTick=GetTickCount64();hornBoxHeld=false;hornBoxTick=sceneMarkerTick;
 if(!boxEditor.authored[0]){boxEditor.boxes[0].center=ignitionTarget;boxEditor.boxes[0].scale=1;}
 if(!boxEditor.authored[1]){boxEditor.boxes[1].center=hornTarget;boxEditor.boxes[1].scale=hornBoxScale;}
 auto keyBox=boxEditor.boxes[0],hornBox=boxEditor.boxes[1];
 auto drawBox=[&](unsigned bone,const InteractionBox& box){auto& m=sceneMarkers[bone];for(unsigned col=0;col<3;++col){DriverIK::V axis{};axis[col]=box.scale*box.stretch[col];auto v=DriverIK::Rotate(box.rotation,axis);for(unsigned row=0;row<3;++row)m[row*4+col]=v[row];}m[3]=box.center[0];m[7]=box.center[1];m[11]=box.center[2];m[15]=1;};
 if(boxEditor.enabled)drawBox(44,keyBox);
 auto handWorld=DriverIK::FK(pose,positions);bool keyFresh=ignitionTargetTick && sceneMarkerTick-ignitionTargetTick<250;
 bool activeRight=controls[1].enabled && hands[1].valid && head.valid && (mask&2);
 auto handBoxCenter=DriverIK::Add(handWorld.p[28],DriverIK::Rotate(handWorld.q[28],handBoxOffset));
 if(boxEditor.enabled && activeRight){auto& m=sceneMarkers[48];for(unsigned col=0;col<3;++col){DriverIK::V a{};a[col]=1;auto v=DriverIK::Rotate(handWorld.q[28],a);for(unsigned row=0;row<3;++row)m[row*4+col]=v[row];}m[3]=handBoxCenter[0];m[7]=handBoxCenter[1];m[11]=handBoxCenter[2];m[15]=1;}
 bool hornFresh=hornTargetTick && sceneMarkerTick-hornTargetTick<250;
 if(boxEditor.enabled)drawBox(49,hornBox);
 for(unsigned side=0;side<2;++side){unsigned wrist=side?28:7;bool active=controls[side].enabled && hands[side].valid && head.valid && (mask&(1u<<side));auto offset=side?handBoxOffset:leftHandBoxOffset;auto center=DriverIK::Add(handWorld.p[wrist],DriverIK::Rotate(handWorld.q[wrist],offset));
  if(boxEditor.enabled && !side && active){auto& m=sceneMarkers[45];for(unsigned col=0;col<3;++col){DriverIK::V a{};a[col]=1;auto v=DriverIK::Rotate(handWorld.q[wrist],a);for(unsigned row=0;row<3;++row)m[row*4+col]=v[row];}m[3]=center[0];m[7]=center[1];m[11]=center[2];m[15]=1;}
  if(!boxEditor.enabled && active && hornFresh && IgnitionBoxesOverlap(hornBox.center,DriverIK::V{hornBox.half[0]*hornBox.scale*hornBox.stretch[0],hornBox.half[1]*hornBox.scale*hornBox.stretch[1],hornBox.half[2]*hornBox.scale*hornBox.stretch[2]},center,handWorld.q[wrist],{.04f,.035f,.055f},hornBox.rotation))hornBoxHeld=true;
 }
 bool overlap=activeRight && keyFresh && IgnitionBoxesOverlap(keyBox.center,DriverIK::V{keyBox.half[0]*keyBox.scale*keyBox.stretch[0],keyBox.half[1]*keyBox.scale*keyBox.stretch[1],keyBox.half[2]*keyBox.scale*keyBox.stretch[2]},handBoxCenter,handWorld.q[28],{.04f,.035f,.055f},keyBox.rotation);
 if(ignitionTrigger.Step(fingerTrigger[1],activeRight && keyFresh && !boxEditor.enabled,overlap)){interactionRequestTick=sceneMarkerTick;ControllerToggleFeedback(1);}

 if(head.valid && sceneXRFrameTick && sceneMarkerTick-sceneXRFrameTick<250 && sceneCameraTick && sceneMarkerTick-sceneCameraTick<250){
 for(unsigned side=0;side<2;++side)if(hands[side].valid && !controls[side].enabled){auto position=actualSceneFrame.Position(hands[side].point,head.point,head.rotation);auto rotation=actualSceneFrame.Orientation(hands[side].rotation,head.rotation);
  auto& m=sceneMarkers[side?28:7];for(unsigned col=0;col<3;++col){DriverIK::V axis{};axis[col]=1;auto v=DriverIK::Rotate(rotation,axis);for(unsigned row=0;row<3;++row)m[row*4+col]=v[row];}m[3]=position[0];m[7]=position[1];m[11]=position[2];m[15]=1;}
 }

 return mask;
}
void ControllerStatus(char* buffer,size_t length){std::lock_guard lock(guard);std::snprintf(buffer,length,"CONTROLLER_IK connected=%d error=%d marker_error=%d native_input=%d twist_share=%.3f grip=%.3f,%.3f trigger=%.3f,%.3f left_valid=%d right_valid=%d left_enabled=%d right_enabled=%d samples=%llu,%llu solves=%llu,%llu rejected=%llu,%llu calibrations=%llu,%llu finger_mode=%s,%s mode_switches=%llu,%llu finger_pose_revision=%llu,%llu finger_pose_rejected=%llu,%llu",systemVR?1:0,errorCode,markerError,NativeControllerInputStatus(),twistShare,fingerGrip[0],fingerGrip[1],fingerTrigger[0],fingerTrigger[1],hands[0].valid?1:0,hands[1].valid?1:0,controls[0].enabled?1:0,controls[1].enabled?1:0,samples[0],samples[1],solves[0],solves[1],rejects[0],rejects[1],calibrations[0],calibrations[1],fingerMode[0].Name(),fingerMode[1].Name(),fingerMode[0].switches,fingerMode[1].switches,liveFingers[0].Revision(),liveFingers[1].Revision(),liveFingers[0].Rejected(),liveFingers[1].Rejected());}
void CloseControllers(){std::lock_guard lock(guard);HideMarkers();if(markerOverlay)for(auto& side:markers)for(auto handle:side)if(handle)markerOverlay->DestroyOverlay(handle);for(auto& side:markers)for(auto& handle:side)handle=0;markerOverlay=nullptr;systemVR=nullptr;if(initialized && shutdownVR)shutdownVR();initialized=false;if(library)FreeLibrary(library);library=nullptr;}

bool ApplyControllerMarkerPalette(DriverPalette& palette){std::lock_guard lock(guard);palette={};if(sceneMarkerTick && GetTickCount64()-sceneMarkerTick<=250)palette=sceneMarkers;return true;}

void ResetControllerMarkerFrame(){std::lock_guard lock(guard);markerCalibrated=false;}

void SetControllerSceneXRFrame(const float* reference,const float* current){std::lock_guard lock(guard);actualSceneFrame.neutral={reference[0],reference[1],reference[2]};actualSceneFrame.neutralQ={reference[4],reference[5],reference[6],reference[7]};actualSceneFrame.xrHead={current[0],current[1],current[2]};actualSceneFrame.xrHeadQ={current[4],current[5],current[6],current[7]};sceneXRFrameTick=GetTickCount64();}
void SetControllerSceneCamera(float x,float y,float z){if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))return;std::lock_guard lock(guard);actualSceneFrame.camera={-.3847368658f+x,1.066045642f+y,.1469421685f+z};sceneCameraTick=GetTickCount64();}

void SetIgnitionInteractionTarget(DriverIK::V point){for(float v:point)if(!std::isfinite(v))return;std::lock_guard lock(guard);ignitionTarget=DriverIK::Add(point,ignitionBoxOffset);ignitionTargetTick=GetTickCount64();}
bool ConsumeControllerIgnitionRequest(){std::lock_guard lock(guard);auto event=interactionRequestTick;interactionRequestTick=0;return event && GetTickCount64()-event<250;}

void SetHornInteractionTarget(DriverIK::V point){for(float v:point)if(!std::isfinite(v))return;std::lock_guard lock(guard);hornTarget=DriverIK::Add(point,hornBoxOffset);hornTargetTick=GetTickCount64();}
bool ControllerHornBoxHeld(){std::lock_guard lock(guard);return hornBoxHeld && hornBoxTick && GetTickCount64()-hornBoxTick<250;}

int ControllerBoxSetupKey(bool held){std::lock_guard lock(guard);return boxEditor.Key(held);}
