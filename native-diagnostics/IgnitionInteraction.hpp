#pragma once
#include "RuntimeArmIK.hpp"
inline bool IgnitionBoxesOverlap(DriverIK::V key,DriverIK::V keyHalf,DriverIK::V hand,DriverIK::Q rotation,DriverIK::V handHalf){
 using namespace DriverIK;V axes[6]={{1,0,0},{0,1,0},{0,0,1},Rotate(rotation,{1,0,0}),Rotate(rotation,{0,1,0}),Rotate(rotation,{0,0,1})};V delta=Sub(hand,key);
 for(unsigned i=0;i<15;++i){V a=i<6?axes[i]:Cross(axes[(i-6)/3],axes[3+(i-6)%3]);if(Length(a)<1e-6f)continue;a=Unit(a);float radius=0;for(unsigned j=0;j<3;++j)radius+=keyHalf[j]*std::abs(Dot(a,axes[j]))+handHalf[j]*std::abs(Dot(a,axes[j+3]));if(std::abs(Dot(delta,a))>radius)return false;}return true;
}
struct IgnitionTriggerGate {bool held=true;bool Step(float trigger,bool valid,bool overlap){if(!valid || !std::isfinite(trigger)){held=true;return false;}if(trigger<.2f)held=false;if(trigger>.7f && !held){held=true;return overlap;}return false;}};
