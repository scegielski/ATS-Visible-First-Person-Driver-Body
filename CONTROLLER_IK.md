# Controller hand IK

The user validated both hands in the Mustang with the 0.18.4 build. An early OpenXR API layer registers Oculus Touch thumbstick actions before ATS attaches its session action sets. The SPF plugin reads those actions and polls SteamVR controller poses through OpenVR, then applies runtime arm IK after torso compensation. All components run inside ATS; there is no companion process. Other OpenXR applications are passed through by the layer. This route is validated with SteamVR and Oculus Touch controllers only.

Press each thumbstick to calibrate and enable that hand. Hold the controller at the intended wheel grip: its position/orientation and headset heading align with the current animated wrist without jumping. Later clicks toggle the hand; holding for 0.7 seconds recalibrates and enables it. Re-enabling calibrates again. World changes disable both hands and clear calibration. Activation/release blend over 0.18 seconds. Lost tracking fades back to steering. Targets clamp to the arm envelope without stretching bones; fingers retain their animations. Existing gestures, horn, signal and ignition temporarily take priority on their respective arms.

## Installation and launch

With ATS closed, install the matching plugin DLL, spf_controller_openvr.dll and the license files into the existing SPF plugin folder. Install controller-layer/spf_controller_xr_layer.dll and its JSON manifest together there. Register the manifest's absolute path as a DWORD value of zero under HKCU\SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit. Retain other registered layers.

Install controller-layer/openxr_loader.dll in the ATS bin/win_x64 directory and set o_openxr_external_loader to 1 in the game config, backing up both beforehand. The official loader is Khronos 1.1.63. Keep the existing body archive and calibrated live pose files.

The working startup explicitly enables XR_APILAYER_SPF_driver_controller_input and supplies the Steam application identity. Use Launch-ATS-ControllerHands.ps1 with Steam already running. Normal Steam launch did not activate the bridge in the comparison; routine Steam launch integration remains pending. The launcher affects only the child ATS process. To roll back, set the external-loader option to 0 and disable/remove only this layer registration. Local backups are in backups/controller-ik.

## Validation and limitation

Regression tests pass for all steering frames, both arms, calibration, toggle/hold edges, mapped wrist displacement, unchanged other joints and unreachable target clamping. Layer tests preserve ATS action sets and synchronization inputs, verify per-hand boolean states and reject unfocused input. Runtime logs captured over eight thousand left-hand and six thousand right-hand solves, with zero rejected solves and zero caught pose faults. The user confirmed both controller hands work.

The current elbow guide uses the animated elbow plus a small inward/downward offset. The user reports intermittent wrong-way bends. This is the working checkpoint before elbow-stability changes. Finger tracking is not implemented.

