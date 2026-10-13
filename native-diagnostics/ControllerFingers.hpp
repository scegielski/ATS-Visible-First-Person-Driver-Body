#pragma once
#include "ControllerFingerTargets.hpp"
inline bool BlendControllerFingers(DriverIK::Pose& pose,bool right,float grip,float trigger,float blend,const std::array<ControllerFingerTarget,15>* edited=nullptr){
 if(!std::isfinite(grip) || !std::isfinite(trigger) || !std::isfinite(blend))return false;
 grip=std::clamp(grip,0.f,1.f);trigger=std::clamp(trigger,0.f,1.f);blend=std::clamp(blend,0.f,1.f);if(blend==0)return true;
 const auto& targets=edited?*edited:(right?kControllerRightFingers:kControllerLeftFingers);
 for(const auto& target:targets){auto desired=HornSlerp(target.open,target.closed,target.triggerControlled?trigger:grip);pose[target.bone]=HornSlerp(pose[target.bone],desired,blend);}
 return true;
}
