#pragma once
#include "WindowGesture.hpp"
struct IgnitionMotion {
 float reach=0,turn=0,age=0;bool active=false;
 void Start() noexcept {active=true;age=0;}
 void Step(bool starting,bool fresh,float seconds) noexcept {
  const float dt=std::isfinite(seconds)?std::clamp(seconds,0.f,.05f):0.f;
  if(!fresh)active=false;
  if(active){
   age+=dt;
   const bool arrived=reach>=1;
   reach=std::min(1.f,reach+dt/.45f);
   if(arrived)turn=std::min(1.f,turn+dt/.18f);
   // Complete the visual turn even for a short start pulse; bound a stuck signal.
   if((!starting && turn>=1 && age>=.85f) || age>=5.f)active=false;
  }else{
   turn=std::max(0.f,turn-dt/.12f);
   if(turn==0)reach=std::max(0.f,reach-dt/.35f);
  }
 }
 WindowGestureMotion Pose() const noexcept{const float setup=std::clamp((reach-.35f)/.65f,0.f,1.f);return {reach,setup*setup*(3-2*setup),turn};}
};
