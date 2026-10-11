#pragma once
#include "HornPose.hpp"
#include "WindowGestureTargets.hpp"
struct WindowGestureMotion {
 float reach=0,press=0;
 void Step(bool held,float seconds) noexcept {
  const float dt=std::isfinite(seconds)?std::clamp(seconds,0.f,.05f):0.f;
  if(held) {
   reach=std::min(1.f,reach+dt/.45f);
   press=std::min(1.f,press+dt/.30f);
  } else {
   if(press>0)press=std::max(0.f,press-dt/.12f);
   else reach=std::max(0.f,reach-dt/.35f);
  }
 }
};
struct LeftArmGestures {
 HornMotion horn;
 WindowGestureMotion window;
 void Step(bool hornHeld,bool windowHeld,float seconds) noexcept {
  // Return one gesture to the current grip before starting the other.
  if(hornHeld){window.Step(false,seconds);horn.Step(window.reach==0,seconds);}
  else if(horn.reach>0){horn.Step(false,seconds);window.Step(false,seconds);}
  else window.Step(windowHeld,seconds);
 }
};
inline void BlendWindowGesture(std::array<std::array<float,4>,61>& rotations,const WindowGestureMotion& motion) noexcept {
 if(motion.reach<=0)return;
 const float reach=motion.reach*motion.reach*(3-2*motion.reach);
 for(const auto& target:kWindowGestureTargets){
  const auto desired=HornSlerp(target.hover,target.pressed,motion.press);
  rotations[target.bone]=HornSlerp(rotations[target.bone],desired,reach);
 }
}
