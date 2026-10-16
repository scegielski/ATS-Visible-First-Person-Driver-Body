#pragma once
#include "RuntimeArmIK.hpp"
struct ControllerSceneFrame {
 DriverIK::V cameraBase{-.3847368658f,1.066045642f,.1469421685f},seatOffset{};
 bool SetVehicleCamera(DriverIK::V cabin,DriverIK::V head) {
  for(auto v:{cabin,head})for(float x:v)if(!std::isfinite(x)||std::abs(x)>50)return false;
  if(DriverIK::Length(head)<.05f)return false; // SDK omitted the head attribute.
  cameraBase=DriverIK::Add(cabin,head);camera=DriverIK::Add(cameraBase,seatOffset);return true;
 }
 void SetSeatOffset(DriverIK::V offset){seatOffset=offset;camera=DriverIK::Add(cameraBase,seatOffset);}
 void ResetVehicleCamera(){cameraBase={-.3847368658f,1.066045642f,.1469421685f};seatOffset={};camera=cameraBase;}
 DriverIK::V neutral{},xrHead{},camera{};DriverIK::Q neutralQ{1,0,0,0},xrHeadQ{1,0,0,0};
 DriverIK::V Position(DriverIK::V controller,DriverIK::V steamHead,DriverIK::Q steamHeadQ) const {
  auto space=DriverIK::Product(xrHeadQ,DriverIK::Inverse(steamHeadQ));auto local=DriverIK::Add(xrHead,DriverIK::Rotate(space,DriverIK::Sub(controller,steamHead)));
  return DriverIK::Add(camera,DriverIK::Rotate(DriverIK::Inverse(neutralQ),DriverIK::Sub(local,neutral)));
 }
 DriverIK::Q Orientation(DriverIK::Q controller,DriverIK::Q steamHeadQ) const {return DriverIK::Product(DriverIK::Inverse(neutralQ),DriverIK::Product(xrHeadQ,DriverIK::Product(DriverIK::Inverse(steamHeadQ),controller)));}
};
