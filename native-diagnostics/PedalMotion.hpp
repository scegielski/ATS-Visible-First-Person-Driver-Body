#pragma once
#include "SteeringPose.hpp"
inline bool IsBrakeClip(float duration,uint32_t frames) noexcept {
 return std::isfinite(duration) && std::fabs(duration-162.5f)<=.01f && frames==3901;
}
inline bool IsClutchClip(float duration,uint32_t frames) noexcept {
 return std::isfinite(duration) && std::fabs(duration-1462.8333333f)<=.01f && frames==35109;
}
inline float PedalValue(float value) noexcept {
 return std::isfinite(value) ? std::clamp(value,0.f,1.f) : 0.f;
}
inline float Approach(float current,float target,float step) noexcept {
 return current<target ? std::min(target,current+step) : std::max(target,current-step);
}
inline uint32_t ClutchStartFrame(uint32_t rightStartFrame,float clutch) noexcept {
 return static_cast<uint32_t>(std::lround(PedalValue(clutch)*8.f))*3901u+rightStartFrame;
}
struct PedalMotion {
 float transfer=0,press=0;
 enum Phase { Gas,LiftGas,MoveToBrake,BrakeHover,BrakePress,LiftBrake,ReturnToGas };
 Phase phase=Gas;
 void Step(float gas,float brake,float seconds) noexcept {
  gas=PedalValue(gas);brake=PedalValue(brake);
  const float dt=std::isfinite(seconds)?std::clamp(seconds,0.f,.05f):0.f;
  if(brake>.01f) {
   if(transfer<1.f) {
    press=Approach(press,0.f,dt/.1f);
    if(press==0.f) {transfer=Approach(transfer,1.f,dt/.2f);phase=transfer==1.f?BrakeHover:MoveToBrake;}
    else phase=LiftGas;
   } else {press=Approach(press,brake,dt/.1f);phase=press>0.f?BrakePress:BrakeHover;}
  } else {
   if(transfer>0.f) {
    press=Approach(press,0.f,dt/.1f);
    if(press==0.f) {transfer=Approach(transfer,0.f,dt/.2f);phase=ReturnToGas;}
    else phase=LiftBrake;
   } else {press=gas;phase=Gas;}
  }
 }
 uint32_t StartFrame() const noexcept {
  if(transfer<=0.f)return ThrottleStartFrame(press);
  if(transfer>=1.f)return (50u+static_cast<uint32_t>(std::lround(PedalValue(press)*32.f)))*47u;
  return (33u+static_cast<uint32_t>(std::lround(PedalValue(transfer)*16.f)))*47u;
 }
};
inline const char* PedalPhaseName(PedalMotion::Phase phase) noexcept {
 switch(phase) {
 case PedalMotion::Gas:return "gas";
 case PedalMotion::LiftGas:return "lift_gas";
 case PedalMotion::MoveToBrake:return "move_to_brake";
 case PedalMotion::BrakeHover:return "brake_hover";
 case PedalMotion::BrakePress:return "brake_press";
 case PedalMotion::LiftBrake:return "lift_brake";
 case PedalMotion::ReturnToGas:return "return_to_gas";
 }
 return "unknown";
}
