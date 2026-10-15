#pragma once
#include "vendor/openvr.h"
#include <algorithm>
#include <cmath>
// SteamVR Quest component definitions use axes 0=stick, 1=trigger, 2=grip,
// and legacy button IDs 2=X/A, 1=Y/B, 32=stick click.
inline vr::VRControllerState_t ControllerModelInput(bool click,bool primary,bool secondary,float grip,float trigger,float x,float y){
 vr::VRControllerState_t state{};auto finite=[](float v,float lo,float hi){return std::isfinite(v)?std::clamp(v,lo,hi):0.f;};
 state.rAxis[0]={finite(x,-1,1),finite(y,-1,1)};state.rAxis[1].x=finite(trigger,0,1);state.rAxis[2].x=finite(grip,0,1);
 if(click)state.ulButtonPressed|=vr::ButtonMaskFromId(vr::k_EButton_Axis0);
 if(primary)state.ulButtonPressed|=vr::ButtonMaskFromId(vr::k_EButton_Grip);
 if(secondary)state.ulButtonPressed|=vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu);
 if(trigger>=.95f)state.ulButtonPressed|=vr::ButtonMaskFromId(vr::k_EButton_Axis1);
 state.ulButtonTouched=state.ulButtonPressed;return state;
}
