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
inline InteractionBox AttachHandBox(const InteractionBox& local,DriverIK::V wrist,DriverIK::Q rotation){auto box=local;box.center=DriverIK::Add(wrist,DriverIK::Rotate(rotation,local.center));box.rotation=DriverIK::Product(rotation,local.rotation);return box;}
inline InteractionBox DetachHandBox(const InteractionBox& world,DriverIK::V wrist,DriverIK::Q rotation){auto box=world;box.center=DriverIK::Rotate(DriverIK::Inverse(rotation),DriverIK::Sub(world.center,wrist));box.rotation=DriverIK::Product(DriverIK::Inverse(rotation),world.rotation);return box;}
struct HandBoxAttachments {
 InteractionBox local[2]{{{.14f,0,0},{.04f,.035f,.055f}},{{-.14f,-.02f,0},{.04f,.035f,.055f}}};
 DriverIK::V wrist[2]{};DriverIK::Q rotation[2]{{1,0,0,0},{1,0,0,0}};bool valid=false;
 void Load(InteractionBoxEditor& editor){for(unsigned i=0;i<2;++i){local[i]=editor.boxes[12+i];if(valid)editor.boxes[12+i]=AttachHandBox(local[i],wrist[i],rotation[i]);}}
 void CaptureEdit(const InteractionBoxEditor& editor){if(valid)for(unsigned i=0;i<2;++i)local[i]=DetachHandBox(editor.boxes[12+i],wrist[i],rotation[i]);}
 InteractionBoxEditor Saved(const InteractionBoxEditor& editor)const{auto saved=editor;for(unsigned i=0;i<2;++i)saved.boxes[12+i]=local[i];return saved;}
};
