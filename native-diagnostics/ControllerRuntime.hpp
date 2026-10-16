#pragma once
#include "ControllerIK.hpp"
#include "VehicleSteeringPose.hpp"
void PollControllers(bool active,uint64_t generation);
unsigned ApplyControllers(DriverIK::Pose& pose,const DriverIK::Positions& positions,bool allowLeft,bool allowRight);
void CloseControllers();
void ControllerStatus(char* buffer,size_t length);

#include "BodyVisibility.hpp"
bool ApplyControllerMarkerPalette(DriverPalette& palette);

void ResetControllerMarkerFrame();

void SetControllerSceneXRFrame(const float* reference,const float* current);
void SetControllerSceneCamera(float x,float y,float z);
bool SetControllerVehicleCamera(DriverIK::V cabin,DriverIK::V head);

void SetIgnitionInteractionTarget(DriverIK::V point);
bool ConsumeControllerIgnitionRequest();

void SetHornInteractionTarget(DriverIK::V point);
bool ControllerHornBoxHeld();

int ControllerBoxSetupKey(bool held);

float ControllerWindowDirection(unsigned side);

bool ConsumeControllerLightsRequest();

bool ConsumeControllerWipersRequest();

#include "RadioInteraction.hpp"
ControllerRadioCommand TakeControllerRadioCommand();

void QueueControllerRadioTuneSound();

void QueueControllerRadioClickSound(bool playing);

void PollControllerMenuMouse(bool menu);

void PlayVRRecenterFeedback();

void SetControllerVehicleIdentity(const char* brand,const char* model);

VehicleSteeringPose SnapshotVehicleSteeringPose();

bool ConsumeControllerDisplayRequest(unsigned action);

bool ConsumeControllerCameraPosition(DriverIK::V& position);
