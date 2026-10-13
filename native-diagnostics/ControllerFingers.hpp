#pragma once
#include "ControllerFingerTargets.hpp"
inline bool BlendControllerFingers(DriverIK::Pose& pose,bool right,float grip,float trigger,float blend,const std::array<ControllerFingerTarget,15>* edited=nullptr,const std::array<float,15>* routing=nullptr,float thumb=-1.f,int previewFinger=-1,float preview=0){
 if(!std::isfinite(grip) || !std::isfinite(trigger) || !std::isfinite(blend))return false;
 grip=std::clamp(grip,0.f,1.f);trigger=std::clamp(trigger,0.f,1.f);blend=std::clamp(blend,0.f,1.f);if(blend==0)return true;
 const auto& targets=edited?*edited:(right?kControllerRightFingers:kControllerLeftFingers);
 for(unsigned i=0;i<targets.size();++i){const auto& target=targets[i];float w=routing?std::clamp((*routing)[i],0.f,1.f):(target.triggerControlled?1.f:0.f);w=w*w*(3-2*w);float curl=(thumb>=0 && i>=12)?std::clamp(thumb,0.f,1.f):grip+(trigger-grip)*w;if(static_cast<int>(i/3)==previewFinger)curl+=(1-2*curl)*std::clamp(preview,0.f,1.f);auto desired=HornSlerp(target.open,target.closed,curl);pose[target.bone]=HornSlerp(pose[target.bone],desired,blend);}
 return true;
}
