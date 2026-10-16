#pragma once
#include "IgnitionInteraction.hpp"
struct SteeringHandDragInput {DriverIK::V position{};bool valid=false;float trigger=0;int target=-1;};
struct SteeringHandDrag {
 IgnitionTriggerGate gates[2];int owner=-1,hand=-1;DriverIK::V origin{},initial{};
 void Release(){owner=hand=-1;}
 bool Step(const SteeringHandDragInput (&inputs)[2],bool active,DriverIK::V (&offsets)[2]){
  bool edges[2]{};for(unsigned i=0;i<2;++i)edges[i]=gates[i].Step(inputs[i].trigger,active&&inputs[i].valid,inputs[i].target>=0&&inputs[i].target<2);
  if(owner>=0&&(!active||!inputs[owner].valid||!std::isfinite(inputs[owner].trigger)||inputs[owner].trigger<.2f))Release();
  if(owner<0&&active)for(unsigned i=0;i<2;++i)if(edges[i]){owner=i;hand=inputs[i].target;origin=inputs[i].position;initial=offsets[hand];break;}
  if(owner<0)return false;auto next=DriverIK::Add(initial,DriverIK::Sub(inputs[owner].position,origin));for(float v:next)if(!std::isfinite(v))return false;float length=DriverIK::Length(next);if(length>.5f)next=DriverIK::Mul(next,.5f/length);if(DriverIK::Length(DriverIK::Sub(next,offsets[hand]))<1e-6f)return false;offsets[hand]=next;return true;
 }
};
