#pragma once
#include "RuntimeArmIK.hpp"
struct ControllerSceneFrame {
 DriverIK::Q viewRotation{1,0,0,0};
 DriverIK::Q TrackingToCabin()const{return DriverIK::Product(viewRotation,DriverIK::Inverse(neutralQ));}
 DriverIK::Q CabinToTracking()const{return DriverIK::Inverse(TrackingToCabin());}
 static DriverIK::Q LookRotation(DriverIK::V a){return DriverIK::Product(DriverIK::Q{std::cos(a[0]/2),0,std::sin(a[0]/2),0},DriverIK::Q{std::cos(a[1]/2),std::sin(a[1]/2),0,0});}
 void SetViewAngles(DriverIK::V a){viewRotation=LookRotation(a);}
 void SetAppliedViewAngles(DriverIK::V before,DriverIK::V after){viewRotation=DriverIK::Product(LookRotation(after),DriverIK::Inverse(LookRotation(before)));}
 DriverIK::V cameraBase{-.3847368658f,1.066045642f,.1469421685f},seatOffset{};
 bool SetVehicleCamera(DriverIK::V cabin,DriverIK::V head) {
  for(auto v:{cabin,head})for(float x:v)if(!std::isfinite(x)||std::abs(x)>50)return false;
  if(DriverIK::Length(head)<.05f)return false; // SDK omitted the head attribute.
  cameraBase=DriverIK::Add(cabin,head);camera=DriverIK::Add(cameraBase,seatOffset);return true;
 }
 void SetSeatOffset(DriverIK::V offset){seatOffset=offset;camera=DriverIK::Add(cameraBase,seatOffset);}
 void ResetVehicleCamera(){viewRotation={1,0,0,0};cameraBase={-.3847368658f,1.066045642f,.1469421685f};seatOffset={};camera=cameraBase;}
 DriverIK::V neutral{},xrHead{},camera{};DriverIK::Q neutralQ{1,0,0,0},xrHeadQ{1,0,0,0};
 DriverIK::V HeadInCabin()const{return DriverIK::Add(camera,DriverIK::Rotate(DriverIK::Inverse(neutralQ),DriverIK::Sub(xrHead,neutral)));}
 DriverIK::V CabinPointToTracking(DriverIK::V point)const{if(viewRotation==DriverIK::Q{1,0,0,0})return DriverIK::Add(neutral,DriverIK::Rotate(neutralQ,DriverIK::Sub(point,camera)));return DriverIK::Add(xrHead,DriverIK::Rotate(CabinToTracking(),DriverIK::Sub(point,HeadInCabin())));}
 DriverIK::V Position(DriverIK::V controller,DriverIK::V steamHead,DriverIK::Q steamHeadQ) const {
  auto space=DriverIK::Product(xrHeadQ,DriverIK::Inverse(steamHeadQ));auto local=DriverIK::Add(xrHead,DriverIK::Rotate(space,DriverIK::Sub(controller,steamHead)));
  return DriverIK::Add(HeadInCabin(),DriverIK::Rotate(TrackingToCabin(),DriverIK::Sub(local,xrHead)));
 }
 DriverIK::Q Orientation(DriverIK::Q controller,DriverIK::Q steamHeadQ) const {return DriverIK::Product(TrackingToCabin(),DriverIK::Product(xrHeadQ,DriverIK::Product(DriverIK::Inverse(steamHeadQ),controller)));}
};
