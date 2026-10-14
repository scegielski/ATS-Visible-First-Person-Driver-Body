# ATS Visible First Person Driver Body

Plugin output and SPF identity: `SPF_VisibleFirstPersonDriverBody` (SPF Visible First Person Driver Body), version 0.22.2. Renaming an existing installation requires moving its calibrated config files, enabled setting, and early OpenXR bridge registration to the new plugin directory. The ATS VR Controller Hands shortcut uses the migrated launcher.


Current validated release: **0.18.9 plugin + 0.7.0 body archive**. The user confirmed the intact head works in first-person VR without collapse or offset. Our animated body replaces only the player's stock exterior driver, leaving traffic instances outside the hide guard.

The original 61-bone driver mesh supports live steering and re-grabs, accelerator/brake/clutch motion, game-bound horn and turn signals, ignition with torso lean, runtime arm IK and engine starting delayed until the hand reaches the key, and independent hand gestures. Live OpenXR head tracking drives seated torso lean in interior and exterior views, with arm IK retaining hand targets and neck/head rotation following the headset in both camera views. Pose files, elbow offsets, turn-signal timing, tracking strength and lean limits are editable live. The head is restored from the original installed game mesh; the default interior head remains intact and tracked with zero offset. Optional collapse/offset controls use the corrected row-major render palette. Materials and textures remain references to installed game assets. The Mustang is the validated vehicle; broader calibrations remain planned.

Controller hands support per-hand thumbstick calibration, grip/trigger finger curl and live editable open/closed poses and finger routing through an early in-process OpenXR bridge. Both hands were visually validated; intermittent elbow flips remain a known limitation. See [CONTROLLER_IK.md](CONTROLLER_IK.md) for dependencies and the required controller-enabled launcher.

## Install

With ATS closed, install `releases/SPF_DriverAnimationDiagnostics-0.18.9-live-finger-routing-test.dll` under the existing `SPF_DriverAnimationDiagnostics.dll` name in the SPF plugin folder. Enable the plugin in SPF's Plugins panel. Replace the current body archive with `releases/ATS-VisibleDriverBody-Mustang-0.7.0-exterior-head-test.scs`, preserving the existing installed filename if the profile uses it. The current installation retains `ATS-VisibleDriverBody-Mustang-0.1.0.scs`; do not enable a duplicate archive.

Copy the current `signal_left_pose.txt`, `signal_right_pose.txt`, `signal_activation_delay.txt` and `signal_actuation_seconds.txt` from releases into the plugin config folder. Also copy `head_tracking_gain.txt`, `head_tracking_limits.txt`, `interior_head_mode.txt` and `interior_head_backshift.txt` into config. The approved head settings are `visible` and `0.00`. Keep the existing gesture, ignition and elbow files. Calibrate in the normal seated position using your usual ATS reset-head-tracking control, then switch views as desired. Forward/side range and spine-angle settings have no arbitrary upper caps; physical arm reach still constrains the resulting pose. Add `body_visibility.txt`: `replace-exterior` shows our full body outside and hides only the confirmed player stock driver; `interior` shows our body only in the interior camera. This setting reloads live.

See [NATIVE_RECENTER.md](NATIVE_RECENTER.md), [HEAD_TRACKING.md](HEAD_TRACKING.md), [TURN_SIGNAL_DELAY.md](TURN_SIGNAL_DELAY.md), [ENGINE_START_DELAY.md](ENGINE_START_DELAY.md), [BODY_VISIBILITY.md](BODY_VISIBILITY.md), [EXTERIOR_HEAD.md](EXTERIOR_HEAD.md), [IGNITION.md](IGNITION.md), [TURN_SIGNALS.md](TURN_SIGNALS.md), and [LEFT_ELBOW.md](LEFT_ELBOW.md). Earlier release sections below are historical checkpoints; the 0.6.1 pedal alignment archive remains an uninstalled checkpoint.

