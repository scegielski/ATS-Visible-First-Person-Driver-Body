#pragma once
#include "RuntimeArmIK.hpp"
struct CameraSeatDrag {bool armed=false,active=false,blocksBoxGrabs=false;DriverIK::V origin{},initial{};
 void Cancel(){active=false;armed=false;blocksBoxGrabs=false;}
 bool Step(bool permitted,const bool (&tracked)[2],const float (&grips)[2],DriverIK::V midpoint,DriverIK::V current,DriverIK::V& output){
  if(!permitted||!tracked[0]||!tracked[1]||!std::isfinite(grips[0])||!std::isfinite(grips[1])){Cancel();return false;}for(auto vector:{midpoint,current})for(float value:vector)if(!std::isfinite(value)){Cancel();return false;}if(grips[0]<.3f&&grips[1]<.3f){armed=true;blocksBoxGrabs=false;}if(active&&(grips[0]<.3f||grips[1]<.3f)){active=false;armed=false;return false;}if(!active&&armed&&grips[0]>.65f&&grips[1]>.65f){origin=midpoint;initial=current;active=true;armed=false;blocksBoxGrabs=true;}if(!active)return false;auto delta=DriverIK::Sub(midpoint,origin);for(float& axis:delta)axis=-axis;auto next=DriverIK::Add(initial,delta);for(float v:next)if(!std::isfinite(v))return false;float length=DriverIK::Length(next);if(length>2)next=DriverIK::Mul(next,2/length);if(DriverIK::Length(DriverIK::Sub(next,current))<1e-6f)return false;output=next;return true;
 }};

struct SetupCameraMode {bool enabled=false,inSetup=false;void Step(bool setup){if(setup!=inSetup||!setup)enabled=false;inSetup=setup;}bool Toggle(bool setup){Step(setup);if(!setup)return false;enabled=!enabled;return true;}};

struct CameraAngleDrag {
 int owner=-1;bool ready[2]{};DriverIK::Q origin{1,0,0,0};DriverIK::V initial{};
 void Cancel(){owner=-1;ready[0]=ready[1]=false;}
 bool Step(bool permitted,const bool (&tracked)[2],const float (&triggers)[2],const DriverIK::Q (&rotations)[2],DriverIK::V current,DriverIK::V& output){
  if(!permitted){Cancel();return false;}for(unsigned side=0;side<2;++side)if(tracked[side]&&std::isfinite(triggers[side])&&triggers[side]<.2f)ready[side]=true;
  if(owner>=0&&(!tracked[owner]||!std::isfinite(triggers[owner])||triggers[owner]<.2f)){owner=-1;return false;}
  if(owner<0)for(unsigned side=0;side<2;++side)if(ready[side]&&tracked[side]&&triggers[side]>.65f){owner=side;ready[side]=false;origin=rotations[side];initial=current;break;}
  if(owner<0)return false;auto delta=DriverIK::Product(DriverIK::Inverse(origin),rotations[owner]);auto forward=DriverIK::Rotate(delta,{0,0,-1});auto next=initial;next[0]+=std::atan2(-forward[0],-forward[2]);next[1]+=std::asin(std::clamp(forward[1],-1.f,1.f));next[0]=std::remainder(next[0],6.283185307f);next[1]=std::clamp(next[1],-1.5f,1.5f);for(float v:next)if(!std::isfinite(v))return false;if(DriverIK::Length(DriverIK::Sub(next,current))<1e-6f)return false;output=next;return true;
 }
};
