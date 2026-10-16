#pragma once
#include "InteractionBoxEditor.hpp"
#include "IgnitionInteraction.hpp"
inline int MirrorContact(const InteractionBoxEditor& editor,DriverIK::V center,DriverIK::Q rotation){
 int found=-1;float nearest=1e9f;
 for(unsigned i=2;i<12;++i){const auto& b=editor.boxes[i];DriverIK::V half{};for(unsigned j=0;j<3;++j)half[j]=b.half[j]*b.scale*b.stretch[j];if(IgnitionBoxesOverlap(b.center,half,center,rotation,{.04f,.035f,.055f},b.rotation)){float d=DriverIK::Length(DriverIK::Sub(center,b.center));if(d<nearest){nearest=d;found=static_cast<int>(i-2);}}}return found;
}
inline int MirrorPhysicalSlot(int mirror){constexpr int slots[7]{0,4,2,6,-1,-1,9};return mirror>=0 && mirror<7?slots[mirror]:-1;}

struct ControllerContactVolume {DriverIK::V center{};DriverIK::Q rotation{1,0,0,0};bool valid=false;};
inline ControllerContactVolume ControllerInteractionVolume(bool enabled,bool tracked,bool sceneFresh,bool solved,DriverIK::V palm,DriverIK::Q palmQ,DriverIK::V controller,DriverIK::Q controllerQ){
 if(!tracked || !sceneFresh || (enabled && !solved))return {};
 return enabled?ControllerContactVolume{palm,palmQ,true}:ControllerContactVolume{controller,controllerQ,true};
}

struct InteractionRayHit {int box=-1;float distance=2.f;};
inline InteractionRayHit RayInteractionBoxes(const InteractionBoxEditor& editor,DriverIK::V origin,DriverIK::V direction,float range=2.f){
 InteractionRayHit hit{-1,range};for(float v:origin)if(!std::isfinite(v))return hit;for(float v:direction)if(!std::isfinite(v))return hit;
 float length=DriverIK::Length(direction);if(length<1e-6f || !std::isfinite(range) || range<=0)return hit;direction=DriverIK::Mul(direction,1.f/length);
 for(unsigned i=0;i<InteractionBoxCount;++i){const auto& box=editor.boxes[i];auto inverse=DriverIK::Inverse(box.rotation);auto o=DriverIK::Rotate(inverse,DriverIK::Sub(origin,box.center));auto d=DriverIK::Rotate(inverse,direction);float entryT=0,exitT=range;bool valid=true;
  for(unsigned j=0;j<3;++j){float half=box.half[j]*box.scale*box.stretch[j];if(std::abs(d[j])<1e-6f){if(std::abs(o[j])>half){valid=false;break;}}else{float a=(-half-o[j])/d[j],b=(half-o[j])/d[j];if(a>b)std::swap(a,b);entryT=std::max(entryT,a);exitT=std::min(exitT,b);if(entryT>exitT){valid=false;break;}}}
  if(valid && entryT<=hit.distance && (hit.box<0 || entryT<hit.distance)){hit.box=static_cast<int>(i);hit.distance=entryT;}
 }return hit;
}


inline bool ControllerHornPressed(bool target,bool valid,float trigger){return target && valid && std::isfinite(trigger) && trigger>.7f;}
