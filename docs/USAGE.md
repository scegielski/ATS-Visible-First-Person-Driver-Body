# Controls and configuration

See the [README](../README.md) for installation and supported scope.

## Calibration and hand tracking

Sit normally and use ATS's assigned head-reset control to establish neutral head and torso tracking. Hold each controller at that hand's wheel grip, then click its thumbstick to calibrate and enable hand IK. Later clicks toggle that hand; holding the click recalibrates. Each hand operates independently. Engage/disengage produces a tone and vibration.

Controller models and white laser pointers appear in the cabin while that hand's IK is disabled. Enabled hands use contact targeting. Targets outside arm reach can remain beyond the wrist even though the controller continues moving.

## Fingers and gestures

| Control | Behavior |
| --- | --- |
| X left / A right | Select index, middle, ring or pinky; spoken name and brief flex preview |
| Left stick right / right stick left | Add or remove the selected finger from trigger control; return stick to center between toggles |
| Trigger | Curl assigned fingers; index is assigned by default |
| Grip | Curl the remaining non-thumb fingers |
| Y left / B right | Close that thumb while held |
| F10 / F11 | Left / right bird gesture; follows calibrated head yaw and pitch |

Finger assignment changes blend smoothly. Cabin actions can also use the trigger while their target is selected.

## Menu pointing

Point either controller at the VR menu. The cursor follows the selected laser hit; trigger left-clicks or left-drags outside the world map; on the detected world map, release without dragging to left-click, or hold and move to right-drag; grip right-clicks or right-drags, and the thumbstick scrolls. Either button over the panel selects that controller; ownership stays fixed during a drag. Hold trigger and move the controller to drag the world map; grip also supports right-drag. Map right-drag requires a 150 ms hold and an eight-pixel motion threshold. A completed drag does not emit a left click on release. Trigger routing is chosen on press and held until release. The former grip-activated fallback air mouse has been removed. Menu support starts through the early OpenXR bridge before normal SPF activation.

## Cabin controls

Outside setup, touch a box with an enabled hand or target it with a disabled hand's laser. Boxes and labels are hidden during normal play.

| Target | Input |
| --- | --- |
| Ignition | Fresh trigger press advances the native electricity/start/stop sequence |
| Horn | Hold trigger while targeting; release or leave to stop |
| Left / center / right mirror | Joystick adjusts the selected mirror; servo sound accompanies movement |
| Seat | Joystick adjusts height and fore-aft position |
| Left / right window | Joystick up closes, down opens |
| Wheel | Joystick left/right telescopes; up/down tilts |
| Lights | Fresh trigger press cycles light states |
| Wipers | Fresh trigger press advances speed, wrapping from maximum to off |
| Radio | Left/right stick nudges change station; up/down changes volume; trigger toggles playback |

## F9 setup editor

Press F9 to show the labeled boxes and suspend their actions. Grip near a box to move and rotate it. Grab with both hands to resize along the local axis closest to the hand separation. Release and re-grab along another direction to resize a different dimension. Release to save; press F9 to return to normal play.

## Configuration files

Paths below are relative to the dedicated SPF plugin folder, `bin/win_x64/plugins/spfPlugins/SPF_VisibleFirstPersonDriverBody`.

| File | Purpose |
| --- | --- |
| `config/mustang_poses.ini` | Live finger, gesture, ignition and turn-signal poses |
| `config/mustang_boxes.ini` | Live box positions, rotations and sizes; F9 saves here |
| `config/settings.json` | SPF-owned preferences and keybindings |

General tracking, timing, visibility, elbow and wrist settings are compiled into the DLL. There is no installed `driver_body.ini`. Pose edits reload in about half a second; invalid edits retain the last valid pose. The INI section names and formats must be preserved. Updates retain existing Mustang configurations.
