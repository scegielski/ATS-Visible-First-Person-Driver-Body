#pragma once
#include <algorithm>
#include <cmath>
// Separate reach and press: release lifts the palm before withdrawing the arm.
// The eventual pose blend must always use this frame's steering grip as its base.
struct HornMotion {
 float reach=0,press=0;
 void Step(bool held,float seconds) noexcept {
  const float dt=std::isfinite(seconds)?std::clamp(seconds,0.f,.05f):0.f;
  if(held) {
   if(reach<1)reach=std::min(1.f,reach+dt/.25f);
   else press=std::min(1.f,press+dt/.08f);
  } else {
   if(press>0)press=std::max(0.f,press-dt/.08f);
   else reach=std::max(0.f,reach-dt/.25f);
  }
 }
};
