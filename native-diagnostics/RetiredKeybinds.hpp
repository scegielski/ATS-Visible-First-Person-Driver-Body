#pragma once
inline constexpr const char* RetiredControllerKeybinds[]{"Move.reset","Move.down","Move.left","Move.right","Move.up","Setup.EditInteractionBoxes","Select.center","Select.cycle","Select.left","Select.right","Select.seat","Select.steering_extension","Select.steering_tilt"};
// SPF removes an action only if it owns it. Claim metadata first so saved actions
// absent from the new manifest are removed from merged config and the input UI.
template<class Claim,class Remove> void RetireControllerKeybinds(Claim claim,Remove remove){for(const char* action:RetiredControllerKeybinds){claim(action);remove(action);}}
