#pragma once
#include "ControllerIK.hpp"
void PollControllers(bool active,uint64_t generation);
unsigned ApplyControllers(DriverIK::Pose& pose,const DriverIK::Positions& positions,bool allowLeft,bool allowRight);
void CloseControllers();
void ControllerStatus(char* buffer,size_t length);
