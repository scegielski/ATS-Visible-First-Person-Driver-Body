#pragma once
#include "WindowGesture.hpp"
struct SignalSelection {
 int previous=-1;
 int Sample(bool left,bool right,bool hazards) noexcept {
  if(hazards || (left && right)){previous=-1;return 0;}
  int selected=left==right?0:left?1:2;
  int result=previous>=0 && selected!=previous?selected:0;previous=selected;return result;
 }
 void Reset() noexcept {previous=-1;}
};
struct SignalMotion {
 enum Phase {Idle,Reach,Actuate,Return};Phase phase=Idle;
 float reach=0,press=0,hold=0,age=0;int direction=0;
 void Start(int value) noexcept {if(phase==Idle && (value==1 || value==2)){direction=value;age=0;phase=Reach;}}
 void Step(bool fresh,bool interrupt,float seconds,float actuationSeconds=.12f) noexcept {
  float dt=std::isfinite(seconds)?std::clamp(seconds,0.f,.05f):0.f;
  const float actuation=std::isfinite(actuationSeconds)?std::clamp(actuationSeconds,.05f,.6f):.12f;
  if(phase!=Idle)age+=dt;
  if(!fresh || interrupt)phase=reach>0?Return:Idle;
  if(phase==Reach){reach=std::min(1.f,reach+dt/.22f);if(reach>=1)phase=Actuate;}
  else if(phase==Actuate){press=std::min(1.f,press+dt/actuation);if(press>=1){hold+=dt;if(hold>=.12f)phase=Return;}}
  else if(phase==Return){reach=std::max(0.f,reach-dt/.22f);if(reach==0){phase=Idle;press=hold=0;direction=0;}}
 }
 WindowGestureMotion Pose() const noexcept{const float setup=std::clamp((reach-.35f)/.65f,0.f,1.f);return {reach,setup*setup*(3-2*setup),press};}
};