## Interaction setup

Use the controller-enabled launcher. Press F9 to edit the labeled ignition and horn volumes; grip near a box to move and rotate it. Both grips resize the local axis closest to controller separation. Release and regrab from another direction to edit a different dimension. Releasing saves config/interaction_boxes.txt; F9 exits. Interaction boxes hide and normal horn/ignition actions resume outside setup. Either tracked hand contact honks; ignition requires a right-trigger click. Current paired archive: releases/ATS-VisibleDriverBody-Mustang-0.10.2-labeled-interaction-boxes-test.scs.

## Source and validation

- build_stock_variant.py reads installed stock driver assets, hides head faces, writes a private descriptor and creates a neutral animation clip.
- make_locator_only_mount.py writes the empty animated actor mount with attachment-transform compensation.
- stock_assets.py is the read-only SCS archive reader used for asset lookup.
- generate_body.py creates the independent procedural fallback.
- diagnose_stock_rig.py reconstructs stock bind matrices and pose transforms for analysis only; it does not bake the shipping body into static geometry.
- test_stock_animation.py validates PMA v5 pointers, bone mapping, neutral frames and rejection of the prior malformed header.
- stock_driver_validation.json records checks and the user's successful 0.2.4 VR test.

The main visibility defect was the generated PMA header: data offsets start at byte 64, but the earlier writer edited byte 72 onward. The corrected hold clip declares arrays at 84/92/153 and preserves the original pose/rig mapping. See ANIMATION_FIX.md and TIFA_COMPARISON.md for the diagnostic evidence.

## Build prerequisites

Python with NumPy is needed for rig analysis. The existing workspace provides extracted Ford assets at ../extracted-ford and stock-reference/driver.pmg plus ford_mustang_67_ani.pma for diagnostics. These game/reference files, downloaded SCS conversion tools and Blender reference checkout are excluded from Git. SCS Conversion Tools 2.21 converts the original fallback source in tools/base/model/spf_driver_body. Run build_stock_variant.py afterward to regenerate the private stock avatar and locator-only mount; run test_stock_animation.py to check the clip. This checkpoint includes the exact verified package and generated model outputs, rather than depending on an untested rebuild.

No external reference mod is installed by these scripts. The project is hosted in the private GitHub repository scegielski/ATS-Visible-First-Person-Driver-Body.

## Live steering validation

See STEERING_INTEGRATION.md and live_steering_validation.json. Runtime observations identified the private interior model and exterior driver using the same animation advancement routine. The plugin reproduces the exterior reset/enable/advance sequence only for the private model with the 47-frame steering clip. An exact executable SHA-256 guard prevents enabling these internal hooks on another game build. Regression tests cover argument and return forwarding, selective float replacement, steering mapping and rejection of the seated-hold clip.

build_steering_prototype.py generates the steering archive from the unchanged package sources and original stock animation. The user confirmed live steering, good wheel alignment and natural hand re-grabbing in first-person VR. Reload regression and future game builds have not been validated. To roll back, restore the 0.2.4 archive and remove or disable the steering plugin with ATS closed.

## Accelerator foot validation

See THROTTLE_INTEGRATION.md and live_throttle_validation.json. build_pedal_prototype.py generates 33 independently selectable throttle poses with the same steering tracks at every level. Only the right ankle is rotated, up to 12 degrees downward. The PMA duration header is corrected for the expanded 1551-frame clip. The user confirmed the foot test works in first-person VR. Offline pose/track checks and native forwarding, steering/throttle mapping and clip-guard tests pass.

## Brake, clutch and final alignment

