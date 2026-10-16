# Controls and configuration

See the [README](../README.md) for installation and supported scope.

## Driver body and vehicle support

On the supported ATS build, enabling the plugin renders the game-owned player driver in the cabin without selecting a body accessory. The native vehicle animation supplies the driver's seated placement. The old accessory body is suppressed while the native driver is available. Native object ownership stays with ATS, including vehicle switching and shutdown.

Setup boxes use the independent VR renderer and do not require a body accessory or mesh. New vehicle profiles start with a four-column, three-row grid of ten-centimetre cubes; the row heights are 1.60, 1.36 and 1.12 metres in vehicle space, with depth -0.40 metres. Hand grab boxes also default to ten-centimetre cubes. Existing calibrated profiles are preserved. The twelve cabin boxes are available without a driver; the two hand grab boxes additionally need a valid live wrist pose. Box edits save to the active vehicle's profile.

## Calibration and hand tracking

Sit normally and use ATS's assigned head-reset control to establish neutral head and torso tracking. Point the left controller at the left hand, or the right controller at the right hand, and press trigger once to engage it. Touching the corresponding grab box also works. Calibration captures that controller's current pose and the hand's current position; subsequent movement is relative to that grab.

Double-click that controller's trigger to release the engaged hand without targeting a box. The activation click is excluded from the release gesture. Thumbstick clicks no longer engage, release or recalibrate hands. Each hand operates independently; engagement and release retain their sound and vibration feedback.

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

Double-press X on the left controller for Enter, or Y for Escape, in menus or in the cabin while ATS has focus. The two presses must be within 500 ms. Holding a button does not repeat the key. Accepted Enter and Escape shortcuts play distinct keyboard clicks. Double-press B on the right controller to request ATS head reset and refresh seated body calibration. A spoken "VR recenter" cue confirms the reset from a fresh headset pose, including the title screen; it does not wait for the driver body. This uses the native game action after SPF hook activation and does not depend on the keyboard binding. Cabin finger controls are unchanged.

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

Press F9 to show the labeled boxes and suspend their actions. Grip near a box to move and rotate it. Grab with both hands to resize along the local axis closest to the hand separation. Release and re-grab along another direction to resize a different dimension. The L HAND and R HAND grab boxes follow the wrist bones. Edit their placement and nonuniform size like the other boxes; their saved positions and rotations are local to the wrists. Release to save; press F9 to return to normal play.

## Configuration files

Paths below are relative to the dedicated SPF plugin folder, `bin/win_x64/plugins/spfPlugins/SPF_VisibleFirstPersonDriverBody`.

| File | Purpose |
| --- | --- |
| `config/mustang_poses.ini` | Live finger, gesture, ignition and turn-signal poses |
| `config/vehicles/<brand_model_hash>_poses.ini` | Live per-vehicle resting hand offsets and native head visibility/backshift |
| `config/vehicles/<brand_model_hash>_boxes.ini` | Active vehicle box positions, rotations and sizes; selected from game brand/model IDs; F9 saves here |
| `config/mustang_boxes.ini` | Preserved Mustang seed; copied into its vehicle profile on first use |
| `config/settings.json` | SPF-owned preferences and keybindings |

General tracking, timing, visibility, elbow and wrist settings are compiled into the DLL. There is no installed `driver_body.ini`. Pose edits reload in about half a second; invalid edits retain the last valid pose. The INI section names and formats must be preserved. Updates retain existing Mustang configurations.

Double-press right controller A to toggle cabin setup mode (F9 remains available). Left X retains finger selection; right A is reserved for setup.

Changing setup mode through double-press A or F9 speaks “Setup mode enabled” or “Setup mode disabled”.

Radial menus use eight segments with a 2048-pixel antialiased texture with smoothly rendered Segoe UI labels. Press and hold a controller thumbstick button to open its ring, tilt while holding to highlight an octant, then release the button to select that octant. Release at center to cancel. Tilting without holding does not show a menu. Selection gives vibration.

The left ring right segment toggles Finger editing mode, which defaults off and announces enabled or disabled. While editing, X/A select fingers and Y/B toggle the corresponding trigger assignment, speaking Enabled or Disabled with vibration. Exiting editing retains assignments. Y/B thumb closure, double-B recenter, double-A setup and X/Y keyboard shortcuts are suspended while editing to prevent conflicts; normal controls return when editing is off.

While a radial menu is held open, newly highlighted octants play a short note. Left uses C major; right uses C natural minor. Both ascend clockwise from tonic at top to the upper octave. Holding one octant does not repeat; center is silent.

The left radial top segment (MAP) requests ATS’s native navmap binding to toggle the world map, independent of the keyboard mapping. Hold left thumbstick click, tilt up, and release; repeat to close.