References: [Valve pose query](https://github.com/ValveSoftware/openvr/wiki/IVRSystem::GetDeviceToAbsoluteTrackingPose) and [Khronos loader layer documentation](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/specification/loader/api_layer.adoc).

## Elbow-stability checkpoint (0.18.5)
The new solver uses a torso-relative down/out/back guide and transports the prior elbow bend along the changing shoulder-to-wrist ray. It limits guide rotation to 180 degrees per second and retains continuity when the guide is nearly parallel to the reach. The controller hand target and wrist orientation are preserved. config/controller_elbow_guides.txt reloads every half second; left/right lines contain finite nonzero model-space guide vectors. Tests cover continuous reach sweeps across all steering frames, wrist position/orientation preservation, and no instantaneous pole reversal. The user requested this checkpoint be committed after using it. Runtime health shows both hands active, thousands of solves and zero rejected solves or caught pose faults. Forearm twist sharing is a separate next change.

## Axial twist sharing checkpoint (0.18.6)
Controller hand roll about the forearm axis is decomposed relative to the calibrated wrist. Half is moved into the forearm's local rotation and the inverse applied at the wrist, preserving the hand's model-space position and orientation. The share fades with controller activation/release. The angle is unwrapped across half turns to avoid discontinuity. Only forearm/wrist local rotations change; finger rotations remain unchanged. config/controller_forearm_twist_share.txt reloads twice a second and accepts 0 through 1, initially 0.50. Zero leaves all roll at the wrist; one places all axial roll in the forearm. Tests cover both arms and all steering poses with continuous 0-to-200-degree rolls, unchanged wrist targets and exact half share. The user requested this version be committed after the runtime test. Health recorded 7245 left and 6763 right solves with zero rejected solves and zero caught pose faults.

## Controller finger controls
Enabled controller hands default to the original mesh's open bind finger pose. Grip controls the thumb, middle, ring and little fingers; trigger independently controls the index. Closed poses reuse the prepared finger-down gesture joints. The early layer now adds per-hand squeeze/value and trigger/value float actions to its existing thumbstick actions. Values are clamped to 0..1 and smoothed over 40 milliseconds, then blended with hand activation/release. Scripted actions retain their existing priority. Tests verify grip/trigger independence, open rest, preservation of arm/body/opposite-hand joints, exact disabled-pose forwarding and per-hand analog action reads. Plugin and early layer are a matching installed pair; runtime visual validation remains pending.

## Live controller finger poses

The plugin watches config/controller_left_fingers.txt and config/controller_right_fingers.txt twice a second, outside the animation hook. Each file contains the open and closed local rotations for all 15 finger bones. Valid saves publish an immutable snapshot used by the next controller finger blend. Invalid, missing, incomplete, duplicate, opposite-hand, nonfinite or badly normalized edits retain the last valid pose. Missing initial files use the compiled defaults. The two sides reload independently; input routing stays fixed (index uses trigger, other fingers use grip).

Format: SPF_CONTROLLER_FINGERS 1, followed by 15 rows of `bone INDEX OPEN_W OPEN_X OPEN_Y OPEN_Z CLOSED_W CLOSED_X CLOSED_Y CLOSED_Z`. Comments start with #. Quaternion values near unit length are normalized; the file is bounded to 16 KiB. Left finger bones are 8 through 22; right are 29 through 43. In each side, groups of three joints are index, middle, ring, little, then thumb. These are local joint rotations, not positions. Finger shape changes do not alter IK hand targets, calibration, or wrist/forearm twist settings. Health logs include revisions and rejected-edit counts for each side.

Live resting-pose refinement: both open poses now interpolate gently toward their closed counterparts (finger joints 12%, 18%, 12%; thumb base 30%, middle/tip 15%). All closed quaternion values are preserved exactly. Updated installed and release live files; visual check pending.

Closed middle-finger alignment refinement: removed the gesture's extra off-axis rotation from all three middle joints, retaining the curl component about the original bind bending axis on both hands. Open poses, thumb settings, other fingers and input routing remain byte-identical. Applied via live files; mesh clearance needs the user's visual check.

The first middle-finger correction still intersected in the user's view. Rebuilt both closed middle chains from the original bind rotations with local joint curls of 85, 80 and 25 degrees (post-multiplied), removing the earlier gesture-direction compensation and the incorrect parent-axis curl. Applied through live files; awaiting the next visual check.

## Live finger routing checkpoint (0.18.9)
The default trigger finger is now middle; grip curls thumb, index, ring and little. Each live finger file accepts an optional trigger_finger row with index, middle, ring, little or thumb. All three joints of the selected finger use trigger; the remaining fingers use grip. Routing can change live after this one plugin update. Invalid or duplicate routing keeps the previous valid pose. The release files specify trigger_finger middle and remove off-axis rotations from closed ring/little joints following the user's overlap report. Build and regression tests pass; installation is awaiting ATS exit.

0.18.9 is installed with both live files routed to middle-trigger control. Pre-update plugin and live files are backed up. The existing early input bridge and controller-enabled launcher are unchanged. Visual test pending.

The user requested the complete current finger-control checkpoint be committed. Installed plugin, bridge and both live pose files match the packaged files. Runtime health recorded 4878 left and 4850 right solves, zero rejected solves, zero caught pose faults and zero rejected live files. Both initial live pose revisions were accepted. Grip controls all except middle; trigger controls middle, with the selected finger editable in each live file. Controller X/A mode buttons are a separate subsequent change.

## Controller X/A finger mode test (0.18.10, uncommitted)
The early input layer adds X on the left Oculus Touch controller and A on the right as independent boolean mode actions. Each rising press alternates that hand's trigger-controlled finger between middle and index; grip controls the remaining fingers. Holding a button does not repeat. Tracking recovery with the button held does not create another edge. Choices are independent, reset on world changes, and revert to the live file's routing when its pose revision changes. Pose rotations and thumbstick calibration remain unchanged. The plugin and early layer must be installed as a matching pair. Regression tests cover binding/action type setup, independent mode state, held-button suppression, tracking recovery and live-file override reset. Runtime visual validation pending.

### Finger mode transitions (0.18.11 test)
X/A now blend index and middle input routing over 0.18 seconds, independently per hand. Grip and trigger values remain live during the eased transition; retoggling continues from the current blend. The live `config/controller_finger_mode_blend_seconds.txt` setting accepts a positive finite duration in seconds and reloads every half second; invalid edits retain the previous duration. This plugin update uses the existing 0.18.10 input bridge. Visual validation pending.

### Independent thumb buttons (0.18.12 test)
Hold Y on the left controller or B on the right to curl that thumb; release to return to its live open pose. Thumb input replaces grip control for the three thumb joints and is smoothed like grip/trigger. X/A finger mode switching remains independent. Requires the paired early input bridge update and one restart. Visual validation pending.

### Four-finger cycle (0.18.13 test)
X/A independently advance trigger control through index, middle, ring, and little finger (wrapping to index). Initial selection comes from each live pose file, currently middle, so the first press selects ring. Grip controls the remaining three fingers; Y/B retain separate thumb control. Mode changes retain the live eased blend duration. Tests cover both sides, full wraparound, held-button suppression, tracking recovery, and rapid cycling. Visual validation pending.

### Finger cycle correction (0.18.14 test)
User visual testing identified the ring/little groups as reversed in the assumed skeleton order. Cycle now uses joint groups 0, 1, 3, 2 to produce the visible index, middle, ring, pinky order on each hand. Status labels corrected accordingly. Smooth routing and thumb buttons unchanged. Visual recheck pending.

### Multiple trigger fingers (0.18.15 test)
X/A select the finger without changing current membership. Push the left stick right or the right stick left to add/remove that selected finger. An inward value above 0.7 toggles once; return below 0.3 to rearm. Tracking loss suppresses a held-stick recovery edge. A higher beep means added and a lower beep means removed. Selected members follow trigger together; others follow grip, and Y/B still control thumbs. Smooth live routing transitions retained. Paired input bridge update requires one restart. Visual validation pending.

### Visible X/A cycling restored (0.18.16 test)
X/A move the selected finger membership to the next finger while preserving other assigned fingers. Inward stick still toggles the selected finger membership. This restores visible cycling from the default single-finger mode; when the selected finger is unassigned, cycling only moves the selection until it is added. Regression covers preserving an additional middle-finger assignment. Visual recheck pending.

### Controller vibration feedback (0.18.17 test)
Adds native OpenXR haptic actions for both controllers. A successful X/A cycle or inward-stick membership toggle requests one 60 ms pulse at 60% amplitude on that hand; membership beeps retained. Paired bridge update requires restart. Mock tests validate output action selection, pulse parameters, and invalid side rejection. Physical feedback still needs runtime validation.

### Independent membership correction (0.18.18 test)
X/A now move only the selection cursor and preserve every trigger membership. Inward stick toggles exactly the currently selected finger on/off. This supersedes 0.18.16 assignment-transfer behavior, which interfered with building groups. Haptic selection feedback and membership beeps retained. Tests verify two-finger groups and unchanged membership across selection. Visual validation pending.

### Spoken selection (0.18.19 test)
X/A play pre-generated speech naming the selected side and finger, such as left index or right pinky. Eight WAV files reside in config; playback is asynchronous, with newer cues replacing older playback. Vibration and membership beeps retained. Both live pose files now default trigger assignment to index. Audio requires runtime listening check.

Selection also briefly flexes the chosen finger over 0.35 seconds, then returns to its current live grip/trigger pose without changing membership. For a closed finger the preview opens it; for an open finger it curls it.

### Final controller selection checkpoint
User requested commit and push after testing spoken names on both hands. Installed 0.18.19 plugin and 0.18.17 input bridge match packaged generic release binaries. Both live files default to index. Automated regression checks pass. Physical vibration and full multi-finger visual behavior have not received an explicit final confirmation.

### Disabled-hand calibration markers (0.18.20 test)
Uses SteamVR OpenVR overlays to show a six-centimetre ring/crosshair at each tracked controller position while that hand IK is disabled: blue left, orange right. Markers face the headset and use standing tracking space directly, independent of hand calibration. Hidden on tracking loss, inactive game context, or hand activation; destroyed on plugin shutdown. Overlay errors appear in controller status. VR visibility and placement require runtime validation. OpenVR overlay reference: https://github.com/ValveSoftware/openvr/wiki/IVROverlay_Overview

### Controller-oriented axis markers (0.18.21 test)
Replaces head-facing bitmap crosshairs with red X, green Y, blue Z axes attached to each controller device transform. Each 12 cm axis uses two crossed thin overlay strips to remain visible from multiple angles, extending from the tracked origin in the positive direction. These are spatial overlay planes, not a game mesh; no game-world occlusion. Visual validation pending.

### Scene mesh marker prototype (0.18.22 / body archive 0.8.0)
A second private animated accessory model contains solid axes weighted to two dedicated marker palette entries. Its identity guard is separate from the body. Controller positions relative to the headset are mapped through the current driver head transform; this eye-to-head approximation needs VR placement validation. Driver geometry and animation tracks remain byte unchanged. Overlay markers disabled for this test. Marker palette collapses when tracking is stale or that hand IK is enabled. Requires paired plugin/archive installation, and possibly reselecting the body accessory to recreate the new hookup.

### Fixed seated marker frame (0.18.23 test)
User confirmed scene meshes render but reported displaced positions and markers moving with head movement. Replaces animated-head-relative frame with a captured seated tracking-space origin and inverse neutral yaw. Controller transforms depend only on controller movement after capture; native game head reset recaptures the frame. Head anchor still uses the model head origin, so physical eye alignment needs visual calibration.

### Marker material isolation (archive 0.8.1)
Replaces every marker PMD material reference with private RGB materials: X red, Y green, Z blue. A private opaque white DDS/TOBJ supplies the base texture; no driver texture references remain on marker descriptor. Driver descriptor and textures unchanged. Visual color check pending.

### Persistent controller axes (0.18.24 test)
Scene axes remain visible for valid controller tracking regardless of per-hand IK enable state, allowing controller-to-hand alignment inspection while engaged. Stale/invalid tracking still collapses the marker palette. Visual check pending.

### Shared calibrated marker target (0.18.25 test)
After each hand has calibrated, its marker uses the same origin, wheel-grip anchor, basis and relative orientation as controller IK, before reach clamping and activation blending. Marker update runs after calibration processing to avoid a frame using old calibration. Uncalibrated hands retain the seated reference so they remain locatable. Reach limits and transient activation/action priority can still separate wrist from marker. Visual alignment pending.

### Marker frame diagnostic (0.18.26)
Static trace found the engine recenter reference at VR owner offsets 0x410/0x420 and current head pose at 0x438/0x448; getter at 0x14162e580 calls the reference-to-current transform at 0x14139bb90. Adds bounded one-second read-only logging for those values, camera seat offset, and actual driver root/head/wrist palette translations. Placement logic unchanged pending comparison.

### Actual XR/SteamVR frame bridge (0.18.27 test)
Diagnostic identified distinct tracking frames: ATS neutral origin/quaternion and local current head versus SteamVR standing head/controller poses. ControllerSceneFrame aligns headset poses to convert standing into ATS local, applies inverse engine recenter, then adds the Mustang interior camera base plus live seat head_offset. Removes per-hand calibration from marker placement. Tests verify stationary controller invariance under head translation and yaw. Camera base is Mustang-specific and requires visual alignment validation; samples stale beyond 250 ms hide axes.

### Shared physical frame for controller hands (0.18.28 test)
User confirms axes now stay at physical controller positions, but hands separate in some poses. Moves hand IK input into the validated scene frame with no independent yaw basis. Calibration records a rigid controller-to-wrist offset; rotation moves that offset instead of leaving it fixed in world space. Marker physical pose unchanged. Arm envelope remains enforced. Requires hand recalibration; visual hand alignment pending.

### Validated scene/controller checkpoint
User confirmed the scene axes stay at controller locations and the final rigid-offset hand update is exactly aligned. Installed plugin 0.18.28 and colored scene archive 0.8.1 match release binaries. Both regression suites pass. Scene meshes remain visible with hand IK on or off, with physical reach limits preserved. Camera mounting calibration currently targets the Mustang.

### Calibration-only scene axes (0.18.29 test)
Each scene marker now hides while that hand IK is enabled and reappears when disabled, preserving the validated physical controller frame and rigid-offset hand mapping. Each hand toggles independently. Visual recheck pending.

### Hand activation feedback (0.18.30 test)
Thumbstick click state changes request a vibration pulse on that hand plus an ascending two-tone engagement sound or descending two-tone disengagement sound. Long-hold re-enabling also cues the state change; a hold recalibrating an already-enabled hand does not repeat engagement feedback. Existing finger cues unchanged. Visual/audio runtime check pending.

### Validated hand-toggle feedback checkpoint
User confirmed engagement/disengagement tones and controller vibration work in game and requested commit/push. Final plugin 0.18.30 and both sound assets match installed files. Calibration axes hide while each hand is enabled. Both regression suites pass.

### Plugin rename (0.19.0)
Build DLL and SPF identity renamed to SPF_DriverVRBody. Existing plugin directory, settings identity, calibrated pose/sound files, OpenXR registry manifest path, and desktop launcher are migrated together. Controller API layer identity remains unchanged for existing SteamVR bindings.

### Validated rename checkpoint
User confirmed controller buttons and the renamed plugin still work after migration. SPF logs identify SPF_DriverVRBody. Installed DLL matches releases/SPF_DriverVRBody.dll; regression tests pass. For normal launches use the ATS VR Controller Hands desktop shortcut, which sets XR_ENABLE_API_LAYERS before creating ATS.
