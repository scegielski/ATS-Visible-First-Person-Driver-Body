#pragma once
#include "LiveControllerFingers.hpp"
struct ControllerFingerMode {
 std::array<float,15> weights{};bool initialized=false;float previewSeconds=0;
 bool down=false,active=false,stickHeld=true,toggleDown=true;unsigned finger=1,mask=2;uint64_t edits=0;uint64_t revision=UINT64_MAX,switches=0;
 void Step(bool pressed,bool valid,uint64_t poseRevision,const ControllerFingerPose& pose){
  if(!initialized){for(unsigned i=0;i<15;++i)weights[i]=pose[i].triggerControlled?1.f:0.f;initialized=true;}
  if(poseRevision!=revision){revision=poseRevision;active=false;mask=0;for(unsigned i=0;i<4;++i)if(pose[i*3].triggerControlled)mask|=1u<<i;finger=0;for(unsigned i=0;i<4;++i)if(pose[i*3].triggerControlled)finger=i;}
  if(!valid){down=true;return;}
  if(pressed && !down){constexpr unsigned next[]={1,3,0,2};finger=next[finger];previewSeconds=.35f;++switches;}down=pressed;
 }
 bool Toggle(float inward,bool valid){
  if(!valid || !std::isfinite(inward)){stickHeld=true;return false;}
  if(inward<.3f)stickHeld=false;
  if(inward>.7f && !stickHeld){stickHeld=true;active=true;mask^=1u<<finger;++edits;return true;}
  return false;
 }
 bool ToggleButton(bool pressed,bool valid){
  if(!valid){toggleDown=true;return false;}
  bool edge=pressed&&!toggleDown;toggleDown=pressed;
  if(!edge)return false;
  active=true;mask^=1u<<finger;++edits;return true;
 }
 bool AssignDirection(float x,bool valid){
  if(!valid || !std::isfinite(x)){stickHeld=true;return false;}
  if(std::abs(x)<.3f){stickHeld=false;return false;}
  if(std::abs(x)<.7f || stickHeld)return false;
  stickHeld=true;bool enabled=x>0;bool wasEnabled=Selected();
  if(enabled==wasEnabled)return false;
  active=true;if(enabled)mask|=1u<<finger;else mask&=~(1u<<finger);++edits;return true;
 }
 bool Selected() const {return (mask&(1u<<finger))!=0;}
 const char* SpokenFinger() const {const char* names[]={"index","middle","pinky","ring"};return names[finger];}
 const char* Name() const {const char* names[]={"index","middle","little","ring"};return active?names[finger]:"file";}
 void Advance(const ControllerFingerPose& original,float seconds,float duration){
  if(!initialized || !std::isfinite(seconds) || !std::isfinite(duration) || seconds<0 || duration<=0)return;
  previewSeconds=std::max(0.f,previewSeconds-seconds);auto target=Targets(original);float step=seconds/duration;
  for(unsigned i=0;i<15;++i)weights[i]+=std::clamp((target[i].triggerControlled?1.f:0.f)-weights[i],-step,step);
 }
 ControllerFingerPose Targets(const ControllerFingerPose& original) const {
  auto result=original;if(active){for(unsigned i=0;i<result.size();++i)result[i].triggerControlled=i<12 && (mask&(1u<<(i/3)))!=0;}return result;
 }
};
