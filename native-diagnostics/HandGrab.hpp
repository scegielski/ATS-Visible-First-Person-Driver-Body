#pragma once
#include "InteractionBoxEditor.hpp"
#include "IgnitionInteraction.hpp"
#include "MenuDoublePress.hpp"
struct HandGrabGesture {
 IgnitionTriggerGate engage;MenuDoublePress release;bool triggerDown=false;
 int Step(bool engaged,float trigger,bool valid,bool setup,bool target,uint64_t now){
  if(!valid||setup||!std::isfinite(trigger)){engage.Step(0,false,false);release.Step(false,false,now);triggerDown=false;return 0;}
  triggerDown=trigger>(triggerDown?.2f:.7f);
  if(engaged){engage.Step(trigger,false,false);return release.Step(true,triggerDown,now)?-1:0;}
  release.Step(false,false,now);
  if(engage.Step(trigger,true,target)){release.Step(true,triggerDown,now);return 1;}
  return 0;
 }
};
inline DriverIK::Q HandBoxUnitRotation(DriverIK::Q q){float n=0;for(float v:q)n+=v*v;if(!std::isfinite(n)||n<1e-12f)return {1,0,0,0};for(float& v:q)v/=std::sqrt(n);return q;}
inline InteractionBox AttachHandBox(const InteractionBox& local,DriverIK::V wrist,DriverIK::Q rotation){rotation=HandBoxUnitRotation(rotation);auto box=local;box.center=DriverIK::Add(wrist,DriverIK::Rotate(rotation,local.center));box.rotation=HandBoxUnitRotation(DriverIK::Product(rotation,HandBoxUnitRotation(local.rotation)));return box;}
inline InteractionBox DetachHandBox(const InteractionBox& world,DriverIK::V wrist,DriverIK::Q rotation){rotation=HandBoxUnitRotation(rotation);auto box=world;box.center=DriverIK::Rotate(DriverIK::Inverse(rotation),DriverIK::Sub(world.center,wrist));box.rotation=HandBoxUnitRotation(DriverIK::Product(DriverIK::Inverse(rotation),HandBoxUnitRotation(world.rotation)));return box;}
struct HandBoxAttachments {
 InteractionBox local[4]{{{.07f,0,0},{.04f,.035f,.055f},{1,0,0,0},1,{1.25f,1.428571429f,.909090909f}},{{-.07f,-.02f,0},{.04f,.035f,.055f},{1,0,0,0},1,{1.25f,1.428571429f,.909090909f}},{{0,0,0},{.05f,.05f,.05f}},{{0,0,0},{.05f,.05f,.05f}}};
 DriverIK::V wrist[4]{};DriverIK::Q rotation[4]{{1,0,0,0},{1,0,0,0},{1,0,0,0},{1,0,0,0}};bool valid=false;
 void Load(InteractionBoxEditor& editor){for(unsigned i=0;i<4;++i){unsigned box=i<2?12+i:14+i;local[i]=editor.boxes[box];if(valid)editor.boxes[box]=AttachHandBox(local[i],wrist[i],rotation[i]);}}
 void CaptureEdit(const InteractionBoxEditor& editor){if(valid&&((editor.selected>=12&&editor.selected<14)||editor.selected>=16)){unsigned i=unsigned(editor.selected-(editor.selected<14?12:14));local[i]=DetachHandBox(editor.boxes[editor.selected],wrist[i],rotation[i]);}}
 InteractionBoxEditor Saved(const InteractionBoxEditor& editor)const{auto saved=editor;for(unsigned i=0;i<4;++i)saved.boxes[i<2?12+i:14+i]=local[i];return saved;}
};
