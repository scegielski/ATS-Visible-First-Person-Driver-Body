#pragma once
#include "RuntimeArmIK.hpp"
struct ControllerSceneFrame {
 DriverIK::V neutral{},xrHead{},camera{};DriverIK::Q neutralQ{1,0,0,0},xrHeadQ{1,0,0,0};
 DriverIK::V Position(DriverIK::V controller,DriverIK::V steamHead,DriverIK::Q steamHeadQ) const {
  auto space=DriverIK::Product(xrHeadQ,DriverIK::Inverse(steamHeadQ));auto local=DriverIK::Add(xrHead,DriverIK::Rotate(space,DriverIK::Sub(controller,steamHead)));
  return DriverIK::Add(camera,DriverIK::Rotate(DriverIK::Inverse(neutralQ),DriverIK::Sub(local,neutral)));
 }
 DriverIK::Q Orientation(DriverIK::Q controller,DriverIK::Q steamHeadQ) const {return DriverIK::Product(DriverIK::Inverse(neutralQ),DriverIK::Product(xrHeadQ,DriverIK::Product(DriverIK::Inverse(steamHeadQ),controller)));}
};
