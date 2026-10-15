#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
// On the map, defer trigger click until release or convert motion to right drag.
struct MenuButtonRoute {
 bool triggerHeld=false,gripHeld=false,triggerRight=false,pendingMapClick=false;uint8_t heldMask=0;float travelX=0,travelY=0,heldSeconds=0;
 struct Output {uint8_t pressed=0,released=0,tapped=0;};
 Output Cancel(){Output out{0,heldMask,0};*this={};return out;}
 Output Step(int triggerEdge,int gripEdge,bool mapActive,float dx=0,float dy=0,float dt=.01f){
  uint8_t tapped=0;
  if(triggerEdge>0){triggerHeld=true;triggerRight=mapActive;pendingMapClick=mapActive;travelX=travelY=heldSeconds=0;}
  else if(triggerEdge<0){if(triggerHeld&&pendingMapClick)tapped=1;triggerHeld=pendingMapClick=false;}
  if(triggerHeld&&pendingMapClick&&triggerEdge<=0&&std::isfinite(dx)&&std::isfinite(dy)){
   heldSeconds+=std::isfinite(dt)?std::clamp(dt,0.f,.05f):0.f;travelX+=dx;travelY+=dy;if(heldSeconds>=.15f&&std::hypot(travelX,travelY)>=8)pendingMapClick=false;
  }
  if(gripEdge>0)gripHeld=true;else if(gripEdge<0)gripHeld=false;
  uint8_t next=(triggerHeld&&!triggerRight?1:0)|((gripHeld||(triggerHeld&&triggerRight&&!pendingMapClick))?2:0);
  Output out{static_cast<uint8_t>(next&~heldMask),static_cast<uint8_t>(heldMask&~next),tapped};heldMask=next;return out;
 }
};