Right radial left: QUICK INFO (F1). Right radial top: SERVICES AND ADJUSTMENTS (F7). Hold stick click, tilt to the segment and release; each selection requests the corresponding native advmouse or services action without generating keyboard input.

Right radial native actions: top Services and Adjustments; upper-right Activate; right HUD Widgets; bottom Next Camera; lower-left Interior Camera; left Quick Info. All use game action bindings rather than keyboard events. HUD toggles visibility; camera cycling follows the native game order and availability.

Activate uses SPF semantic virtual input registered during plugin OnLoad. It emits a short activate press/release through the game input system and does not depend on a primary keyboard binding. Startup must have the plugin enabled for device registration. The other radial actions retain their validated native binding hooks.

Left radial lower-right: SETUP MODE ON/OFF. Select it to toggle box setup with the existing spoken cue and vibration. F9 and double-press right A remain available.

Left radial lower-left: VR RECENTER. Select while seated normally; it uses the existing native reset and speaks VR recenter after the reset applies. Double-press right B remains available outside Finger editing mode.

Vehicle box profiles are created automatically when driving telemetry identifies a vehicle. A new non-Mustang vehicle starts with generic boxes: use setup mode to position them. Profiles include all cabin boxes and the two wrist-local grab boxes. Switching vehicles releases hand control and clears pending cabin interactions. Profile edits reload live; invalid files retain the last valid placement. This change covers boxes only: poses and body alignment still use the Mustang configuration. Profiles created at runtime are not overwritten by installer updates.

Setup boxes now render through the controller bridge, independently of the driver-body accessory. The bridge and plugin must be updated together. New vehicles start with twelve labeled cabin boxes arranged in front of the seat for placement with F9 or the controller setup toggle. Box grabbing, rotation, nonuniform scaling and saving work without a body. The two wrist-attached boxes appear only when a real driver skeleton is available. Outside setup mode, controller lasers can operate the configured cabin boxes without a driver body.

The driver body reuses the game-owned native player model and draws it with the interior render context; no accessory is required. The active build does not create or release native body scene objects.

Setup meshes are anchored in OpenXR LOCAL tracking space and rendered using display-time eye poses, rather than game-update head-relative poses. The compositor pass uses the headset-recommended resolution (up to 4096 pixels per dimension) and four-sample antialiasing when supported.

### Live resting-hand placement

Each vehicle gets a separate poses file next to its boxes file. Under `[steering_grips]`, edit `left_offset` and `right_offset`, each with three metre values: X right, Y up, Z backward toward the driver. For example, `left_offset 0 0 0.02` moves the resting left hand two centimetres back. Keep hands disengaged while checking resting wheel grips; re-engage afterward to capture the adjusted grip.

The `[head]` section contains `visible 1` and `backshift_m 0`. Set `visible 0` to collapse the head in the interior camera, or use a small backshift to reposition it instead. Head visibility defaults on. Changes reload within about half a second; malformed edits retain the last valid settings. These offsets supplement the vehicle's own native steering pose; existing finger/gesture definitions still come from `mustang_poses.ini`.

Controller-to-body mapping uses the active vehicle's reported cabin and head origin plus the seat adjustment. Existing box coordinate values remain unchanged when that camera basis changes; they may need repositioning in setup for the corrected vehicle frame.

### Bronco VR validation

The user confirmed accessory-free body visibility, normal shutdown, continuous hand updates, controller-to-hand tracking, sharp and stable independent boxes, and the corrected hand-box rotation and readable labels. Bronco resting hand offsets were tuned live to two centimetres up and five centimetres back and are saved in its vehicle pose profile. The final hand-box transform correction passed all seven native test suites and installer lifecycle tests.

### Dashboard and infotainment boxes

DASHBOARD and INFOTAINMENT each cycle the corresponding native game display mode once per trigger press while touched by an engaged hand or targeted by a controller laser. No keyboard binding is required. Place and resize them in setup mode; their placements save in the active vehicle box profile. Existing profiles keep their saved placements and gain two new ten-centimetre boxes in a fifth grid column: Dashboard to the right of Center Mirror and Infotainment to the right of Right Window.

The controller infotainment cycle inserts the native Parking Cameras action when the ordinary mode sequence wraps. Selecting the infotainment box again while parking cameras are active requests the native camera toggle instead of advancing a camera subview. Native camera availability and driving restrictions remain controlled by ATS. This extension requires the verified native infotainment-state hook.

While targeting INFOTAINMENT, nudge either controller thumbstick up to cycle the native navigation zoom mode. Return the stick to center between changes; setup mode and an open radial menu suppress this shortcut.