See BRAKE_INTEGRATION.md, PEDAL_ALIGNMENT.md, CLUTCH_INTEGRATION.md and live_pedals_validation.json. build_brake_prototype.py authors the current right-pedal poses and static left-leg shift; build_clutch_prototype.py adds independent clutch levels and produces the final combined archive. Tests cover pedal sequencing, priority, independent clutch selection, bounds, invalid input and argument/return preservation. Offline pose checks verify unchanged steering/gas tracks, leg lengths, target positions, downward ankle motion, PMA metadata and archive round trips. The combined animation uses about 120 MB uncompressed; the archive is about 52 MB. The final two-centimetre rightward brake adjustment remains awaiting a visual check.

## Left-hand horn animation

The installed and user-validated horn plugin is **0.7.2 game horn test**. Use any ATS horn binding to release the left grip, reach with an open palm, press the horn and return to the current steering grip. The right arm continues steering and the independent pedals keep playing. The plugin reads ATS's own horn state; no separate SPF binding is needed. The user's secondary binding is confirmed working.

The builder uses two-joint IK to generate compact arm targets; the runtime plugin blends selected left-arm and finger rotations before skeleton hierarchy assembly. Final offsets from the original wrist midpoint are 9.5 cm down and 0.5 cm left, at the original depth. The palm aligns with the estimated wheel plane. The final VR alignment was confirmed good by the user. See HORN_INTEGRATION.md and the horn validation records.

Install releases/SPF_DriverAnimationDiagnostics-0.7.2-game-horn-test.dll as SPF_DriverAnimationDiagnostics.dll in the existing SPF plugin directory with ATS closed. The horn works with the currently installed 0.6.0 pedal archive and requires no archive expansion. The previously committed 0.6.1 archive with 14 cm brake reach remains built but not installed or visually validated.

The latest installed input update is the **0.7.2 game horn test** plugin. It reads the player's validated game horn state, so all ATS horn bindings drive the same gesture without an SPF-specific keybind. Both bindings were correlated with the field, and the user confirmed the secondary-binding animation works. See game_horn_discovery.json for validation and guards.

## Middle-finger gesture and live pose editing

The installed and user-validated **0.10.0 right gesture test** plugin supports a held middle-finger gesture above the steering wheel. Default F10 can be reassigned through SPF's existing Gestures.LeftWindowBird action. The hand reaches with the middle finger down, raises it after arrival, and returns directly to the current steering grip on release. Gesture transitions run 1.5 times faster than the first prototype. The game-state horn takes priority, while steering and pedals remain independent.

The current pose shares axial rotation between forearm and wrist, keeps a natural bend in the raised finger, and uses the user-confirmed thumb pose. The user confirmed the final sequence works great. See WINDOW_GESTURE.md and the validation records.

The plugin watches config/gesture_pose.txt in its own directory and hot-reloads valid saves within about half a second. The version-2 file contains reach, intermediate finger-down and final held quaternions for 21 left-arm/finger bones. Invalid or incomplete edits retain the last valid pose. No rebuild or game restart is needed for pose edits. Timing logic changes still require a plugin update. See LIVE_POSE.md for format, scope and validation.

Install releases/SPF_DriverAnimationDiagnostics-0.10.0-right-gesture-test.dll as SPF_DriverAnimationDiagnostics.dll in the existing SPF plugin directory with ATS closed, and copy releases/gesture_pose.txt into its config directory. Preserve customized pose files when upgrading. The current body archive remains unchanged.

## Independent right-hand gesture

The user-validated **0.10.0 right gesture test** plugin adds an independent F11 right-hand gesture alongside the F10 left-hand gesture. SPF actions Gestures.RightHandBird and Gestures.LeftWindowBird can be rebound. Both hands have finger-down arrival and direct steering return, with independent live config/right_gesture_pose.txt and config/gesture_pose.txt files. Side-specific validation prevents either file from targeting the other arm.

The final gesture targets are three inches left for the left hand and three inches right for the right hand, measured from their previous centered targets. The user confirmed both look good, and both live reloads were recorded. Include both release pose files when installing this plugin. The DLL's original fallback poses are centered; the live files carry the validated offsets. See RIGHT_GESTURE.md and the validation records.
