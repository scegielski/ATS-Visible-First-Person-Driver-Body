#pragma once
#include "IgnitionInteraction.hpp"
struct SteeringHandDragInput {DriverIK::V position{};bool valid=false;float trigger=0;int target=-1;DriverIK::Q rotation{1,0,0,0};};
struct SteeringHandDrag {
 IgnitionTriggerGate gates[2];int owner=-1,hand=-1;DriverIK::V origin{},initial{};
 DriverIK::Q rotationOrigin{1,0,0,0},rotationInitial{1,0,0,0};
 void Release(){owner=hand=-1;}
 template<size_t N> bool Step(const SteeringHandDragInput (&inputs)[2],bool active,DriverIK::V (&offsets)[N],DriverIK::Q* rotations=nullptr){
  bool edges[2]{};for(unsigned i=0;i<2;++i)edges[i]=gates[i].Step(inputs[i].trigger,active&&inputs[i].valid,inputs[i].target>=0&&static_cast<size_t>(inputs[i].target)<N);
  if(owner>=0&&(!active||!inputs[owner].valid||!std::isfinite(inputs[owner].trigger)||inputs[owner].trigger<.2f))Release();
  if(owner<0&&active)for(unsigned i=0;i<2;++i)if(edges[i]){owner=i;hand=inputs[i].target;origin=inputs[i].position;initial=offsets[hand];rotationOrigin=inputs[i].rotation;if(rotations&&hand>=2)rotationInitial=rotations[hand-2];break;}
  if(owner<0)return false;bool rotated=false;if(rotations&&hand>=2){auto q=DriverIK::Product(DriverIK::Product(inputs[owner].rotation,DriverIK::Inverse(rotationOrigin)),rotationInitial);float n=0;for(float v:q)n+=v*v;if(std::isfinite(n)&&n>1e-8f){for(float& v:q)v/=std::sqrt(n);float d=0;for(unsigned axis=0;axis<4;++axis)d+=q[axis]*rotations[hand-2][axis];if(std::abs(d)<.9999999f){rotations[hand-2]=q;rotated=true;}}}auto next=DriverIK::Add(initial,DriverIK::Sub(inputs[owner].position,origin));for(float v:next)if(!std::isfinite(v))return false;float length=DriverIK::Length(next);if(hand<2&&length>.5f)next=DriverIK::Mul(next,.5f/length);if(DriverIK::Length(DriverIK::Sub(next,offsets[hand]))<1e-6f)return rotated;offsets[hand]=next;return true;
 }
};
