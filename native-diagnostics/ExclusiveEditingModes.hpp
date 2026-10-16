#pragma once
#include "InteractionBoxEditor.hpp"
#include "ControllerRadial.hpp"
inline int ToggleExclusiveBoxSetup(InteractionBoxEditor& boxes,ControllerFingerSetupMode& fingers){int result=boxes.Toggle();if(boxes.enabled)fingers.enabled=false;return result;}
inline int KeyExclusiveBoxSetup(InteractionBoxEditor& boxes,ControllerFingerSetupMode& fingers,bool held){int result=boxes.Key(held);if(result&&boxes.enabled)fingers.enabled=false;return result;}
inline bool SelectExclusiveFingerSetup(InteractionBoxEditor& boxes,ControllerFingerSetupMode& fingers,unsigned side,int segment){if(!fingers.Select(side,segment))return false;if(fingers.enabled&&boxes.enabled)boxes.Toggle();return true;}
