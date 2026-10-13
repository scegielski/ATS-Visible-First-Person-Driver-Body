#include <Windows.h>
#include "ControllerRuntime.hpp"
#include "NativeControllerInput.hpp"
#include "vendor/openvr.h"
#include <mutex>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
namespace {
std::mutex guard;HMODULE library=nullptr;vr::IVRSystem* systemVR=nullptr;bool initialized=false;
using Init=uint32_t (__cdecl*)(vr::EVRInitError*,vr::EVRApplicationType,const char*);
using Interface=void* (__cdecl*)(const char*,vr::EVRInitError*);
using Shutdown=void (__cdecl*)();Shutdown shutdownVR=nullptr;
struct Tracked {DriverIK::V point{};DriverIK::Q rotation{1,0,0,0};bool valid=false;};
ControllerElbowGuides elbowGuides;std::filesystem::path guideFile;uint64_t guidePoll=0,lastSolveTick[2]{};
Tracked head,hands[2];ControllerHand controls[2];float weights[2]{};uint64_t tick=0,generationSeen=0,attempt=0,samples[2]{},solves[2]{},rejects[2]{},calibrations[2]{};int errorCode=0;
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
 GetModuleFileNameW(self,path,MAX_PATH);guideFile=std::filesystem::path(path).parent_path()/"config"/"controller_elbow_guides.txt";wchar_t* slash=wcsrchr(path,L'\\');if(!slash)return false;wcscpy_s(slash+1,MAX_PATH-(slash+1-path),L"spf_controller_openvr.dll");
 library=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if(!library){errorCode=static_cast<int>(GetLastError());return false;}
 auto init=reinterpret_cast<Init>(GetProcAddress(library,"VR_InitInternal2"));auto get=reinterpret_cast<Interface>(GetProcAddress(library,"VR_GetGenericInterface"));shutdownVR=reinterpret_cast<Shutdown>(GetProcAddress(library,"VR_ShutdownInternal"));
 vr::EVRInitError error=vr::VRInitError_None;
 if(init && get && shutdownVR){init(&error,vr::VRApplication_Background,nullptr);initialized=error==vr::VRInitError_None;if(initialized)systemVR=static_cast<vr::IVRSystem*>(get(vr::IVRSystem_Version,&error));}
 errorCode=static_cast<int>(error);if(systemVR)return true;
 if(initialized && shutdownVR)shutdownVR();initialized=false;FreeLibrary(library);library=nullptr;return false;
}
}
void PollControllers(bool active,uint64_t generation){
 std::lock_guard lock(guard);uint64_t now=GetTickCount64();float dt=tick?std::min(.05f,(now-tick)*.001f):0;tick=now;
 if(generationSeen!=generation){generationSeen=generation;for(unsigned i=0;i<2;++i){controls[i]={};weights[i]=0;lastSolveTick[i]=0;}}
 if(!systemVR && now-attempt>=5000){attempt=now;Connect();}
 if(!guideFile.empty() && now-guidePoll>=500){guidePoll=now;std::ifstream input(guideFile);if(input){std::string text((std::istreambuf_iterator<char>(input)),{});ControllerElbowGuides parsed;if(ParseControllerElbowGuides(text,parsed))elbowGuides=parsed;}}

 bool nativePressed[2]{};bool nativeValid=active && PollNativeControllerButtons(nativePressed);
 head.valid=false;for(auto& h:hands)h.valid=false;
 if(systemVR && active){vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount]{};systemVR->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding,0,poses,vr::k_unMaxTrackedDeviceCount);auto observedHead=Convert(poses[0]);if(observedHead.valid)head=observedHead;
  for(unsigned i=0;i<2;++i){auto index=systemVR->GetTrackedDeviceIndexForControllerRole(i?vr::TrackedControllerRole_RightHand:vr::TrackedControllerRole_LeftHand);bool pressed=false;
   if(index<vr::k_unMaxTrackedDeviceCount){auto observedHand=Convert(poses[index]);if(observedHand.valid)hands[i]=observedHand;vr::VRControllerState_t state{};if(systemVR->GetControllerState(index,&state,sizeof(state)))pressed=(state.ulButtonPressed&vr::ButtonMaskFromId(vr::k_EButton_Axis0))!=0;}
   bool valid=hands[i].valid && head.valid;controls[i].Button(nativeValid?nativePressed[i]:pressed,now,valid);if(valid)++samples[i];
  }
 }
 for(unsigned i=0;i<2;++i){float desired=active && hands[i].valid && head.valid && controls[i].enabled?1.f:0.f;weights[i]+=std::clamp(desired-weights[i],-dt/.18f,dt/.18f);}
}
unsigned ApplyControllers(DriverIK::Pose& pose,const DriverIK::Positions& positions,bool allowLeft,bool allowRight){
 std::lock_guard lock(guard);if(!tick || GetTickCount64()-tick>250)return 0;unsigned mask=0;
 for(unsigned i=0;i<2;++i){if(!(i?allowRight:allowLeft) || weights[i]<=0 || (controls[i].calibrate && (!hands[i].valid || !head.valid)))continue;
  auto temporary=controls[i];temporary.enabled=true;bool calibration=temporary.calibrate;auto now=GetTickCount64();float seconds=lastSolveTick[i]?std::min(.05f,(now-lastSolveTick[i])*.001f):1.f/90.f;
  if(ApplyControllerHand(pose,positions,i!=0,temporary,hands[i].point,hands[i].rotation,head.rotation,weights[i],seconds,elbowGuides.values[i])){controls[i].origin=temporary.origin;controls[i].originRotation=temporary.originRotation;controls[i].anchor=temporary.anchor;controls[i].anchorRotation=temporary.anchorRotation;controls[i].basis=temporary.basis;controls[i].calibrate=temporary.calibrate;controls[i].bodyOrientation=temporary.bodyOrientation;controls[i].priorDirection=temporary.priorDirection;controls[i].priorBend=temporary.priorBend;controls[i].haveBend=temporary.haveBend;lastSolveTick[i]=now;mask|=1u<<i;++solves[i];if(calibration)++calibrations[i];}else ++rejects[i];
 }
 return mask;
}
void ControllerStatus(char* buffer,size_t length){std::lock_guard lock(guard);std::snprintf(buffer,length,"CONTROLLER_IK connected=%d error=%d native_input=%d left_valid=%d right_valid=%d left_enabled=%d right_enabled=%d samples=%llu,%llu solves=%llu,%llu rejected=%llu,%llu calibrations=%llu,%llu",systemVR?1:0,errorCode,NativeControllerInputStatus(),hands[0].valid?1:0,hands[1].valid?1:0,controls[0].enabled?1:0,controls[1].enabled?1:0,samples[0],samples[1],solves[0],solves[1],rejects[0],rejects[1],calibrations[0],calibrations[1]);}
void CloseControllers(){std::lock_guard lock(guard);systemVR=nullptr;if(initialized && shutdownVR)shutdownVR();initialized=false;if(library)FreeLibrary(library);library=nullptr;}
