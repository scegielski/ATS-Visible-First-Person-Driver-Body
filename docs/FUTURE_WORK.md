# Future work: controller driving and broader ATS VR support

This is a planning document, not a list of implemented features. The current working baseline includes tracked hands, interactive cabin boxes, menu pointers, and radial menus. The next goal is driving with motion controllers without a physical steering wheel or keyboard.

## Controller Driving Mode

Add a Driving Mode toggle to the unused bottom segment of the left radial menu, with a visible state and spoken enter/exit confirmation. Start with the mode off. Proposed mappings, still subject to testing:

| Controller input | Proposed driving action |
| --- | --- |
| Left thumbstick horizontal | Steering |
| Right thumbstick up/down | Shift up/down, once per nudge with a return to center |
| Right trigger | Accelerator |
| Left trigger | Brake |
| One grip, side to be decided | Clutch |

Use the game's semantic input channels where supported, rather than keyboard simulation. SPF's semantic virtual-input API is already validated for Activate; steering, pedals, clutch, and shifting still require implementation and testing. Register any semantic device during the framework's required startup phase.

Driving Mode owns driving inputs while active. Suspend conflicting finger editing, hand engagement/release gestures, cabin trigger actions, grip-based box editing, and menu mouse input as appropriate to the current context. Define how hand poses respond without letting finger animation accidentally operate vehicle controls.

Keep radial menus accessible: holding thumbstick click opens a ring and temporarily suspends driving input from menu navigation. Releasing selects an item or cancels at center. Require a deliberate return to neutral before resuming driving input, particularly accelerator and brake triggers. The mode must always have a clear, usable exit.

## Tracking loss and transitions

- Release synthesized buttons and accelerator/clutch input on controller loss, focus loss, plugin disable, world unload, or exit from Driving Mode.
- Choose and test steering and braking behavior for tracking loss; do not assume an abrupt steering reset or automatic full braking is appropriate.
- Prevent stuck inputs and unintended acceleration across pause screens, radial menus, reconnects, and device handoffs.
- Keep physical wheel/pedal inputs usable and define precedence when physical and virtual inputs are present together.

## Vehicle profiles beyond the Mustang

Use the active vehicle identity reported by the game to select separate configuration files automatically. Do not select profiles solely by a localized display name or the currently visible body mesh. Investigate which game/SDK identity fields reliably distinguish stock vehicles and mod vehicles before finalizing the lookup key.

Each vehicle profile should own its vehicle-specific data:

- Driver alignment and any vehicle-specific rig or mounting information.
- Steering-wheel grips, reach targets, and vehicle-specific hand/gesture poses.
- Cabin interaction-box positions, rotations, and sizes.
- Hand grab-box offsets and other collision offsets that need vehicle-specific calibration.

Keep the Mustang's validated defaults as its own profile. Preserve the existing distinction between pose data and box placements; the exact per-vehicle filename or directory convention can be chosen during implementation. Maintain a stable mapping from game identity to profile files so two vehicles with similar names cannot overwrite each other's settings.

F9 setup edits must save to the active vehicle's files. Switching vehicles or loading a different save must switch profiles and clear transient calibration and interaction state. Do not write one vehicle's placements into another profile. For an unknown vehicle, clearly report the missing profile and provide a calibration/setup path rather than silently applying Mustang positions.

Separate distributed defaults from user-edited placements, preserve edits during installer updates, and support adding a vehicle profile without rebuilding the DLL. Validate at least one additional vehicle, then switch repeatedly between it and the Mustang and confirm persistence after restarting ATS.

## Complete the essential driving controls

Audit controller access to indicators, hazard lights, parking brake, reverse/neutral, cruise control, lights/high beam, and any transmission-specific controls. Existing cabin and radial actions should remain reachable in Driving Mode. Decide which actions belong in a radial menu versus a physical-looking cabin interaction, and label them clearly.

## Headset-only acceptance walkthrough

Test an entire session without reaching for a keyboard, mouse, wheel, or pedals:

1. Launch ATS and select the profile/save.
2. Navigate menus, choose a job, and interact with service/job prompts.
3. Enter Driving Mode, steer, accelerate, brake, shift, reverse, and park.
4. Use mirrors, lights, wipers, windows, radio, and other essential controls.
5. Open/close the map and other menus, including click and drag interactions.
6. Recenter, recover from controller tracking loss, and resume deliberately.
7. Save, return to the title screen, and quit.

Record missing controls and awkward transitions from this walkthrough before expanding the feature list. Repeat it on more than one vehicle and with a fresh installation.

## Product scope and naming

Revisit the repository and product name after controller driving and the headset-only workflow are validated. “ATS VR Conversion” is a candidate name, not a rename request for the current implementation. Broader vehicle support and a clear account of remaining limitations should accompany any claim of a total conversion. Vehicle exit/walking remains a separate exploratory feature.
