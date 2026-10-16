#pragma once
#include "RuntimeArmIK.hpp"
struct CameraSeatDrag {bool armed=false,active=false;DriverIK::V origin{},initial{};
 void Cancel(){active=false;armed=false;}
 bool Step(bool permitted,const bool (&tracked)[2],const float (&grips)[2],DriverIK::V midpoint,DriverIK::V current,DriverIK::V& output){
  if(!permitted||!tracked[0]||!tracked[1]||!std::isfinite(grips[0])||!std::isfinite(grips[1])){Cancel();return false;}for(auto vector:{midpoint,current})for(float value:vector)if(!std::isfinite(value)){Cancel();return false;}if(grips[0]<.3f&&grips[1]<.3f)armed=true;if(active&&(grips[0]<.3f||grips[1]<.3f)){active=false;armed=false;return false;}if(!active&&armed&&grips[0]>.65f&&grips[1]>.65f){origin=midpoint;initial=current;active=true;armed=false;}if(!active)return false;auto next=DriverIK::Add(initial,DriverIK::Sub(midpoint,origin));for(float v:next)if(!std::isfinite(v))return false;float length=DriverIK::Length(next);if(length>2)next=DriverIK::Mul(next,2/length);if(DriverIK::Length(DriverIK::Sub(next,current))<1e-6f)return false;output=next;return true;
 }};
