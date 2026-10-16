# Development history

Archived checkpoint notes, preserved for technical provenance. These describe older builds and may contain obsolete paths, launch instructions, limitations and pending tests. For current instructions use the [README](../README.md), [controls guide](USAGE.md), [installer guide](../installer/README.md) and [developer guide](../native-diagnostics/README.md).


<a id="animation-fix"></a>

## ANIMATION_FIX.md

### 0.2.4: malformed seated clip corrected

Runtime differential: user confirms Diagnostic A (Tifa on carpet) visible and Diagnostic B (stock driver on toyseat) invisible. This disproves carpet-only spawning as the principal failure.

Comparison with the working Tifa PMA and original Mustang PMA exposed our clip-writer bug: PMA version 5 has five data-pointer/size fields at offsets 64..80. The writer had incorrectly begun editing at offset 72. The two-frame file therefore declared bone indices at 272 instead of 92 and transforms at 84 instead of 153. Those addresses pointed into unrelated arrays. This was a confirmed invalid clip, regardless of absent log errors.

The corrected 6985-byte clip declares time array 84, bone indices 92, transforms 153, end 6985 and expanded size 7009. It retains the original 61 bone-index mapping and duplicates the exact original Mustang neutral frame. The model, rig, head suppression, material paths and mount are unchanged. Regression tests use header-declared pointers and reject the earlier malformed header; finite transform/quaternion checks pass. ATS visibility remains unverified until the user runs this correction.

Restart ATS; restore original carpet if Diagnostic A was selected, then choose SPF Stock Driver Body (animated prototype). If Diagnostic B remains selected in passenger accessories, restore Tifa there to avoid two copies. No further attachment experiment is needed for this test.

#### Runtime confirmation — October 11, 2026

The user tested installed 0.2.4 in Mustang VR and confirmed the body is visible and in the perfect position. This closes the visibility/placement milestone. Live steering and pedal synchronization remain unimplemented.


<a id="body-visibility"></a>

## BODY_VISIBILITY.md

### Camera-Conditioned Driver Visibility

Plugin 0.13.2 is installed and visually validated with the 0.7.0 body archive. SPF supplies the active camera type; type 2 is the current interior/VR camera. Unknown camera state leaves the exterior stock driver visible and hides the private head as a first-person fallback.

#### Live modes

`body_visibility.txt` accepts:

- `replace-exterior` (current): our animated body remains visible; outside the interior camera, the player's stock driver is hidden after its exact controller/model identity is observed.
- `interior`: our body is hidden in recognized non-interior cameras, leaving the stock exterior driver visible.

The setting reloads twice a second; invalid edits keep the prior mode. In either mode, the private head/neck is hidden in the interior view and restored outside through normal palette regeneration.

#### Identity and render guards

The same guarded player-owner/vehicle chain used by the game horn and engine queries nominates the driver controller at vehicle+0x1df0 and its model at +0x1df8. The player updater's static caller uses this controller; traffic uses +0x2c8. Hiding requires CaptureDriver to observe the exact nominated controller/model pair with stock model identity and a fresh timestamp. A shared asset name alone is insufficient. No shared resource or global traffic model is modified.

The skeleton post hook verifies executable fingerprint, 61-bone count, private/player instance identity, output palette pointer and finite matrices. It collapses only the selected instance's rendered bone palette to a finite point after the original hierarchy assembly. Animation inputs remain intact, and the original matrices regenerate on the next call. Write faults stop further visibility writes.

Regression tests cover caller arguments, stack alignment through normal calls, integer/floating return preservation, post-callback ordering, finite matrix collapse, head-only bone changes, wrong owner and traffic/different-model rejection. The user confirmed both visibility directions and the final exterior-head version. Runtime recorded 12,312 player-controller matches, 445 stock-hide frames, 3,326 interior head-hide frames and zero visibility write faults. Installed DLL, archive and mode file match their release copies byte-for-byte.

This is a guarded render-palette implementation, not a public engine visibility flag. It preserves the engine's bounds; general rigs, other vehicles and separate mirror-camera behavior are not validated.


<a id="brake-integration"></a>

## BRAKE_INTEGRATION.md

### Staged brake reach and ankle press

The 0.5.0 brake-test archive pairs with the 0.4.0 SPF plugin. Raw brake input takes priority over raw gas input. The right ankle releases its accelerator press before the leg reaches left. Once the foot arrives over the brake, the ankle presses downward using the same 12-degree motion as the validated gas pose. Brake release lifts the ankle, returns the leg to the accelerator and restores any held gas input.

The reach moves the ankle 8 cm toward the left foot, lifting it along a small arc and ending 5 mm above its prior height. A two-joint leg solve rotates the thigh and shin without stretching the leg or moving the hips. The foot's orientation is maintained during the reach, then the ankle rotates about the same hinge as the gas prototype. Lateral movement takes about 0.2 seconds; ankle lift/press takes about 0.1 seconds at full travel. The actual pedal spacing and visual alignment still need a VR test.

The private clip contains 83 pose blocks of 47 steering frames: 33 byte-identical gas blocks, 17 transfer/hover blocks, and 33 brake press blocks. Only the right thigh, shin and foot tracks differ in the added blocks. The original steering arm tracks and all other bones remain byte-identical. PMA frame count, duration (162.5 seconds), array pointers and normalized rotations are checked. Offline tests validate ankle targeting, preserved leg lengths, downward brake toe movement and unchanged gas/arm tracks. Native tests verify lift-before-move, hover-before-press, brake priority, release-before-return, interrupted movement, invalid input, clip guards and frame bounds, alongside the existing forwarding regressions.

Motion state is reset for actor identity, clip index or world-generation changes. Stale control telemetry selects neutral. The original executable hash and private-model guards still apply; previous steering and throttle clips remain supported. PEDAL_PHASE logs report the sequence and BRAKE health logs report input, transfer, press and override count. Runtime pose validation is pending.

Test parked in first-person VR: press and release the gas, then hold the brake. Confirm the right leg moves left, briefly hovers and then presses at the ankle. Release the brake and confirm the foot returns right. Try holding gas while pressing/releasing brake, and steer during each state. If the reach misses the brake, adjust the authored lateral distance before claiming alignment. To roll back, close ATS and restore the 0.4.0 throttle archive and 0.3.0 plugin from work/pre-brake-backup under their existing installed filenames.


<a id="clutch-integration"></a>

## CLUTCH_INTEGRATION.md

### Independent clutch ankle press

#### Current alignment adjustment

The user subsequently confirmed the combined 0.6.0 clutch test looks good. Its runtime logs recorded full clutch input and zero caught faults. The final 0.6.1 archive is built with the requested 14 cm reach but is deliberately not installed or relaunched, following the request to commit the changes. See live_pedals_validation.json for the final validation status. Earlier pending-test notes below describe the original test handoff.

The user confirmed the combined clutch test looks good, then requested the brake foot 2 cm to the right. The 0.6.1 pedal-alignment archive reduces the right brake reach from 16 cm to 14 cm while retaining the left-leg 6 cm rest offset, clutch ankle press, gas position, steering and native sequence timing. It uses the same 0.5.0 plugin. build_brake_prototype.py now produces the 0.5.3 right-pedal intermediate clip and build_clutch_prototype.py produces the adjusted combined archive. The previous 0.6.0 archive is retained for rollback. The fourteen-centimetre landing needs a final visual check; see clutch_alignment_validation.json.

The 0.6.0 clutch-test archive and 0.5.0 SPF plugin combine the requested 16 cm right brake reach, 6 cm left resting-leg offset and independent left clutch press. The left thigh and shin keep the shifted resting pose. Only left-foot bone 51 rotates when raw clutch input is pressed; the ankle position remains fixed. The motion uses the same 12-degree downward hinge rotation as the gas foot, with a roughly 0.1-second full press/release. Its toes lower about 2.7 cm. Steering and right gas/brake movement continue independently, including while the clutch is held.

build_brake_prototype.py generates the adjusted 0.5.2 right-pedal clip; build_clutch_prototype.py adds nine clutch levels. Each clutch level contains the same 83 right-pedal blocks and their 47 steering frames. Released clutch is byte-identical to the adjusted right-pedal clip. At every other clutch level, all tracks outside bone 51 are byte-identical. PMA pointer, duration, frame-count and quaternion checks pass; FK confirms fixed ankle position and downward toe motion. The expanded clip has 35109 frames and a duration of 1462.833333 seconds. Its uncompressed data is about 120 MB; the deflated mod archive is about 52 MB. This is an experimental single-clip implementation; loading and VR performance still require a runtime check.

The SPF plugin samples raw clutch input separately from gas and brake. It smooths the ankle over 0.1 seconds and selects the clutch block around the existing independent right-pedal pose. Native tests cover every right-pedal block across clutch input levels, total frame bounds, clip guards, invalid-input release and unchanged forwarding/return behavior. Earlier steering/throttle/brake clips remain supported. Motion resets on actor, index or world-generation changes; stale telemetry returns the clutch to its released pose. CLUTCH health logs report input, smoothed press and override count.

The user confirmed the initial brake sequence works and the 12 cm right / 6 cm left alignment looked good, then requested another 4 cm of brake reach and clutch animation. The new 16 cm reach and clutch remain pending visual validation. With ATS closed, install the paired files, retaining the existing archive/DLL names. Rollback copies live under work/pre-clutch-backup. Test in first-person VR while parked: clutch release, half press, full press, release; then hold clutch while steering and pressing/releasing gas and brake. Confirm the left leg stays in place and its ankle presses downward, and check the farther-left brake landing and loading/frame behavior.


<a id="controller-ik"></a>

## CONTROLLER_IK.md

### Controller hand IK

The user validated both hands in the Mustang with the 0.18.4 build. An early OpenXR API layer registers Oculus Touch thumbstick actions before ATS attaches its session action sets. The SPF plugin reads those actions and polls SteamVR controller poses through OpenVR, then applies runtime arm IK after torso compensation. All components run inside ATS; there is no companion process. Other OpenXR applications are passed through by the layer. This route is validated with SteamVR and Oculus Touch controllers only.

Press each thumbstick to calibrate and enable that hand. Hold the controller at the intended wheel grip: its position/orientation and headset heading align with the current animated wrist without jumping. Later clicks toggle the hand; holding for 0.7 seconds recalibrates and enables it. Re-enabling calibrates again. World changes disable both hands and clear calibration. Activation/release blend over 0.18 seconds. Lost tracking fades back to steering. Targets clamp to the arm envelope without stretching bones; fingers retain their animations. Existing gestures, horn, signal and ignition temporarily take priority on their respective arms.

#### Installation and launch

With ATS closed, install the matching plugin DLL, spf_controller_openvr.dll and the license files into the existing SPF plugin folder. Install controller-layer/spf_controller_xr_layer.dll and its JSON manifest together there. Register the manifest's absolute path as a DWORD value of zero under HKCU\SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit. Retain other registered layers.

Install controller-layer/openxr_loader.dll in the ATS bin/win_x64 directory and set o_openxr_external_loader to 1 in the game config, backing up both beforehand. The official loader is Khronos 1.1.63. Keep the existing body archive and calibrated live pose files.

The working startup explicitly enables XR_APILAYER_SPF_driver_controller_input and supplies the Steam application identity. Use Launch-ATS-ControllerHands.ps1 with Steam already running. Normal Steam launch did not activate the bridge in the comparison; routine Steam launch integration remains pending. The launcher affects only the child ATS process. To roll back, set the external-loader option to 0 and disable/remove only this layer registration. Local backups are in backups/controller-ik.

#### Validation and limitation

Regression tests pass for all steering frames, both arms, calibration, toggle/hold edges, mapped wrist displacement, unchanged other joints and unreachable target clamping. Layer tests preserve ATS action sets and synchronization inputs, verify per-hand boolean states and reject unfocused input. Runtime logs captured over eight thousand left-hand and six thousand right-hand solves, with zero rejected solves and zero caught pose faults. The user confirmed both controller hands work.

The current elbow guide uses the animated elbow plus a small inward/downward offset. The user reports intermittent wrong-way bends. This is the working checkpoint before elbow-stability changes. Finger tracking is not implemented.

References: [Valve pose query](https://github.com/ValveSoftware/openvr/wiki/IVRSystem::GetDeviceToAbsoluteTrackingPose) and [Khronos loader layer documentation](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/specification/loader/api_layer.adoc).

#### Elbow-stability checkpoint (0.18.5)
The new solver uses a torso-relative down/out/back guide and transports the prior elbow bend along the changing shoulder-to-wrist ray. It limits guide rotation to 180 degrees per second and retains continuity when the guide is nearly parallel to the reach. The controller hand target and wrist orientation are preserved. config/controller_elbow_guides.txt reloads every half second; left/right lines contain finite nonzero model-space guide vectors. Tests cover continuous reach sweeps across all steering frames, wrist position/orientation preservation, and no instantaneous pole reversal. The user requested this checkpoint be committed after using it. Runtime health shows both hands active, thousands of solves and zero rejected solves or caught pose faults. Forearm twist sharing is a separate next change.

#### Axial twist sharing checkpoint (0.18.6)
Controller hand roll about the forearm axis is decomposed relative to the calibrated wrist. Half is moved into the forearm's local rotation and the inverse applied at the wrist, preserving the hand's model-space position and orientation. The share fades with controller activation/release. The angle is unwrapped across half turns to avoid discontinuity. Only forearm/wrist local rotations change; finger rotations remain unchanged. config/controller_forearm_twist_share.txt reloads twice a second and accepts 0 through 1, initially 0.50. Zero leaves all roll at the wrist; one places all axial roll in the forearm. Tests cover both arms and all steering poses with continuous 0-to-200-degree rolls, unchanged wrist targets and exact half share. The user requested this version be committed after the runtime test. Health recorded 7245 left and 6763 right solves with zero rejected solves and zero caught pose faults.

#### Controller finger controls
Enabled controller hands default to the original mesh's open bind finger pose. Grip controls the thumb, middle, ring and little fingers; trigger independently controls the index. Closed poses reuse the prepared finger-down gesture joints. The early layer now adds per-hand squeeze/value and trigger/value float actions to its existing thumbstick actions. Values are clamped to 0..1 and smoothed over 40 milliseconds, then blended with hand activation/release. Scripted actions retain their existing priority. Tests verify grip/trigger independence, open rest, preservation of arm/body/opposite-hand joints, exact disabled-pose forwarding and per-hand analog action reads. Plugin and early layer are a matching installed pair; runtime visual validation remains pending.

#### Live controller finger poses

The plugin watches config/controller_left_fingers.txt and config/controller_right_fingers.txt twice a second, outside the animation hook. Each file contains the open and closed local rotations for all 15 finger bones. Valid saves publish an immutable snapshot used by the next controller finger blend. Invalid, missing, incomplete, duplicate, opposite-hand, nonfinite or badly normalized edits retain the last valid pose. Missing initial files use the compiled defaults. The two sides reload independently; input routing stays fixed (index uses trigger, other fingers use grip).

Format: SPF_CONTROLLER_FINGERS 1, followed by 15 rows of `bone INDEX OPEN_W OPEN_X OPEN_Y OPEN_Z CLOSED_W CLOSED_X CLOSED_Y CLOSED_Z`. Comments start with #. Quaternion values near unit length are normalized; the file is bounded to 16 KiB. Left finger bones are 8 through 22; right are 29 through 43. In each side, groups of three joints are index, middle, ring, little, then thumb. These are local joint rotations, not positions. Finger shape changes do not alter IK hand targets, calibration, or wrist/forearm twist settings. Health logs include revisions and rejected-edit counts for each side.

Live resting-pose refinement: both open poses now interpolate gently toward their closed counterparts (finger joints 12%, 18%, 12%; thumb base 30%, middle/tip 15%). All closed quaternion values are preserved exactly. Updated installed and release live files; visual check pending.

Closed middle-finger alignment refinement: removed the gesture's extra off-axis rotation from all three middle joints, retaining the curl component about the original bind bending axis on both hands. Open poses, thumb settings, other fingers and input routing remain byte-identical. Applied via live files; mesh clearance needs the user's visual check.

The first middle-finger correction still intersected in the user's view. Rebuilt both closed middle chains from the original bind rotations with local joint curls of 85, 80 and 25 degrees (post-multiplied), removing the earlier gesture-direction compensation and the incorrect parent-axis curl. Applied through live files; awaiting the next visual check.

#### Live finger routing checkpoint (0.18.9)
The default trigger finger is now middle; grip curls thumb, index, ring and little. Each live finger file accepts an optional trigger_finger row with index, middle, ring, little or thumb. All three joints of the selected finger use trigger; the remaining fingers use grip. Routing can change live after this one plugin update. Invalid or duplicate routing keeps the previous valid pose. The release files specify trigger_finger middle and remove off-axis rotations from closed ring/little joints following the user's overlap report. Build and regression tests pass; installation is awaiting ATS exit.

0.18.9 is installed with both live files routed to middle-trigger control. Pre-update plugin and live files are backed up. The existing early input bridge and controller-enabled launcher are unchanged. Visual test pending.

The user requested the complete current finger-control checkpoint be committed. Installed plugin, bridge and both live pose files match the packaged files. Runtime health recorded 4878 left and 4850 right solves, zero rejected solves, zero caught pose faults and zero rejected live files. Both initial live pose revisions were accepted. Grip controls all except middle; trigger controls middle, with the selected finger editable in each live file. Controller X/A mode buttons are a separate subsequent change.

#### Controller X/A finger mode test (0.18.10, uncommitted)
The early input layer adds X on the left Oculus Touch controller and A on the right as independent boolean mode actions. Each rising press alternates that hand's trigger-controlled finger between middle and index; grip controls the remaining fingers. Holding a button does not repeat. Tracking recovery with the button held does not create another edge. Choices are independent, reset on world changes, and revert to the live file's routing when its pose revision changes. Pose rotations and thumbstick calibration remain unchanged. The plugin and early layer must be installed as a matching pair. Regression tests cover binding/action type setup, independent mode state, held-button suppression, tracking recovery and live-file override reset. Runtime visual validation pending.

##### Finger mode transitions (0.18.11 test)
X/A now blend index and middle input routing over 0.18 seconds, independently per hand. Grip and trigger values remain live during the eased transition; retoggling continues from the current blend. The live `config/controller_finger_mode_blend_seconds.txt` setting accepts a positive finite duration in seconds and reloads every half second; invalid edits retain the previous duration. This plugin update uses the existing 0.18.10 input bridge. Visual validation pending.

##### Independent thumb buttons (0.18.12 test)
Hold Y on the left controller or B on the right to curl that thumb; release to return to its live open pose. Thumb input replaces grip control for the three thumb joints and is smoothed like grip/trigger. X/A finger mode switching remains independent. Requires the paired early input bridge update and one restart. Visual validation pending.

##### Four-finger cycle (0.18.13 test)
X/A independently advance trigger control through index, middle, ring, and little finger (wrapping to index). Initial selection comes from each live pose file, currently middle, so the first press selects ring. Grip controls the remaining three fingers; Y/B retain separate thumb control. Mode changes retain the live eased blend duration. Tests cover both sides, full wraparound, held-button suppression, tracking recovery, and rapid cycling. Visual validation pending.

##### Finger cycle correction (0.18.14 test)
User visual testing identified the ring/little groups as reversed in the assumed skeleton order. Cycle now uses joint groups 0, 1, 3, 2 to produce the visible index, middle, ring, pinky order on each hand. Status labels corrected accordingly. Smooth routing and thumb buttons unchanged. Visual recheck pending.

##### Multiple trigger fingers (0.18.15 test)
X/A select the finger without changing current membership. Push the left stick right or the right stick left to add/remove that selected finger. An inward value above 0.7 toggles once; return below 0.3 to rearm. Tracking loss suppresses a held-stick recovery edge. A higher beep means added and a lower beep means removed. Selected members follow trigger together; others follow grip, and Y/B still control thumbs. Smooth live routing transitions retained. Paired input bridge update requires one restart. Visual validation pending.

##### Visible X/A cycling restored (0.18.16 test)
X/A move the selected finger membership to the next finger while preserving other assigned fingers. Inward stick still toggles the selected finger membership. This restores visible cycling from the default single-finger mode; when the selected finger is unassigned, cycling only moves the selection until it is added. Regression covers preserving an additional middle-finger assignment. Visual recheck pending.

##### Controller vibration feedback (0.18.17 test)
Adds native OpenXR haptic actions for both controllers. A successful X/A cycle or inward-stick membership toggle requests one 60 ms pulse at 60% amplitude on that hand; membership beeps retained. Paired bridge update requires restart. Mock tests validate output action selection, pulse parameters, and invalid side rejection. Physical feedback still needs runtime validation.

##### Independent membership correction (0.18.18 test)
X/A now move only the selection cursor and preserve every trigger membership. Inward stick toggles exactly the currently selected finger on/off. This supersedes 0.18.16 assignment-transfer behavior, which interfered with building groups. Haptic selection feedback and membership beeps retained. Tests verify two-finger groups and unchanged membership across selection. Visual validation pending.

##### Spoken selection (0.18.19 test)
X/A play pre-generated speech naming the selected side and finger, such as left index or right pinky. Eight WAV files reside in config; playback is asynchronous, with newer cues replacing older playback. Vibration and membership beeps retained. Both live pose files now default trigger assignment to index. Audio requires runtime listening check.

Selection also briefly flexes the chosen finger over 0.35 seconds, then returns to its current live grip/trigger pose without changing membership. For a closed finger the preview opens it; for an open finger it curls it.

##### Final controller selection checkpoint
User requested commit and push after testing spoken names on both hands. Installed 0.18.19 plugin and 0.18.17 input bridge match packaged generic release binaries. Both live files default to index. Automated regression checks pass. Physical vibration and full multi-finger visual behavior have not received an explicit final confirmation.

##### Disabled-hand calibration markers (0.18.20 test)
Uses SteamVR OpenVR overlays to show a six-centimetre ring/crosshair at each tracked controller position while that hand IK is disabled: blue left, orange right. Markers face the headset and use standing tracking space directly, independent of hand calibration. Hidden on tracking loss, inactive game context, or hand activation; destroyed on plugin shutdown. Overlay errors appear in controller status. VR visibility and placement require runtime validation. OpenVR overlay reference: https://github.com/ValveSoftware/openvr/wiki/IVROverlay_Overview

##### Controller-oriented axis markers (0.18.21 test)
Replaces head-facing bitmap crosshairs with red X, green Y, blue Z axes attached to each controller device transform. Each 12 cm axis uses two crossed thin overlay strips to remain visible from multiple angles, extending from the tracked origin in the positive direction. These are spatial overlay planes, not a game mesh; no game-world occlusion. Visual validation pending.

##### Scene mesh marker prototype (0.18.22 / body archive 0.8.0)
A second private animated accessory model contains solid axes weighted to two dedicated marker palette entries. Its identity guard is separate from the body. Controller positions relative to the headset are mapped through the current driver head transform; this eye-to-head approximation needs VR placement validation. Driver geometry and animation tracks remain byte unchanged. Overlay markers disabled for this test. Marker palette collapses when tracking is stale or that hand IK is enabled. Requires paired plugin/archive installation, and possibly reselecting the body accessory to recreate the new hookup.

##### Fixed seated marker frame (0.18.23 test)
User confirmed scene meshes render but reported displaced positions and markers moving with head movement. Replaces animated-head-relative frame with a captured seated tracking-space origin and inverse neutral yaw. Controller transforms depend only on controller movement after capture; native game head reset recaptures the frame. Head anchor still uses the model head origin, so physical eye alignment needs visual calibration.

##### Marker material isolation (archive 0.8.1)
Replaces every marker PMD material reference with private RGB materials: X red, Y green, Z blue. A private opaque white DDS/TOBJ supplies the base texture; no driver texture references remain on marker descriptor. Driver descriptor and textures unchanged. Visual color check pending.

##### Persistent controller axes (0.18.24 test)
Scene axes remain visible for valid controller tracking regardless of per-hand IK enable state, allowing controller-to-hand alignment inspection while engaged. Stale/invalid tracking still collapses the marker palette. Visual check pending.

##### Shared calibrated marker target (0.18.25 test)
After each hand has calibrated, its marker uses the same origin, wheel-grip anchor, basis and relative orientation as controller IK, before reach clamping and activation blending. Marker update runs after calibration processing to avoid a frame using old calibration. Uncalibrated hands retain the seated reference so they remain locatable. Reach limits and transient activation/action priority can still separate wrist from marker. Visual alignment pending.

##### Marker frame diagnostic (0.18.26)
Static trace found the engine recenter reference at VR owner offsets 0x410/0x420 and current head pose at 0x438/0x448; getter at 0x14162e580 calls the reference-to-current transform at 0x14139bb90. Adds bounded one-second read-only logging for those values, camera seat offset, and actual driver root/head/wrist palette translations. Placement logic unchanged pending comparison.

##### Actual XR/SteamVR frame bridge (0.18.27 test)
Diagnostic identified distinct tracking frames: ATS neutral origin/quaternion and local current head versus SteamVR standing head/controller poses. ControllerSceneFrame aligns headset poses to convert standing into ATS local, applies inverse engine recenter, then adds the Mustang interior camera base plus live seat head_offset. Removes per-hand calibration from marker placement. Tests verify stationary controller invariance under head translation and yaw. Camera base is Mustang-specific and requires visual alignment validation; samples stale beyond 250 ms hide axes.

##### Shared physical frame for controller hands (0.18.28 test)
User confirms axes now stay at physical controller positions, but hands separate in some poses. Moves hand IK input into the validated scene frame with no independent yaw basis. Calibration records a rigid controller-to-wrist offset; rotation moves that offset instead of leaving it fixed in world space. Marker physical pose unchanged. Arm envelope remains enforced. Requires hand recalibration; visual hand alignment pending.

##### Validated scene/controller checkpoint
User confirmed the scene axes stay at controller locations and the final rigid-offset hand update is exactly aligned. Installed plugin 0.18.28 and colored scene archive 0.8.1 match release binaries. Both regression suites pass. Scene meshes remain visible with hand IK on or off, with physical reach limits preserved. Camera mounting calibration currently targets the Mustang.

##### Calibration-only scene axes (0.18.29 test)
Each scene marker now hides while that hand IK is enabled and reappears when disabled, preserving the validated physical controller frame and rigid-offset hand mapping. Each hand toggles independently. Visual recheck pending.

##### Hand activation feedback (0.18.30 test)
Thumbstick click state changes request a vibration pulse on that hand plus an ascending two-tone engagement sound or descending two-tone disengagement sound. Long-hold re-enabling also cues the state change; a hold recalibrating an already-enabled hand does not repeat engagement feedback. Existing finger cues unchanged. Visual/audio runtime check pending.

##### Validated hand-toggle feedback checkpoint
User confirmed engagement/disengagement tones and controller vibration work in game and requested commit/push. Final plugin 0.18.30 and both sound assets match installed files. Calibration axes hide while each hand is enabled. Both regression suites pass.

##### Plugin rename (0.19.0)
Build DLL and SPF identity renamed to SPF_DriverVRBody. Existing plugin directory, settings identity, calibrated pose/sound files, OpenXR registry manifest path, and desktop launcher are migrated together. Controller API layer identity remains unchanged for existing SteamVR bindings.

##### Validated rename checkpoint
User confirmed controller buttons and the renamed plugin still work after migration. SPF logs identify SPF_DriverVRBody. Installed DLL matches releases/SPF_DriverVRBody.dll; regression tests pass. For normal launches use the ATS VR Controller Hands desktop shortcut, which sets XR_ENABLE_API_LAYERS before creating ATS.

##### Repository-matching plugin name (0.19.1)
Plugin output and SPF identity renamed to SPF_VisibleFirstPersonDriverBody at user request. Existing configuration, enabled state, controller bridge manifest registration, and desktop shortcut migrate from SPF_DriverVRBody.

##### Controller ignition overlap prototype (0.20.0 / archive 0.9.0)
Adds a visible red 8 cm ignition wireframe at the live ignition arrival thumb target plus the existing 15.24 cm forward correction. A blue oriented box follows the solved right wrist (8 x 7 x 11 cm). Exact oriented-box SAT overlap plus a fresh right trigger squeeze requests one engine start on the plugin update thread when player identity, telemetry, and off state are valid. Uses the existing original engine routine and suppresses automatic reach playback because the tracked hand is already at the key. Held trigger outside then entering does not start; release and squeeze inside. Requires right controller hand enabled. Visible alignment and engine interaction pending VR test.

Live ignition box adjustment: config/ignition_box_offset.txt contains three finite metre offsets (X right, Y up, Z rearward) from the generated key target. Polled every 500 ms outside the animation hook; incomplete or invalid edits retain the previous position. The same position drives both the wireframe and overlap test. Initial values 0 0 0.

##### Hand collision box toward fingertips (0.20.1 test)
Stock bind transforms show the middle-finger base at approximately -9.5 cm on the wrist local X axis. Move the blue hand box 8 cm along that direction, keeping its rotation attached to the wrist and using the same center for collision. New live config/ignition_hand_box_offset.txt accepts three finite hand-local metre offsets, defaults -0.08 0 0, polled every 500 ms with last-valid retention. Requires one plugin restart for initial support.

##### Native ignition/electricity cycle (0.20.2 test)
Overlap trigger now invokes the game combined engine/electricity control at validated entry 0x1405e1a70 rather than always requesting engine start. This honors normal game configuration: electricity-on first, start next, and shutdown while running. A signature guard and fresh player/state checks gate calls. The existing reach delay is bypassed only for this synchronous controller-originated invocation because the hand already contacts the ignition. Running/Off states accepted, transition states ignored. Right hand box final live offset -0.14 -0.02 0 accepted visually by user. Native multi-click sequence still needs runtime validation.

##### Native ignition signature correction (0.20.3)
User reports no response on native-cycle prototype. Executable byte verification found a leading 0x40 REX prefix before push RBX; the 0.20.2 signature omitted it, rejecting all calls. Guard corrected against actual executable bytes. Other overlap and state logic unchanged; runtime multi-click validation pending.

##### Ignition interaction checkpoint (0.20.4)
Native-cycle logs confirm starting, shutdown, and electricity-only transitions from overlap trigger presses. User calibrated the hand box to -0.14 -0.02 0 and ignition box to -0.01 0 0, then requested hiding the red ignition wireframe and commit/push. Only rendering is hidden; SAT collision and live position offset remain active.

##### Horn contact prototype (0.21.0 / archive 0.10.0)
Adds a visible green wireframe near the compiled horn press wrist position. With the right tracked hand enabled, its blue hand box overlapping the horn box and trigger held combines native horn input for the exact player horn object only. Release, leaving contact, disabling tracking, or stale samples stops the override. Existing keyboard/wheel input is preserved. The automatic left-arm horn reach is suppressed during controller contact. New horn_box_offset.txt allows live position tuning. Forwarding regression preserves receiver and other input arguments. Runtime horn playback and placement validation pending.

##### Live horn box scale (0.21.1 test)
Horn box size increased by 25% on every dimension, from 12x10x10 cm to 15x12.5x12.5 cm. New horn_box_scale.txt defaults to 1.25, reads a positive finite multiplier live every 500 ms, retaining last valid value on bad input. Both rendered palette scale and SAT half extents use that multiplier. Existing tuned center retained at 0.01 0.07 -0.10 offset.

##### Explicit controller layer discovery in launcher
A later desktop launch failed OpenXR initialization with XR_ERROR_API_LAYER_NOT_PRESENT despite valid VR arguments and an existing per-user registry manifest. Launcher now sets child-only XR_API_LAYER_PATH to the verified bridge manifest/DLL directory in addition to XR_ENABLE_API_LAYERS. Installed and repository launcher locations are supported. Runtime confirmation pending.

##### Contact horn from either hand (0.21.2 / archive 0.10.1)
Removes trigger requirement from horn interaction. Either enabled tracked hand box overlapping the horn box holds the native horn input; both clear, tracking loss, or disabling hands releases it. Adds a left-hand visible collision box with mirrored fingertip-axis offset (left local +X versus right -X, verified from stock bind transforms). Ignition retains right-trigger click behavior. Includes pending 25% horn size enlargement and live size file. Runtime validation pending.

##### Independent left hand collision box (0.21.3 test)
Moves left box 2 cm along local +Y relative to its previous 0.14 -0.02 0 position, toward palm for visual comparison. New live horn_left_hand_box_offset.txt defaults 0.14 0 0, independent of right-hand ignition box. Both left visible mesh and horn overlap use its center. Requires one restart to add file support; subsequent offsets apply live.

##### Controller box setup mode (0.22.0)
F9 toggles SPF Setup.EditInteractionBoxes (remappable). Both target wireframes show while editing and ignition/horn interaction is disabled. Grip near a box acquires it using physical controller position; one grip moves/rotates, both grips scale from controller separation and rotate from their connecting axis. Releasing a grip transfers to the remaining hand without jumping; releasing both saves. Tracking loss ends/transfers a grab using the last valid box pose. Placement writes atomically to config/interaction_boxes.txt outside the animation hook on release/exit. Strict parser retains last valid placement on partial/invalid edits. Authored placements override the earlier pose-relative offset/scale files; initial positions still derive from those until grabbed. Tests cover drag/rotation/scaling/handoff/serialization/hotkey edges. VR interaction check pending.

##### Nonuniform two-hand box resizing (0.22.1 test)
At two-hand acquisition, the editor picks the box-local axis most aligned with hand separation. Spreading or closing changes only that dimension; release/regrab along another direction to edit another axis. One-hand rotation remains available; two-hand resizing keeps box rotation fixed for predictable axis scaling. Version 2 placement files persist per-axis stretch factors and retain compatibility with version 1 uniform-scale files. Rendered column scales, collision half extents, and grab hit tests all use per-axis factors.

##### Setup-only interaction wireframes (0.22.2 test)
Ignition, horn, and both hand collision box meshes render only while box setup mode is enabled. Outside setup their palettes collapse while collision/trigger behavior remains active. Disabled-hand controller calibration axes retain their existing visibility rule.

##### Setup target labels (archive 0.10.2)
Adds IGNITION and HORN geometry labels above their target wireframes, weighted to the same target bones. Labels inherit position, rotation, per-axis scale, and setup-only visibility; collision half extents unchanged. Text is formed from double-sided mesh quads using an embedded 5x7 font and private existing target materials. VR legibility pending.

##### Setup editor and horn checkpoint
User requested commit and push after the labeled-box build. Installed plugin 0.22.2 and scene archive 0.10.2 match final release binaries. Current live horn offsets/scales and saved box placements (if present) are included. Regression tests cover forwarding, overlap, grip editing, rotation, axis-specific scaling, handoff and persistence. Detailed per-feature VR test results beyond user acceptance are not claimed. Launcher explicit bridge search path is included.


<a id="controller-menu-models"></a>

## CONTROLLER_MENU_MODELS.md

### SteamVR controller models and shared pointers

Version 0.37.4 loads each controller's SteamVR render-model name, component meshes and textures at runtime. No Quest assets are distributed. Models appear in menus and in the cabin while that hand's IK is disabled; axes remain a fallback when model rendering is unavailable.

The bridge draws textured geometry in a separate D3D11 OpenXR stereo projection layer using runtime eye poses and FOVs. It retains the original frame layers and display time, restores the game graphics state, and introduces no additional input synchronization. Typed render-target views support SteamVR's typeless swapchain textures.

Both menu and cabin targeting use SteamVR's tip component transform. Both share white overlay beams and hit dots. Beam width stays at 1.5 millimetres regardless of hit distance; texture aspect changes to preserve beam length without widening misses. Cabin collision queries remain in the existing scene frame, with their hit distance passed to the shared pointer display.

All four regression suites pass, including software D3D11 stereo rendering, parallax, graphics-state restoration, inactive-layer behavior and original frame forwarding. In VR the user confirmed the Quest models, menu pointing/clicking, cabin models and aiming, and final white fixed-width pointer appearance. This visual check does not establish long-term timing behavior across all runtime sessions.

Installed plugin and bridge match the release artifacts. Diagnostics are in controller_models.log and MENU_MODEL entries in the plugin and early-menu logs.

Version 0.37.5 feeds the native controller bridge inputs into SteamVR component state: stick axes/click, X/A, Y/B, analog trigger and grip. The mapping follows the installed Quest render-model definitions. It falls back to legacy controller state when native input is unavailable. All four suites pass, including mapping and neutral-release checks. Installed for the user test; the user requested commit and push afterward without a separate detailed component-by-component visual report.


<a id="controller-menu-mouse"></a>

## CONTROLLER_MENU_MOUSE.md

### Controller menu pointing checkpoint

Menus show controller axes, straight forward beams, and hit dots. Point either controller at the captured VR menu surface without holding grip. A fresh trigger press selects that controller for cursor ownership; holding trigger drags and the thumbstick scrolls. Ownership stays stable through brief misses. Cabin interactions pause during menu control.

The ray uses the final native menu transform with its per-eye translation removed, supports flat and curved panels, and is expressed relative to the headset. Beam and intersection use the same raw controller forward axis. Quest tip pitch compensation is disabled at the user's request.

Cursor movement currently uses smoothed relative mouse input. Direct Windows cursor positioning was tested and did not move ATS's menu cursor. Relative gain starts at 1.5 on each axis. The installed config file `controller_menu_cursor_gain.txt` contains horizontal and vertical multipliers, for example `1.5 1.5`; valid edits reload every half second, invalid values preserve the last valid pair. Accepted values are finite, positive, and at most 10. This is a tuning workaround; direct internal ATS cursor positioning remains unimplemented.

The grip-and-rotate air mouse remains a fallback when the native menu surface is unavailable. Version-specific native hooks retain their executable and signature guards. Controller input reads do not add OpenXR synchronization calls.

Validation: both regression suites pass, including ray/surface geometry, eye-offset removal, pointer ownership, stationary movement, independent gain, reverse travel, and invalid edits. The user reports the relative-pointer version is almost working, but exact cursor/laser alignment remains unresolved. No claim of pixel-exact positioning or complete click coverage is made. Plugin 0.34.8 is installed with matching release binary.

#### Native absolute cursor (0.35.1)

Supported ATS builds now use the native input cursor and absolute-position event queue. The guarded path checks the executable signature, input object vtable, window, and viewport before writing position and submitting the event. Relative input remains a fallback. Native positioning uses the game's actual viewport rather than desktop dimensions and bypasses relative gain.

The curved panel follows the native mesh generator and renderer: angle is `2*(u-.5)/radius`, X is scaled by half width, Y by half height, and Z by full width. Its cross section is elliptical. Tests reconstruct rendered points across horizontal/vertical coordinates and several radii, then recover their UVs. The user confirmed alignment looks good in VR.

Startup limitation: the observed launch began at 09:21:48; SPF preload finished at 09:22:19, SDK initialization began at 09:23:59, and our first menu surface appeared at 09:24:01. Native pointer updates followed within one second. Early menu startup is a separate follow-up.

#### Early menu startup (0.36.2)

The existing in-process OpenXR bridge loads the adjacent driver DLL after successful instance creation, outside its input mutex. The DLL validates the executable hash and menu/cursor instruction bytes, then installs its two menu observation hooks using vendored MinHook 1.3.4. The bridge retains the DLL reference while SPF probes plugin manifests.

Before SPF activation, a background thread handles controller poses, overlays, and menu input. The menu render callback only publishes the surface and consumes queued native cursor coordinates. Surface publication and cursor consumption do not wait on the worker. Cursor commands expire after 100ms. The sync callback only records focus, and input reads never add an OpenXR sync. SPF activation reuses the early-owned hooks and takes over polling; the worker exits. Explicit plugin unload stops early polling.

The launcher prefers the installed bridge directory over development copies. Normal launch enables controller input. The diagnostic `-DisableControllerBridge` switch removes the explicit layer request and path and sets the manifest disable flag for that child process only.

The user confirmed early pointers and clicks work and left-eye lag/tearing are absent after a full SteamVR restart with the latest paired binaries. Earlier comparisons without restarting SteamVR were inconclusive; one attempted bridge-disabled run still loaded the bridge because an explicit layer request remained. A corrected diagnostic run was verified bridge-free. The final enabled run was verified to load both DLLs from the installed directory. This establishes the tested working state, not a definitive cause for the previous stereo issue. If stereo timing remains disturbed across ATS restarts, fully exit and restart SteamVR before comparing builds.

The early startup log is `logs/controller_menu_early.log`. In the final test, early host readiness, surface capture, and native cursor updates occurred before SPF plugin activation. All three regression suites pass, covering native geometry, cursor command freshness/consumption, bridge sync and focus behavior, and menu hook installation/restoration. The unsupported-host guard was also checked. Cabin behavior after loading a save still needs separate coverage for this startup revision.


<a id="differential-test"></a>

## DIFFERENTIAL_TEST.md

### 0.2.3 diagnostic comparison — not a confirmed visibility fix

The 0.2.2 log confirms mounted version and stock carpet selection, without a stock actor/model load error. Tifa workshop item 3810525140 is the working passenger reference. The two added entries isolate the rendering path; no Tifa assets are copied or modified.

Test A: choose Diagnostic A - Tifa actor on carpet. It references the exact installed tifa_seat animated hookup, compensates for the carpet transform, and moves the known toyseat pose sideways onto the driver position. Check for the conspicuous Tifa character. This tests whether any known working actor can spawn from the carpet wrapper.

Test B: return carpet to original, then in passenger/toyseat accessories select Diagnostic B - Stock driver on toyseat. This uses our same headless stock model, skeleton and neutral animation through the known toyseat category; the mount compensates for that attachment to place the driver in the original intended location. This temporarily replaces the selected passenger accessory; reselect Tifa to restore it afterward.

Report A visible yes/no and B visible yes/no (T-pose also counts as visible, but report it).

A no / B yes points to the carpet actor path and supports moving to direct animated-interior integration. A yes / B no points to our stock actor registration/model/clip rather than nested carpet support. Both no leaves registration/instantiation or placement unresolved and warrants an actual engine observer rather than another fit tweak. Both yes means the combination or original hookup instance needs inspection. These are diagnostic outcomes, not definitive causes without runtime log corroboration.

The existing procedural and SPF Stock Driver Body options remain intact. No test alters the rig or bakes the body into static geometry.


<a id="engine-start-delay"></a>

## ENGINE_START_DELAY.md

### Engine Start Synchronization Prototype

Version 0.14.0 holds eligible player engine-start requests until the ignition hand reaches the key. The original request routine was first observed unchanged: two normal starts arrived with start=1, immediate=0 and prior engine state off, then the game entered starting; two stops arrived with start=0, immediate=0 and prior state running, then the game entered stopping.

A queued request starts the existing ignition reach while the engine stays off. After the pose is applied successfully at full reach with runtime IK, the animation publishes its matching request sequence. SPF's update callback replays the original engine-control routine once. The subsequent starting-state transition does not trigger a duplicate hand animation.

The queue requires the guarded player vehicle, engine off, electricity enabled, fresh controls and a recently observed private body. It is restricted to identified player input caller sites. Unknown caller paths, immediate/forced requests, and non-player requests pass to the game unchanged. Automatic-start path exclusion still needs runtime testing.

A stop request or electricity being turned off cancels the queued start and withdraws the hand. Vehicle changes, world reload, invalid/stale data or another engine state discard the queue. If the animation cannot report arrival while the vehicle remains valid and powered, a two-second timeout releases the start rather than blocking it indefinitely. Repeated eligible requests do not restart the delay.

Build and regression checks pass for queue/release once, duplicate requests, stop cancellation, vehicle/world changes, stale data, timeout, manual caller guards, untouched argument forwarding and suppressed-call branching. The replay uses the observed three arguments; static callers do not consume its return. Previous body/head, signal and elbow features remain unchanged.

Runtime synchronization passed the user test: three requests were released at key arrival, each entering the starting state, with no timeout fallback. The observed manual caller was the guarded toggle routine. The user approved the result and requested this commit. Quick cancellation and automatic-start behavior have not been observed in game; their intended handling is covered by regression tests and caller restrictions. The installed plugin is backed up before replacement; the body archive and calibrated files are preserved.


<a id="exterior-head"></a>

## EXTERIOR_HEAD.md

### Intact tracked driver head

Validated release: plugin 0.16.13 with the unchanged 0.7.0 full-driver archive. The archive restores the original installed driver mesh and its unchanged 61-bone skeleton. Animation/attachment files and installed game material/texture references remain intact.

The approved final state keeps the full head and neck visible and tracked in both interior and exterior camera views, with zero interior offset and no collapse. The user confirmed no head hiding workaround is needed in first-person VR. The existing guarded player exterior replacement still hides only the original player driver outside, leaving traffic unmatched.

#### Approved live configuration

Copy these files from releases into the existing plugin config folder:

- `interior_head_mode.txt`: `visible`
- `interior_head_backshift.txt`: `0.00`

Both reload twice a second outside the skeleton callback. Head orientation follows the calibrated headset pose in both views, with world-space correction accounting for torso lean. The head's default shape and position are preserved.

Optional diagnostic controls remain available: `collapse` maps the head and neck to the actual neck origin, and a signed offset moves them along model Z (positive rearward, negative forward). These controls affect only interior rendering. Invalid/missing/incomplete edits retain the last valid settings. The final approved configuration leaves both workarounds off.

#### Verified render-palette layout

The live buffer and original assembly routine establish row-major affine matrices with translations at elements 3, 7 and 11. Previous tests incorrectly used column-major translation entries 12�14, which actually belong to the homogeneous row. That caused offsets to change W rather than XYZ and made collapsed head vertices fall to zero, producing the reported stretched polygons. An earlier root-axis change did not resolve this layout mistake.

The render hook sees world bone transforms before subsequent inverse-bind multiplication. Collapse uses the neck matrix origin directly rather than transforming its bind position again. Corrected head collapse and intact offsets retain a (0,0,0,1) homogeneous row. Whole-body palette collapse also uses the verified translation entries and correct root origin.

#### Validation

Regression tests include row-major translated/rotated fixtures, homogeneous-row preservation, unchanged other bones, signed model-axis offsets, invalid input without mutation, and vertex transforms after inverse-bind multiplication. These prove collapsed vertices remain at the neck with W=1 and intact offsets affect the intended translation.

The user confirmed the corrected collapse looked better, then confirmed the intact head moved with a ten-centimetre rearward offset. Render logging recorded exactly (0,0,0.10) for that test and (0,0,0) after resetting it. The user selected visible mode, zero offset and tracking in both views as the final state. The installed plugin and two live head settings match the release files. See interior_head_validation.json.


<a id="gesture-head-follow"></a>

## GESTURE_HEAD_FOLLOW.md

### Head-directed hand gesture prototype

0.17.1 lets either engaged middle-finger gesture follow calibrated headset yaw and pitch. The upper-arm rotation changes at the shoulder, moving the whole arm/hand around that pivot without changing local finger joints or segment lengths. A leftward look swings the active gesture left, and a rightward look swings it right. The response is weighted by the gesture's smooth reach blend, so release returns to the current steering pose. Looking up or down swings the gesture arm vertically; roll is excluded. Pitch is applied about the yaw-directed horizontal shoulder axis.

The shoulder adjustment is applied after the live gesture pose and before torso IK. Torso compensation therefore preserves the updated hand target, rather than holding it at the previous fixed gesture point. Existing horn/signal/ignition priority and opposite-arm steering remain. Both camera views use the same calibrated head pose. Stale head tracking uses the existing fade toward neutral.

`config/gesture_head_follow_gain.txt` reloads twice a second and accepts a finite value from 0 to 1. Default 1.0 follows the full calibrated yaw and pitch; 0 disables following. Invalid/missing edits retain the last accepted value. The head and torso strength/limits retain their current settings. Tests cover both arms through every steering frame, positive/negative yaw, shoulder-pivot position, arm lengths, unchanged other local joints/fingers, release and torso IK retaining the moving hand target. The yaw response has been visually validated.

The user validated yaw follow before requesting pitch. Combined yaw/pitch tests now cover both arms and the full steering range, untouched local finger joints and torso IK retaining the new targets. The combined build is installed and regression-tested; a separate visual confirmation of pitch was not recorded before the user requested commit and push.


<a id="head-tracking"></a>

## HEAD_TRACKING.md

### Live VR head and torso tracking

Validated release: 0.16.13. The user confirmed torso tracking inside and outside and exterior head animation. The final runtime snapshot recorded more than twenty-two thousand torso updates and more than three thousand exterior head updates with no rejected solves; the last hand-target error was below a millimetre. See head_tracking_validation.json.

#### Tracked pose and calibration

A guarded read-only observer runs after the game's local-space OpenXR xrLocateViews call. It averages the two eye positions and hemisphere-normalizes orientation, preserving tracking-space identity and predicted display time. The view-relative query is excluded because its pose describes eye offsets rather than room movement. Successful result, stereo count, structure layout, finite values, quaternion norms and eye separation are checked. Torso application additionally requires valid and tracked position/orientation flags, fresh controls and recent private-body observation.

A seated baseline is captured in the interior camera. Use your usual ATS reset-head-tracking control once while seated neutrally to calibrate. The native action is observed without hardcoding its physical binding. Switching to an exterior view retains calibration. On world/tracking-space change, exterior mode waits for interior calibration or native recenter. Unknown cameras disable application. ATS recenter now automatically recalibrates the body from the next fresh headset sample; see [NATIVE_RECENTER.md](#native-recenter).

#### Body and head behavior

Head translation drives a restrained three-spine-joint lean, and one fifth of head yaw drives a torso turn capped at ten degrees before gain. Inputs smooth over approximately a tenth of a second. Hips and legs remain seated. Analytic arm IK preserves the current wrist targets and orientations, including steering re-grabs and active horn, gestures, signals or ignition. Unreachable combinations reduce lean transactionally instead of stretching the arms. Calibrated elbow guides still apply afterward.

In interior and exterior views, calibrated headset rotation drives neck and head joints, with a world-space target compensating torso lean rather than applying its rotation twice. Correction is distributed 65 percent to neck, with the residual at the head. Total head rotation is bounded to 85 degrees. The user-approved first-person head remains intact with zero offset; the guarded player exterior replacement remains active. Lost tracking fades toward neutral. This approximates torso motion from head tracking rather than reconstructing full-body pose.

#### Live configuration

Copy both files from releases to the plugin config directory. They reload twice a second outside the animation hook. Missing, invalid, oversized or incomplete edits retain the last valid values.

`head_tracking_gain.txt` accepts one number from 0 to 1; the approved value is `1.0`. Zero fades motion to neutral.

`head_tracking_limits.txt` contains these ordered entries:

```
forward_m 0.70
max_lean_degrees 40
lateral_m 0.60
```

All three range settings accept finite, non-negative values without arbitrary upper caps. Older two-line files default lateral range to 0.24 metres. Backward input remains capped at 0.08 metres and vertical input at �0.08 metres before gain. The limits constrain input rather than guarantee equal physical torso displacement: hips stay fixed and spine motion distributes the lean.

#### Validation and references

Regression tests cover observer frame/argument/result forwarding, stereo midpoint, quaternion signs, validity/separation checks, all steering frames, forward/back/side lean, hand position and orientation preservation, horn/gestures/signals/ignition, untouched hips/legs/fingers, calibration, exterior baseline retention, recenter, space changes, live-setting parsing and exterior yaw/pitch/roll without double torso rotation. The installed DLL and head configuration files match the release artifacts. The body archive and existing live arm poses are retained.

Structure layouts follow the [Khronos OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK/blob/main/include/openxr/openxr.h); the existing game query is described in the [xrLocateViews reference](https://registry.khronos.org/OpenXR/specs/1.1/man/html/xrLocateViews.html). The eye midpoint is a head-centre proxy, not an anatomical neck joint.


<a id="horn-integration"></a>

## HORN_INTEGRATION.md

### Left-hand horn prototype

The horn prototype uses a compact native pose blend after steering and pedal tracks are sampled, before hierarchy assembly. It writes only 21 left-arm and finger rotations (shoulder, upper arm, forearm, hand, fifteen finger joints and two twist bones). The right arm, torso, legs, positions, scales and animation resource remain untouched. The released pose uses current steering each frame.

The hand reaches over 0.25 seconds and presses over 0.08 seconds. Release lifts before withdrawing; interrupted taps reverse the reach. Palm press depth is 12 mm. The first wheel-center estimate is the midpoint of the neutral wrist positions, so visual alignment still requires a VR test.

Keyboard H is mirrored through SPF with consume=never. The active ATS profile also has joystick button 19 assigned to horn, but SPF currently identifies joystick buttons without device identity. This first test uses H; wheel-button integration is pending.

Runtime observation validated the private 61-bone pose assembly hook. Sampled left-arm rotations matched source interpolation within quaternion distance 0.000897, with held and released keyboard states recorded. The four temporary arrays are pre-rotation quaternion, scale, rotation quaternion and position. Initial diagnostic position logging mistakenly read scale; the prototype corrects that offset.

Executable SHA and unique entry signatures guard hooks. The callback checks private model identity, matching output pointer and bone count, then validates finite normalized rotations before applying the compact blend. A caught write fault disables further horn overrides. Assembly forwarding tests cover all five arguments and the return value. Tests also cover shortest-arc quaternion blending, release sequencing, current steering grip, and untouched non-left-arm bones.

Release: SPF_DriverAnimationDiagnostics-0.6.1-horn-test.dll. Compatible with the installed steering/pedal body archive; no atlas expansion or mod replacement is required. Previous plugin is backed up. Visual horn validation is pending.

The user reported that the first gesture looked good and requested a two-centimeter lower target plus a flat hand. Version 0.6.2 lowers both hover and press targets by 2 cm and opens the fingers using the rig's bind-pose rotations. These finger rotations blend from and back to the live steering grip. The adjusted pose passes offline checks and still needs visual validation.

Version 0.6.3 corrects the wrist orientation following the user screenshot. The previous open fingers retained the neutral wheel-grip wrist rotation. The builder now estimates the wheel plane from stock wrist positions across steering frames, constructs the open hand basis from middle finger direction and knuckle span, and maps it onto the wheel surface with fingers upward. Assertions verify the palm plane and finger direction, wrist target, preserved arm lengths, and unchanged other bones. The plane is an estimate because re-grabs move off the wheel; visual validation is still required.

Version 0.6.4 follows the user feedback that wrist orientation is almost perfect: both targets move another 3.5 cm down and 2.5 cm forward. Total vertical offset is now 5.5 cm below the original wrist midpoint. The verified orientation, finger pose and timing remain the same. Offline pose checks and native regression pass; final reach alignment is pending visual confirmation.

Version 0.6.5 moves the horn targets another 4 cm down and 3 cm left, retaining the previous 2.5 cm forward reach, wrist orientation and gesture timing. Total offset from the original wrist midpoint is 9.5 cm down, 3 cm left and 2.5 cm forward. Pose checks and native regression pass. The plugin is installed and ATS relaunched; visual confirmation is pending.

Version 0.6.6 addresses clipping shown in the user screenshot. Interpreted correction: move both horn targets 2.5 cm toward the driver and 2.5 cm right. Height, wrist orientation and timing are preserved. This restores original depth and leaves 0.5 cm of the previous leftward shift. Pose checks and native regression pass. Installed after ATS closed; launch requested. Visual clearance check is pending.

#### Final validation

The user confirmed the final 0.6.6 pose is good in VR and authorized committing it. The corrected hand clearance, open palm and wrist orientation are visually validated. The latest runtime log recorded 2092 horn overrides and 0 caught horn faults. Keyboard H is the validated input; wheel-button support remains pending.

#### Game horn input discovery

The user requested using the game horn trigger to include secondary bindings. SPF controls telemetry does not expose horn. Static tracing located the game query_variable("horn") branch and its held-state read through owner+0x31b0, vehicle+0x1d38, then byte+0xf0. The initially found vehicle_horn_control class appears related to tooling and is not used as the gameplay signal. A 0.7.0 observation plugin reads bounded candidates from the SPF player actor and local controller, without changing the validated H-driven gesture. The correct root and held/released behavior must be correlated in game before switching animation input. The plugin was built, tested and installed with the previous working plugin backed up.

The first game-input runtime check yielded unreadable actor/controller candidates. The 0.7.1 observer corrects the root to g_Game, identified independently by SPF session and viewport finders at executable RVA 0x36b8db0. The actor candidates remain observation-only for comparison. No game-state candidate has yet been validated or used to drive animation. Installed with ATS closed and relaunched for another binding correlation test.

#### Shared game horn signal

Live read-only inspection identified the true query owner through g_Game+0xaa0. The verified chain is game global RVA 0x36b8db0 -> owner at game+0xaa0 -> vehicle at owner+0x31b0 -> horn object at vehicle+0x1d38 -> held byte at horn+0xf0. The owner virtual query slot at vtable+0x1b8 must equal executable base+0x4d9c90. A direct live watch captured both secondary and H press/release pairs.

The 0.7.2 plugin uses this game state instead of a separate SPF keybind. Missing data, wrong owner and invalid Boolean release the gesture. The existing executable fingerprint guard applies. Tests cover the complete guarded pointer chain, pressed/released state and rejection behavior, alongside the gesture regressions. Installed after ATS closed; final secondary-binding animation check is pending.

The user confirmed that the secondary binding now animates the hand correctly and authorized committing the game-state input update. The final runtime recorded 2718 horn pose overrides and 0 caught horn faults. Primary and secondary bindings use the same game horn signal; no SPF-specific hotkey is read.


<a id="ignition"></a>

## IGNITION.md

### Ignition Animation and Runtime Arm IK

Version 0.11.5 is built, installed, and visually validated by the user. The engine start signal follows ATS's own vehicle state and therefore its configured bindings. The read-only query uses the same guarded player-vehicle owner as the horn; observed transitions were off, starting, running, stopping, and off. Automatic starts can also trigger the animation. Loading an already-running vehicle does not trigger it.

The right hand follows an intermediate reach waypoint, reaches the calibrated key target, turns 70 degrees in the key-on direction, then returns to the current steering grip. Runtime wrist translation holds the thumb's distal bone as the pivot throughout the turn. The editable ignition pose file reloads twice a second, retaining the last valid pose after an invalid edit.

During ignition, runtime two-joint arm IK compensates for a forward torso lean. The left hand keeps its current steering or gesture target and orientation. The right key target gains another 15.24 cm forward reach. The torso starts at 25 degrees and increases if required, up to 46 degrees. Hips and legs stay seated; fingers retain the current animation. Failed solves leave the prior pose intact. A five-second bound prevents a stuck start state from holding the gesture indefinitely. Ignition takes priority over the right-hand gesture, which returns first.

#### Install

With ATS closed, install `releases/SPF_DriverAnimationDiagnostics-0.11.5-both-elbows-test.dll` under the existing plugin DLL name. Copy `ignition_pose.txt`, `left_elbow_inset.txt`, and `right_elbow_inset.txt` into its config folder. Preserve existing left/right gesture pose files and the tested body archive. The executable fingerprint and private actor identity guards remain in place.

#### Validation

Regression tests cover the complete 47-frame stock steering animation, staged ignition reach, turn and direct return, unit-scale checks, invalid input, unreachable targets and unchanged unrelated bones. Both wrist position/orientation and thumb-pivot error are checked across the turn. Tested hand/pivot position error is below one millimetre. The latest runtime session recorded two ignition starts, 308 torso-IK updates, zero IK rejections, and zero caught pose faults. The user reported the final version looks good.

The installed plugin and three new live files match the release copies byte-for-byte. Ignition alignment is calibrated for the Mustang. Other vehicles and rigs are not yet validated. Startup load delays were reported; logs showed IK inactive while loading, so their cause has not been isolated.


<a id="left-elbow"></a>

## LEFT_ELBOW.md

### Independent Live Elbow Clearance

Version 0.11.5 is installed and visually validated. Per-frame two-joint IK applies inward elbow-guide offsets across steering, horn, gestures and ignition, preserving each hand's current position and orientation.

- `left_elbow_inset.txt`: 0.06 metres, positive values guide the elbow right/inward.
- `right_elbow_inset.txt`: 0.04 metres, positive values guide the elbow left/inward.

Each file contains one decimal value in metres, accepts 0 to 0.15, and reloads twice a second. Zero disables that side's correction. Invalid edits keep the last valid value. A guide offset is not a guaranteed equal elbow displacement: the result depends on the shoulder-to-hand geometry and available bend. File reads stay outside the skeleton hook.

Runtime checks require fresh controls telemetry, unit scales, finite positions, reachable targets, and wrist error below one millimetre. Failed solves leave the pose unchanged. Tests cover stock steering frames and both hands' gestures/ignition. The latest validated session recorded 5,427 updates per elbow, zero rejections, and zero caught pose faults. The user confirmed the final appearance.


<a id="live-pose"></a>

## LIVE_POSE.md

### Runtime gesture pose editing

The installed 0.9.0 plugin watches config/gesture_pose.txt in its own SPF_DriverAnimationDiagnostics directory. Save the file while ATS runs: validated targets become active within about half a second, including while F10 is held. One game restart is needed to install this plugin; subsequent pose edits do not need rebuilding, restarting or toggling the plugin.

The file begins with SPF_GESTURE_POSE 1. Each bone row has its index, then four open/reach quaternion values and four held-gesture quaternion values, in w x y z order. Comment lines identify bone names. All 21 left-arm and finger rows must remain present. Current F10 controls, formation timing, horn priority and pedal animation are unchanged.

Only these left-arm bones are allowed: shoulder 4, upper arm 5, forearm 6, hand 7, finger joints 8�22 and twist bones 23�24. Missing/duplicate bones, extra values, invalid numbers, zero/non-unit quaternions and files larger than 64 KiB are rejected. Small normalization drift is corrected. Failed or partial edits retain the last valid pose. If no valid file has loaded, the compiled pose remains active. Removing the file retains the last valid loaded pose.

File reading and parsing occur outside the animation hook. A validated immutable snapshot is published atomically so the hook sees a complete pose. Log messages beginning POSE_RELOAD identify accepted generations or rejected edits.

build_window_gesture.py exports releases/gesture_pose.txt as well as the fallback C++ targets. To apply future generated geometry changes without rebuilding the plugin, copy the generated pose file to the watched installed config file. Manual quaternion edits can be saved directly. This first implementation live-reloads the middle-finger gesture, not the horn or pedals.

Parser and snapshot tests cover valid publication, existing-snapshot immutability, partial/duplicate/unknown bone rejection, invalid quaternions, size limits, normalization, comments and the live gesture blend. Runtime watcher validation passed: initial load and a comment-only save were accepted while ATS stayed running. The first live geometry edit then added 5 degrees at each curled index joint; visual feedback remains pending.

The user confirmed the live index curl update works. A subsequent runtime file update turns the wrist palm modestly toward the driver while preserving the hand target and compensating the middle finger to remain vertical. Pose checks pass; visual orientation feedback is pending.

#### Three-keyframe pose file

Version 0.9.1 adds a distinct intermediate pose. The current file header is SPF_GESTURE_POSE 2. Each bone row now contains three quaternion groups in w x y z order: reach/open, intermediate finger-down fist, and final held gesture. All three groups can be edited live. Version-1 two-pose files remain readable for compatibility.

During reach the middle finger stays down while the other fingers form the fist. Only after the hand reaches its target does it transition from intermediate to the final raised middle finger. On release the finger folds down first, then the fingers open and the arm withdraws. The existing faster reach timing, hand target, current joint refinements and horn priority are retained. Tests cover the intermediate arrival state, no early extension, incomplete version-2 rejection and all existing regressions. Installed with ATS closed and launch requested; visual sequence validation remains pending.

The user confirmed the arrival sequence looks good but observed an unwanted open-hand stage on release. Version 0.9.2 changes release to begin arm return immediately, fading the current gesture into the current steering grip as it moves. The intermediate finger-down pose remains on entry. Tests verify immediate return without the extra exit pose. This changes compiled timing logic, so installation needs one restart; pose-file editing remains live afterward. Built, tested and installed after ATS exited. The existing live pose file was preserved and launch requested; release visual validation is pending.

#### Final confirmation

The user confirmed the final finger-down entry, held pose and direct return work great. The latest runtime logged 1405 gesture pose updates with 0 caught pose-write faults. The installed pose file matches the committed release file. Runtime reloads and live joint edits were confirmed during the earlier tuning session.

A later runtime left-only edit moves the left hand target 6 inches (15.24 cm) left through shoulder/clavicle and two-joint arm retargeting, preserving arm lengths. The right pose file is not replaced. Visual confirmation of the left reach is pending.


<a id="mirror-interaction"></a>

## MIRROR_INTERACTION.md

### Controller mirror adjustment prototype

Three setup boxes select left, center, and right mirrors. Either enabled controller hand or disabled-hand controller axis may overlap one; that hand's thumbstick adjusts the mirror horizontally and vertically, with a 0.2 deadzone. Leave the box or release the stick to stop. Within a mirror box the stick does not toggle finger assignment. When both hands overlap boxes, the larger stick deflection wins (left on ties). No mirror actions run during setup.

F9 exposes all five labeled boxes. Move/rotate with one grip; resize one local dimension with two grips. Release to save. Version 3 of interaction_boxes.txt includes all five boxes and accepts older two-box files without losing the ignition/horn calibration. Mirror default locations are initial Mustang estimates and need VR alignment using setup mode. Boxes remain invisible in normal play.

#### Native adjustment dependency

The installed SPF_MirrorControls plugin from scegielski/SPF_AdjustmentHotkeys owns the native hooks. A small exported controller-input lease reuses its existing selector, direction branches, native setter, limits, and refresh. The driver plugin registers no competing mirror hooks. The mirror plugin must remain enabled. Requests expire after 150 ms, and the native plugin's pause/focus/version guards remain in force. Left/center/right map to physical slots 0/4/2, as verified for the Mustang in the reference plugin; other vehicle mappings remain unverified.

The matching reference-repository patch is mirror-controller-bridge.patch; copy native-diagnostics/MirrorControllerBridge.hpp to that repository before applying/building the patch. Both modified source files are also in the adjacent local SPF_AdjustmentHotkeys checkout. The staged releases/SPF_MirrorControls-controller-bridge.dll supplies the required export. The OpenXR input bridge also needs the paired update to expose both stick axes.

Validation: driver and controller-layer regressions cover box migration/round trip, mirror overlap/slot routing, and both stick axes. Reference mirror regressions cover actual detour selection/direction scope, one-call forwarding, neutral/released and expired requests. Physical mirror-box placement and adjustment direction still need an in-game check.

When hand tracking is disabled, a controller-centered collision box operates mirrors and horn directly, with right-controller trigger operating ignition. No hand calibration is required for this mode. Setup displays these boxes around the axes; collision wireframes remain hidden outside setup. Enabled hands retain their calibrated palm boxes. Tracking loss and stale scene frames invalidate both modes.

Controller mirror adjustment uses the reference plugin servo.wav loop and its servo_sound setting. The same short input lease selects stereo routing and movement: left/center/right routing, silence in the stick deadzone, and stop on contact loss, setup, pause, or expired input.

Axis mode now emits a two-metre solid scene laser along each controller's local negative Z axis. The nearest rotated/scaled interaction box wins, and the beam ends at that box. Mirror stick adjustment, horn targeting, and right-trigger ignition use the ray hit; enabled hand mode keeps palm contact. Setup displays beams but disables actions. These rays test the interaction boxes only, without world-geometry raycasting. Ray tests cover nearest target, backward/parallel misses, inside origin, rotated and nonuniform boxes. Visual pointing direction needs VR validation.

Horn activation now requires the targeting controller trigger above 0.7, in both palm-contact and laser modes. Either controller may operate it. Releasing the trigger or losing the target stops that controller contribution; keyboard/wheel horn forwarding remains unchanged.

#### Seat box
A sixth labeled SEAT box starts between the legs at a Mustang estimate and is movable, rotatable and scalable in F9 setup. Both hand contact and axis-mode laser targeting route the controller stick to native seat mode 6 in Adjustment Hotkeys. Stick up/down adjusts height; left/right adjusts forward/back using that plugin's established mapping, native vehicle defaults/limits and apply routine. Its existing centered servo plays during adjustment. Version 4 of the saved box format accepts the older two- and five-box formats while preserving the new seat default. The horn-trigger requirement is included in this update. Seat placement, direction and in-game camera behavior require a VR check.

#### Window boxes
Two additional labeled L WINDOW and R WINDOW targets begin near the doors and support the same F9 setup placement, rotation and scaling. Target with enabled hand contact or disabled-hand laser; stick up requests close, down requests open, and neutral releases input. The game retains its own electrical gating, motion, limits and sound. A native entry detour forwards once and changes only open/close flags for exact player window-state identities; manual input takes priority. No other vehicle or window-state object is changed. Input is disabled in setup, pause, focus loss and stale tracking. Version 5 saves eight targets and accepts older saved two/five/six-target files. Initial positions and runtime behavior need in-game validation.

#### Steering-wheel box
A ninth WHEEL target supports hand contact and laser pointing. Stick left/right retracts/extends the column and up/down increases/decreases its tilt using the reference plugin's existing native modes 7 and 8. A combined leased mode 9 applies both axes independently, preserving the other axis and enforcing native defaults and limits. Centered servo feedback follows stick movement. F9 placement, rotation, nonuniform resize, and automatic saving are shared. Version 6 saves nine targets and reads all prior two/five/six/eight-target files. Initial wheel-box placement and column behavior need an in-game check.

#### Lights box
The tenth LIGHTS target starts twelve centimetres left of the saved ignition box until placed in setup. Either enabled-hand contact or axis laser targeting permits one native light-cycle event per fresh trigger press; held triggers do not repeat. The guarded player input-site hook uses ATS's ordinary light-cycle branch and preserves its keyboard/wheel input. Setup, pause, focus loss and stale data suppress controller cycles. Version 7 saves ten targets and reads prior formats. Trigger and actual assembly forwarding tests pass; the native state cycle still needs in-game validation.

#### Wipers box
The eleventh WIPERS target starts ten centimetres above LIGHTS until authored in setup. Either hand contact or controller laser plus a fresh trigger press sends one event through the game's normal wipers-cycle branch. The game chooses its supported speeds and handles motion/sound. Native keyboard input is preserved; holding trigger does not repeat. Version 8 saves eleven targets and accepts earlier formats. Assembly forwarding and target persistence regressions pass; runtime cycle still requires a visual check.

Controller wipers now explicitly wrap from highest supported speed to off, using the native vehicle-dependent state count and setter. Keyboard increase/decrease behavior remains untouched. A simultaneous native key press takes priority to avoid double advancement. The state-count and setter addresses are gated by unique signatures on the inspected executable.

#### Radio box
A twelfth RADIO target starts near the center dashboard and shares setup editing, hand contact and laser targeting. Left/right stick nudges request previous/next through the native player source selection; neutral rearms the next nudge. Held up/down changes radio volume with the native clamp and frame timing. Current player source and station availability follow ATS (music mode can select tracks). Native keybinds are preserved, and setup/focus/pause/stale tracking stop controller input. Version 9 saves twelve targets and migrates older placements. Regressions cover stick edges, held volume, losing target, serialization and the actual observation stub; radio operation and positioning need in-game validation.

Controller station tuning now selects the next/previous entry of the full native internet-radio list, wrapping at both ends, rather than invoking the favorites-filtered hotkey helper. The exact internet-radio vtable and selector signature guard access, with count/current bounds validated before the native selector call. Selection logs show before/requested/after indices and count. Favorites and stream URLs remain unchanged. Volume behavior is retained. Native selector execution still needs an in-game check.

An original synthesized short radio-tuning sound plays once after a native station selection is accepted (verified index), providing immediate feedback during stream buffering. Playback is queued to the ordinary plugin update, uses the existing controller audio output, and does not depend on the internet stream. No continuous playback while the stick is held.

Radio trigger: a fresh press while touching/pointing at RADIO toggles the native pause bit on the existing stream, preserving the selected station. If no stream exists, it starts the selected station (first entry if none selected). Trigger hold does not repeat; setup, tracking validity, pause and focus guards apply. The native pause routine has its own unique signature check. Volume/station-stick controls remain unchanged. Runtime play/pause needs a visual/audio check.

Radio first-use requests load the native station list before selecting a station or starting playback. Accepted trigger play/pause actions queue a short click (`controller_radio_click.wav`) outside the animation hook. The click build is packaged; installation and its audible check are pending.

Ignition accepts independent fresh trigger presses from either hand or pointer. Radio trigger feedback uses `controller_radio_on.wav` for play/resume and `controller_radio_off.wav` for pause, based on native playback state. The directional-feedback build has passed regressions but is not installed yet.


<a id="native-recenter"></a>

## NATIVE_RECENTER.md

### ATS-bound head calibration and editable ranges

Validated release: 0.16.7. Calibration follows ATS's headreset action rather than a separate SPF hotkey. The static action registration and input handler combine keyboard, wheel and semantic bindings. An observer at RVA 0x428872 records the native action edge without consuming or changing it. It waits for a fresh, tracked headset sample captured after the request, applies calibration once, and expires after two seconds without usable tracking. Pending requests clear with world reset. The old F8 action is no longer registered or polled; stored user binding settings are retained.

Runtime logs captured one native action and one subsequent calibration, approximately twenty-two milliseconds later. This validates the action path for the tested binding; it does not independently test every possible physical binding. Regression checks cover unchanged registers/edge, post-request sample ordering, invalid tracking, single application and timeout.

The latest calibrated live limits are 0.70 metres forward, 0.60 metres symmetrically sideways and 40 degrees of spine lean. Their loader now accepts any finite, non-negative numeric values, with no arbitrary upper cap; zero is accepted to disable an axis/angle allowance. Negative and non-finite values and malformed files retain the last valid settings. Changes apply live within about half a second.

Physical arm reach and finite-pose checks remain. The IK solver reduces unreachable leans transactionally while keeping the hands at their current targets. A large configured range therefore does not guarantee equal visible displacement. Tests cover large accepted settings, negative/non-finite rejection and large forward/side requests throughout the stock steering range. The user approved the overall tracking result and requested commit and push; head_tracking_validation.json records the final session results.


<a id="older-mod-comparison"></a>

## OLDER_MOD_COMPARISON.md

### Older ATS Interior Driver comparison

The supplied desktop folder contains the inner ATS_Interior_Driver_1.00_for_1.39.scs and two readable skin replacement folders. The original outer ZIP is readable and has zero encrypted entries. The inner SCS has 532 central-directory entries claiming encryption, but sampled local headers have flags zero, a contradictory method field, and payloads that inflate directly as raw DEFLATE without a password. All 528 nonempty payloads inflate to their declared sizes; four entries are empty/directory records. Its altered CRC metadata does not validate, so generic ZIP readers reject it. The previous claim that its models required a password was incorrect. No password or decryption was needed to inspect the actual payloads.

Readable payloads were written only to work/ATS_VisibleDriverBody/older-driver-reference/readable-inner. Nothing from this mod was installed or executed.

The mod replaces existing animated interior models and their PMAs for nine trucks, plus materials. It supplies no def/ or unit/ replacements. Kenworth T680 has 89 bones including steering_w, pelvis, spine_01/02/03, clavicle_l/r, upperarm_l/r, lowerarm_l/r, hand_l/r, finger bones, neck_01 and head. Cascadia has 105 bones with the same human arm chain merged into dashboard/steering controls. Cascadia sw_rotation.pma is animation format v4, 45 frames, 43 animated bones. That is consistent with the wheel plus human arm chain; exact per-track mapping remains to be decoded because v4 headers differ from the Mustang v5 animation.

This is an integrated animated-interior architecture: the stock truck definition already points at those model paths, and its control animations now include driver tracks. The resulting wheel/arm movement can therefore be driven by the ordinary interior steering animation. This is a concrete architecture lesson, not proof that the 1.39 mod is compatible with 1.61 or Mustang.

Our prototype instead uses an independent 61-bone stock driver and vehicle animated_hookup mounted from the carpet accessory. It preserves the driver rig and uses an engine-played neutral seated clip. Version 0.2.1 corrects vehicle hookup registration to unit/hookup/vehicle and uses accessory_addon_int_data, matching the already-installed animated passenger accessory. The older mod does not reveal another missing dependency or registration fix in 0.2.1. Test 0.2.1 before changing that architecture again.

For future actual wheel synchronization, the older mod suggests merging the independent stock driver rig into the Mustang animated interior rig (currently 46 bones), remapping all existing animation indices, and adding driver tracks to steering-wheel/column animations. That is a separate implementation step; copying old truck models or animations would not produce a Mustang-compatible result. No live steering or pedal animation is claimed by our current hold-pose prototype.

#### October 11 steering compatibility correction

The proposed wheel-animation integration above relied on a pre-1.42 control. Official SCS documentation marks wheel_anim obsolete since 1.42, and the current Mustang definitions omit it. Rig merging alone is therefore insufficient to drive arms on ATS 1.61. See STEERING_INTEGRATION.md for the current evidence and unresolved input bridge.


<a id="pedal-alignment"></a>

## PEDAL_ALIGNMENT.md

### Pedal alignment adjustment

The user confirmed the initial brake reach/hover/press sequence works, but the foot lands to the right of the brake. Per the requested correction, the right brake reach is now 12 cm left from the gas position, compared with 8 cm in the initial test. The left resting ankle is shifted 6 cm left and remains static through every steering, throttle, reach and brake frame. No left pedal animation is introduced.

The 0.5.1 pedal-alignment archive uses the same 0.4.0 SPF plugin as the initial brake prototype. Only the archive manifest and private animation data change. All steering tracks and accelerator tracks outside the static left-leg adjustment remain byte-identical to the validated throttle release. The two-joint leg solves preserve thigh and shin lengths and keep both ankle orientations stable during lateral placement. The brake press remains the same 12-degree downward ankle rotation and its timing is unchanged.

build_brake_prototype.py now generates this adjusted archive with explicit BRAKE_REACH and LEFT_REST_SHIFT values. Its checks validate ankle targets, unchanged leg lengths, normalized rotations, correct PMA pointers and duration, static left-leg tracks across all 3901 frames, preserved gas/arm tracks and downward toe motion. See pedal_alignment_validation.json. The twelve-centimetre brake landing and six-centimetre left-leg rest still require a VR alignment check. Installation requires ATS closed because it locks the mod archive. The prior 0.5.0 brake archive is retained for rollback.


<a id="right-gesture"></a>

## RIGHT_GESTURE.md

### Right-hand gesture prototype

The installed 0.10.0 plugin adds an independent right-hand middle-finger gesture above the wheel. Hold F11; release to return directly to the current steering grip. Change the binding in SPF under Gestures.RightHandBird. The left-hand F10 binding remains intact.

The right hand reaches with its middle finger down, raises it only after arrival, and uses the same faster transition timing as the left hand. Its live pose file is config/right_gesture_pose.txt, with the same version-2 reach/intermediate/held quaternion format. The two loaders enforce distinct bone sets and retain independent last-valid snapshots. Editing one hand's file cannot target the other hand's bones.

The builder mirrors the validated left pose through stock bind frames and retargets the right wrist with two-joint IK. It preserves arm lengths and leaves left-arm, torso and pedal tracks unchanged. The initial above-wheel wrist target matches the left gesture. The right finger-down and raised stages pass geometric checks.

The native tests cover side-specific pose-file validation, immutable last-valid state, normalized right-hand blending, preservation of left hand/torso/pedals, and independence from the left gesture and horn. All tests pass. The right gesture can run while the left horn operates; the existing left gesture/horn priority remains unchanged.

The plugin and right pose file are installed, the left file is preserved, and ATS launch was requested. The right wrist, thumb/finger appearance, and transition alignment still need a VR check. No commit or push has been made for this prototype.

#### Final validation

The user confirmed the right gesture and both final offsets look good: left 3 inches left and right 3 inches right. Both installed live files match their release files. Runtime recorded 8084 left gesture updates, 7582 right gesture updates and 0 caught pose-write faults. The right target offset is specified independently of the left target for future tuning. Ship both live files with the plugin; the original compiled fallback targets are centered. The final offset files are user-validated.


<a id="roadmap"></a>

## ROADMAP.md

### Driver Body Animation Roadmap

Ordered from most feasible to most uncertain, based on the current validated Mustang implementation. This is a planning list, not a commitment to begin implementation.

1. **Vehicle-specific animation calibrations, where needed.** Add profiles for different cars and trucks to align steering grips, pedals, horn targets, and gestures with their cabins. Existing editable poses provide a starting point; automatic profile selection and support for different rigs still need investigation.
2. **Ignition animation � implemented and visually validated.** When the engine-start button is pressed, the driver releases the wheel with the right hand, drops the hand down and forward to the ignition, turns the key, and returns to the current steering grip. Reuse the arm pose system, identify a reliable game engine-start trigger, and calibrate the key position for each supported vehicle.
3. **Head-driven seated torso movement.** Use calibrated VR head movement to estimate restrained upper-body leaning and turning, keeping the hips seated. Arm IK should preserve wheel or controller hand targets. Head tracking access and runtime torso control need validation.
4. **Motion-controller hands.** Read tracked controller poses, calibrate VR space to the cabin, and solve arm IK each frame. Keep physical-wheel driving animations available.
5. **Toggle between controller tracking and driving animations.** Provide a configurable SPF action with a smooth return to the current steering grips, an optional instant snap, and a tracking-loss fallback. Depends on controller-hand support.
6. **Exit and re-enter the parked vehicle.** Coordinate a full-body exit and entry sequence, doorway clearance, body visibility, and camera movement. The vehicle stays parked throughout.
7. **Walk around the parked vehicle in VR.** Investigate whether the service/vehicle-browser walk camera can be used in the main game world and in VR. Movement, collision, body placement, and return to the seat remain unverified.

#### Feasibility notes

The order is provisional. Access to VR tracking, runtime skeleton control, and the walk camera could change it. A visual exit animation and a freely controllable walking mode are separate milestones.


<a id="runtime-fix"></a>

## RUNTIME_FIX.md

### Invisible animated body: 0.2.1 correction

ATS game.log.txt confirms 0.2.0 was mounted and the stock-driver accessory selected repeatedly. It contains no driver-model or animation-load error. The animated passenger mod already installed at workshop item 3810525140 registers its animated_hookup at unit/hookup/vehicle/tifa_seat.sii and uses accessory_addon_int_data. Ours registered at unit/hookup/spf_stock_driver.sii and used the ordinary rigid accessory class. Version 0.2.1 matches the installed working vehicle-hookup layout and interior accessory class. Model, rig, neutral seated clip, head masking and placement are unchanged. Its effectiveness still needs an ATS test.

The downloaded older driver mod was subsequently inspected successfully from the user's desktop folder. Its inner archive has malformed/protected ZIP metadata rather than password-encrypted payloads: 528 nonempty entries inflate directly without a password. The readable models merge driver arm bones into existing truck animated interior rigs; no definition or hookup replacements are included. See OLDER_MOD_COMPARISON.md. It was not installed or executed, and no content was copied into our mod.

Restart ATS, load the Mustang and reselect SPF Stock Driver Body (animated prototype) in carpet accessories. Because the accessory class changed, select the original carpet first, then select the stock-driver option. The procedural fallback remains separate.


<a id="steering-integration"></a>

## STEERING_INTEGRATION.md

### Verified live steering result

The user confirmed the 0.3.0 archive and 0.2.0 plugin work in first-person VR, with good alignment and natural hand repositioning on the wheel. Runtime logs show nonzero interior overrides and zero caught engine-call faults. See live_steering_validation.json. The entries below retain the discovery and pre-test history; statements about pending validation describe those earlier stages.

### Steering integration investigation — October 11, 2026

#### Verified baseline

The starting checkout was clean at 9306a1f (0.2.4). The PMA header regression passes using the bundled Python runtime. The installed archive named 0.1.0 and the repository's 0.2.4 release have the same SHA-256: db8d2f9d53b0a61900bbdc466290f4b972b4769c674d476ad435861a45d5b317. Neither package assets nor the installed archive were changed during this investigation. The user's prior VR visibility/placement confirmation remains the only runtime confirmation; no new in-game steering test was performed.

Run audit_animation_integration.py for a read-only audit. steering_integration_audit.json records both skeletons, varying tracks in the original driver clip, definition inspection and archive hashes. The stock rig has 61 bones; the Mustang animated interior has 46. Original clip variation includes arms, fingers, torso and feet, so replaying all 47 frames as a loop would introduce unrelated movement. Component variation alone does not establish the purpose, steering range or neutral midpoint of those frames.

#### Compatibility correction

The older 1.39 interior driver's merged rig is useful as a geometry/rig reference, but its input binding is obsolete. SCS's 1.42 migration guide says steering wheel animation was removed from accessory_interior_data and wheel_anim became obsolete:

https://modding.scssoft.com/wiki/Games/ETS2/Modding_guides/1.42

The current shared truck-interior animation reference also explicitly labels wheel_anim obsolete in 1.42:

https://modding.scssoft.com/wiki/Documentation/Engine/Truck_Interior_Animations_and_IDs

The extracted Mustang animations.sui contains no wheel_anim assignment. Its steering wheel is an interchangeable accessory. Adding driver bones to the interior and creating a wheel_anim PMA therefore does not establish a supported steering input binding on ATS 1.61. Do not ship that as live steering support.

#### Integration decision

Preserve the animated seated actor as the shipping baseline. Before implementing a merged-rig build, establish a current engine-supported way to sample an arbitrary arm animation from steering input. None was established by this investigation. If testing an undocumented bone/attachment behavior, use a separate diagnostic package and require actual left/center/right runtime evidence before investing in rig merging. Attaching the whole body to the rotating wheel would rotate the torso and legs; it does not meet the desired animated seated-body behavior.

An external runtime bridge would need both input acquisition and a verified API to set the actor's animation sample. Telemetry by itself supplies no proven actor-animation control. This is a separate unresolved architecture, not an implemented fallback.

The current official interior animation table supplies no dedicated throttle/brake/clutch pedal animation entries. brake_pressure_anim is an air-brake pressure gauge signal, not a general pedal-input binding for this Mustang. Foot tracks exist in the source clip, but live pedal synchronization remains unverified and unimplemented. Revisit it after the actor control path is established.

#### Next useful experiment

Identify a documented or experimentally confirmed current actor-animation input bridge. Then isolate arm tracks, reconstruct their poses relative to the actual wheel, establish the neutral frame and steering endpoints, and retain the existing hold pose for torso/legs. Only merge and remap the interior rig if that bridge requires it. A successful PMA build/conversion remains separate from runtime input synchronization.

#### Native bridge investigation

The adjacent SPF-Framework-1.2.5 source registers user and effective steering telemetry in src/Telemetry/SCSTelemetryService.cpp and stores it in ControlsProcessor.cpp. This establishes a source-level input acquisition path, not runtime verification of our body integration. Its GameCameraDebugAnimation interface controls camera animation only; inspection of include/ and src/ found no actor skeleton or PMA sample control API.

The local SCS SDK telemetry initialization structure exposes event/channel callback registration, not actor animation playback control. The official SDK description likewise covers vehicle telemetry and basic input devices:
https://modding.scssoft.com/wiki/Documentation/Engine/SDK/Telemetry

The newly documented 1.61 vehicle_animation_data types are suspension and ignored_anim. That interface does not document a steering-driven arbitrary actor clip:
https://modding.scssoft.com/wiki/Documentation/Engine/Units/vehicle_animation_data

Consequently a native plugin implementing this feature still requires discovering and validating an internal actor animation sampling hook for the installed ATS build. No actor address, layout, calling convention or safe playback hook has been established. Do not substitute the camera animation API or guess memory offsets. This investigation added no DLL, package replacement or claimed steering implementation.

#### Offline hook discovery — first pass

Added discover_animation_hook.py and export_animation_candidates.py. Dependencies are isolated under ignored work/discovery-deps (capstone 5.0.9, pefile 2024.8.26). The scanner reads the installed executable, fingerprints it, finds relevant ASCII anchors and candidate RIP-relative LEA/MOV references, and associates references with PE unwind ranges. It performs no process attachment, memory writes or game replacement.

First pass: 709 matching strings and 800 candidate references. Results are in work/animation_hook_candidates.json; selected disassembly is in work/animation_candidate_disassembly.txt. These include false leads such as sound/graphics drivers, so anchor relevance must be reviewed. PE unwind ranges can describe split function fragments rather than complete functions; do not use every range start as a hook entry.

Two stock driver model-path references occur at preferred-image addresses 0x1409e179f and 0x1409e1b30. Both enclosing code paths call 0x1409e1e40 after formatting the driver model path. Offline disassembly of this common routine shows calls into the model subsystem (including 0x1415d9ff0 and 0x1415e3200). This is a candidate model setup lead, not an identified animation sampler. Work files include driver_model_setup_window.txt. All addresses are build-specific preferred-image addresses and require relocation for live debugging.

Next: establish the correct actor from model setup, inspect its animation/controller references and trace the per-frame playback update. Confirm correlation against left/center/right steering while stationary and against an idle control case. Establish the full ABI and actor lifetime before registering a detour. An observation-only plugin cannot yet be built honestly because its target function ABI remains unknown.

At discovery time ATS was not running and no debugger was found on the command path. No live actor observation, steering trace, animation override or in-game validation has been performed.

#### User runtime evidence — exterior driver

The user reports that the exterior Mustang driver's hands move with steering and that the exterior model overlaps the new interior actor. This confirms user-observed runtime steering behavior of the exterior actor in the current setup. It does not establish that our hookup responds to steering. The camera/view in which the overlap was observed remains to be clarified; visibility in first-person VR could make reusing the already driven actor a materially different option from creating a new playback hook.

Continued offline inspection of the shared driver model-setup routine found model subsystem calls and a constructed auxiliary object. Its type and role remain unidentified; do not label it an animation controller without further evidence. Disassembly is retained under work/driver_controller_candidates.txt (candidate filename only).

The user clarified that the overlap is visible only using an exterior camera. There is no evidence that the exterior actor is rendered in first-person VR. Reusing its steering controller would therefore still require either enabling appropriate first-person rendering with head suppression or transferring its animation sample to our independently rendered actor. Do not treat the exterior-camera observation as first-person body steering success.

The extracted Mustang data.sii explicitly sets driver_animation to /asset/character/human/driver/$driver_type$/ford_mustang_67_ani.pma. This connects the source clip used by our seated hold to the exterior driver definition. The executable also has a nearby /vehicle/driver/%s/driving.pma path reference at preferred-image address 0x1409e146b; further analysis should trace that neighboring animation setup path before selecting any detour.

#### Runtime observation result

After enabling the plugin in SPF's Plugins panel (new plugins default to disabled), both observation hooks installed. The repeated user steering sweep produced 1,214 matched advancement calls, no rejected samples and no ledger overflow. One controller/model pair was observed through the full telemetry range -1 to +1. The clip duration was 1.916667 seconds, matching the original 47-frame driver clip. These are genuine runtime hook observations, distinct from compilation and the synthetic forwarding tests.

steering_runtime_validation.json summarizes the captured evidence. The raw copied sweep log is retained under work/steering_sweep_observer.log. Fresh records agree with the static piecewise mapping: negative steering corresponds approximately to its negation, positive steering approximately to 1 minus steering, and center to the start/end seam. Median discrepancy was 0.006546 normalized units; the snapshots are asynchronous, so this is evidence of correlation rather than exact same-frame error measurement. Do not use the common (steering + 1) / 2 mapping for this source clip.

No interior animation was changed. Version 0.1.1 of the diagnostic adds bounded, read-only identity observations of model instances using the model resource name chain inferred from game error-reporting code. Its purpose is to identify the private interior body by path before attempting animation control. The identity chain itself remains pending runtime confirmation. ATS had been closed when preparing this follow-up; no stale pointer from the previous process was reused.
### Live steering prototype, October 11

Runtime identity validation found separate exterior and private interior model instances passing through the same animation advancement entry. Both paths were readable, with no rejected steering samples. Evidence is saved in interior_identity_validation.json; raw logs remain under work/.

Version 0.2.0 of SPF_DriverAnimationDiagnostics now applies the exterior driver's reset-to-frame-zero / enable / advance sequence to the private interior model only. It validates the animation array bounds, 47-frame count and 1.916667-second duration. The exact executable SHA-256 and original entry signatures remain mandatory. Fresh effective steering determines playback position using the observed stock mapping. Stale or invalid telemetry selects neutral. A caught engine-call fault disables further overrides. Assembly preserves arguments and return behavior while replacing only the third floating-point argument; forwarding and replacement regression tests pass.

The 0.3.0 steering-test archive contains a private arm-only copy of the stock Mustang animation. The shoulder/arm descendants retain their original tracks; all other tracks are frozen at the existing neutral seated frame. The rig, geometry, attachment and neutral frame are unchanged. Only the archive manifest and animation reference differ from the validated 0.2.4 archive, plus the new clip. build_steering_prototype.py generates it without changing the seated-hold package sources.

The paired plugin/archive are installed with byte verification and rollback copies under work/pre-steering-backup. SPF's existing enabled state is retained. This is ready for an in-game test, but actual first-person arm steering has not yet been validated. Test parked in first-person VR: center, left, center, right, center; hold each briefly, then change views and confirm the seated torso/legs remain stable. Inspect STEERING override/fault records alongside the visual result.


<a id="stock-driver"></a>

## STOCK_DRIVER.md

### Stock Mustang driver — animated interior prototype

Select **SPF Stock Driver Body (animated prototype)** in Mustang carpet accessories after restarting ATS. The procedural body remains selectable as a fallback.

Uses the installed male driver model/rig, default Lukas appearance, with a private PMD/PMG path. Head-weighted triangles are made invisible while hands and the original 61-bone skeleton are retained. Stock materials and textures are referenced directly in base_share.scs; no copies of those textures are packaged.

The game animation system drives a two-frame hold clip derived from the neutral frame of the original Mustang animation. This is a rigged actor, not baked rigid geometry. The full original animation contains steering movement; this first milestone holds the neutral seated pose to avoid unrelated looping hand motion. Live wheel/pedal synchronization is not implemented.

The wrapper compensates for the carpet’s 180-degree Y rotation and subtracts the stock cabin interior origin, so the original vehicle-space pose is placed in interior coordinates. Software checks reconstruct the bind skeleton, skin the neutral pose, validate head removal while retaining hand faces, and verify stock dependency paths. Official SCS conversion of the mount succeeds. ATS actor loading, VR camera clearance and wheel fit still require an in-game test.

To test: restart, load Mustang, select the new stock-driver carpet entry, look down and around the wheel in VR. If it is invisible or remains in T-pose, report that and inspect game.log.txt for the hookup or animation load error. Re-select the procedural prototype or original carpet to revert.

Build source: build_stock_variant.py; diagnostic rig parser: diagnose_stock_rig.py; mount source: tools/base/model/spf_stock_driver/mount.pim and mount.pit. Validation: stock_driver_validation.json.

#### Runtime confirmation — October 11, 2026

The user tested installed 0.2.4 in Mustang VR and confirmed the body is visible and in the perfect position. This closes the visibility/placement milestone. Live steering and pedal synchronization remain unimplemented.


<a id="throttle-integration"></a>

## THROTTLE_INTEGRATION.md

### Accelerator foot prototype

The 0.4.0 throttle-test archive and 0.3.0 SPF plugin add a throttle-driven right ankle pose while retaining the validated steering arm tracks. Raw user accelerator input drives the foot, so cruise-control throttle does not move it without a pedal press. The private animation contains 33 independent throttle levels, each with the original 47 steering frames. The plugin resets to the selected throttle block and advances within it using the existing steering mapping. This avoids coupling ankle movement to steering or mutating shared stock animation resources.

Only right-foot bone 57 is rotated, up to 12 degrees downward. Its toe descendants inherit the motion. The hinge axis is derived from the neutral ankle-to-toe vector and avatar up direction. At full gas the toes lower about 2.9 cm. Steering tracks are byte-identical at every throttle level, and all other tracks remain neutral. Offline checks validate track identity, normalized rotations, valid PMA pointers and downward toe motion. C++ tests validate clip guards, throttle bounds, release/half/full gas, independent steering ranges and original argument/return forwarding. The original seated animation regression also passes.

The user confirmed the first-person throttle foot test works. Runtime logs show nonzero throttle overrides and no caught faults; see live_throttle_validation.json. It uses the same executable hash guard and private-model identity as steering. Stale telemetry releases the foot and centers steering. The previous 0.3.0 archive and 0.2.0 DLL are backed up in work/pre-throttle-backup. To revert, close ATS, restore the prior archive using the installed filename ATS-VisibleDriverBody-Mustang-0.1.0.scs, and restore the prior DLL using SPF_DriverAnimationDiagnostics.dll in its existing SPF plugin folder.

Test parked in first-person VR: release accelerator, press halfway, press fully, release; then steer with accelerator held. Confirm the foot presses downward and returns on release, while hands still steer/reposition and the seated leg remains stable. Native logs report THROTTLE override counts, current pedal value and selected starting frame. Foot direction, pedal alignment and amplitude require visual confirmation.


<a id="tifa-comparison"></a>

## TIFA_COMPARISON.md

### 0.2.2 runtime investigation

Latest ATS log confirms version 0.2.1 mounted and spfstock.ford.mustang_67.carpet selected. It has no stock body model/animation load error; user reports no body. Absence of errors does not prove actor instantiation.

Working installed Tifa passenger reference: Steam Workshop item 3810525140, universal/unit/hookup/vehicle/tifa_seat.sii. Mustang definition uses accessory_addon_int_data; animated_hookup references a skinned model and PMA. Its compiled wrapper has zero pieces/materials, two selectable parts/locators and empty-geometry bounds (FLT_MAX / -FLT_MAX). Our wrapper had a tiny triangle and tiny finite bounds at the carpet floor. This mismatch provides a culling hypothesis, not a confirmed engine trace.

Version 0.2.2 replaces our wrapper with an independently written, zero-piece/material, locator-only PMG v21 / PMD v4 with matching empty-geometry structure. It retains our driver placement transform and one visible part/locator. This removes the placeholder geometry/culling variable. Stock 61-bone model, head suppression, neutral animation clip, private paths, hookup registration, interior accessory class and procedural fallback are unchanged. No Tifa meshes, animation or textures were copied.

Minimal writer: make_locator_only_mount.py, called by build_stock_variant.py. Structural checks validate offsets, byte lengths, locator membership, variant visibility and dependency paths. Runtime rendering still needs confirmation: restart ATS and reselect the stock body option after selecting original carpet. If it remains invisible, the carpet-category actor instantiation path versus the known toyseat path remains to be traced; direct interior-rig integration is an alternative rather than proof that this attempt works.


<a id="turn-signals"></a>

## TURN_SIGNALS.md

### Turn Signal Animations

Version 0.12.1 adds left-hand gestures for both turn directions using SPF truck telemetry's selected lblinker/rblinker states. It follows ATS bindings rather than watching separate keys. Flashing lamp states do not replay the gesture; hazards and ambiguous both-side states suppress it. A loaded active signal establishes a baseline without replay. Switching directly between directions triggers the new direction. Cancellation, including automatic cancellation, does not trigger a separate gesture.

The hand passes through a slightly open approach waypoint, closes into the user-approved index/middle/thumb pinch at the stalk, moves down for left or up for right, then returns directly to the current steering grip. The final reach is 2 inches forward, 2 inches down and 1 inch left of the neutral stock wrist. Actuation stroke is 2.5 cm. The waypoint opens the fingers 25 percent toward the bind pose. Right-arm and pedal animations continue independently; global elbow clearance remains active. Horn interrupts the stalk gesture. Pending requests older than two seconds are discarded; stale telemetry returns the hand.

#### Install and live editing

With ATS closed, install `releases/SPF_DriverAnimationDiagnostics-0.12.1-signal-waypoint-test.dll` under the existing plugin DLL name. Copy `signal_left_pose.txt` and `signal_right_pose.txt` into its config folder. Preserve all other pose and elbow files and the tested body archive.

The version-2 pose file's three quaternion groups are approach waypoint, final reach and actuation. Both files reload twice a second and retain the last valid pose after invalid edits. Geometry and finger changes are live; sequence or trigger logic changes require a plugin update.

#### Validation

Build, geometry and regression tests pass. Checks cover arm lengths, unchanged right arm/pedals, full steering-range elbow compatibility, no repeat while selected, hazards, world reset, staged actuation and stale data. The latest runtime recorded both directions, 17 selection events, 1,048 pose updates and zero caught pose faults. Installed files match the release copies byte-for-byte.

The user approved the fingertip pose and requested this commit after the waypoint test. Final waypoint-specific visual feedback was not explicitly recorded; that pose remains editable live. Alignment is calibrated to the Mustang, with other vehicles unvalidated.

The 0.15.0 synchronization prototype delays normal toggle bindings until stalk actuation; see [TURN_SIGNAL_DELAY.md](#turn-signal-delay). Momentary lane-change bindings retain their native timing. The user approved the synchronized delay and final live calibration.


<a id="turn-signal-delay"></a>

## TURN_SIGNAL_DELAY.md

### Synchronized turn signals

Version 0.15.0 delays normal player turn-signal toggles while the left hand approaches the stalk. A guarded input-handler hook holds the request and later injects one edge at the same site. The original game block performs signal selection, blink-phase setup, click event and automatic-cancellation setup. The plugin does not write signal-state fields directly. Both directions passed the user test.

Repeated pending toggles cancel; the opposite direction replaces the pending request. Hazards, vehicle/world changes and an externally changed selection discard it. Already-selected signals turn off immediately. If the animation cannot acknowledge arrival, a two-second fallback releases the valid request. Momentary/lane-change bindings remain native and are not delayed; automatic cancellation is not intercepted.

The first test exposed duplicate animations because replay suppression was consumed by an old telemetry sample. The corrected guard waits for matching selection acknowledgement, bounded to half a second. Regression tests cover old samples, matching/opposite selections, hazards and expiry.

#### Live settings

`config/signal_activation_delay.txt` contains seconds after the hand animation begins, from 0 to 0.45. The approved value is `0.22`. Smaller values activate earlier. Waiting for another left-arm gesture does not consume this delay.

`config/signal_actuation_seconds.txt` contains the duration of the up/down stalk movement, from 0.05 to 0.60. The approved value is `0.26`. The reach and return retain their 0.22-second durations, and the hold remains 0.12 seconds.

Both files reload twice per second outside the animation hook. Missing, invalid, incomplete or oversized edits retain the last accepted values. No restart is needed for subsequent edits. Game sound and lamp latency still need visual/audio calibration when changing the activation value.

#### Pose calibration and validation

The approved live poses move the grab and actuation targets half an inch lower and half an inch left relative to the previous committed poses. A trial half-inch forward move was undone. Hand orientation, finger shape and the approach waypoint are retained. Install the release pose files: the compiled fallback retains the previous calibration.

Build and regression tests pass for request queueing, single replay, cancellation, opposite-direction replacement, timeout, hazard/vehicle/world changes, stale-data pass-through, mid-function register forwarding, replay acknowledgement, live-setting validation and independent stalk speed. The final runtime health snapshot reports 37 releases with zero timeout fallbacks and zero caught pose faults. Cancellation counters also include resets; this does not establish coverage of every cancellation path. Installed release and live files match the packaged artifacts. The user approved the final state for commit.


<a id="window-gesture"></a>

## WINDOW_GESTURE.md

### Left-window middle-finger gesture

Installed prototype: SPF_DriverAnimationDiagnostics 0.8.0.

Hold F10 to release the left steering grip, reach toward the left window, curl the other fingers while extending the middle finger, then hold the pose. Releasing first opens the fingers, then returns the arm to the current steering grip. The right hand keeps steering and the pedals remain independent.

In SPF's keybind settings for SPF_DriverAnimationDiagnostics, reassign Gestures.LeftWindowBird to the desired key or button. F10 is the initial default and is consumed by this action. The action polls physical hold/release state, regardless of the configured logical toggle behavior. The horn still reads ATS's game horn state and has priority; the window gesture returns before the horn reaches, and resumes afterward if its key remains held.

The builder uses two-joint IK to author a wrist target at (-1.02, 1.02, 0.17) meters in the driver model's coordinates, preserving arm lengths. The back of the hand faces the left side and the middle finger points upward. The runtime uses compact pose blending at the previously validated private skeleton hook. No animation archive expansion is required.

Pose checks cover arm lengths, normalized quaternions, curled versus extended fingers, and unchanged torso/right arm/pedals. Native tests cover hold/release, complete gesture formation, horn priority, resume behavior, no competing arm blends and byte-exact released poses. All pass.

The target is an initial geometric estimate. Visual window reach, door/glass clearance, elbow position and finger appearance are pending a VR test. Use an open left window for the first test. ATS was left closed after installation; launch it when ready to test. No commit or push has been made for this prototype.

#### Wheel-position revision

The user confirmed the first gesture was pretty good, then requested moving it in front of the steering wheel and slightly above it, with the thumb wrapped around the outside of the curled fingers. Version 0.8.1 sets the wrist target to the neutral wrist midpoint plus 10 cm up and 8 cm toward the driver. The back of the hand faces the driver and the middle finger remains upward. Two-joint thumb IK positions the thumb across the outer curled-finger knuckles rather than inside the palm. The existing SPF action name and F10 default remain unchanged to preserve bindings. All checks pass; the revised position and thumb require visual confirmation. Installed with ATS closed and launch requested.

Version 0.8.3 responds to VR feedback confirming hand placement but requesting the palm face the driver and less thumb intersection. The wrist flips around the vertical finger direction without moving its target. The full-thumb IK wrap is replaced by small base/middle curls (10 and 15 degrees) and a larger distal curl (65 degrees). This aims to use the tip rather than laying the whole thumb across the fingers. Pose and native tests pass, but thumb mesh clearance still requires visual confirmation. Installed with ATS closed; launch requested.

Version 0.8.4 moves the wrist 8 cm forward and 4 cm up relative to the previous pose, tilts the palm down 45 degrees, and increases the non-middle finger curl. The gesture now forms during the reach (0.30-second formation alongside 0.45-second reach), instead of afterward. A farther initial target failed the preserved-arm-length check, so the final target stays within natural reach. All pose and native checks pass; revised alignment and curl still require visual confirmation. Installed with ATS closed and launch requested.

Version 0.8.5 reverses the palm tilt upward following the user correction, moves the wrist another 8 cm forward and 4 cm up, and uses clavicle rotation to extend naturally without stretching arm bones. Gesture formation remains concurrent with the reach. Pose and native regression checks pass. Installed after ATS closed; launch requested. Visual alignment is pending.

Version 0.8.6 preserves the user-confirmed hand target and changes orientation through forearm axial roll rather than an extra wrist rotation. The wrist uses its stock local quaternion. The middle finger base joint aims the extended finger vertically; the other finger curls increase to 95/115/85 degrees. Assertions verify the unchanged wrist angle, vertical finger direction, exact hand target and preserved bone lengths. All checks pass. Installed after ATS closed and launch requested; appearance still needs visual validation.

The user confirmed the current 0.8.6 position/finger pose looks good and requested committing it as a checkpoint, while identifying excessive forearm roll that twists the elbow mesh. This known visual issue remains in the checkpoint. The next prototype will distribute the same total hand orientation evenly between forearm and wrist.

Version 0.8.7 halves the forearm axial roll and puts the remaining half into the wrist, preserving the previous full-gesture hand world orientation. The total rotation is about 98.66 degrees, split about 49.33 degrees each. Hand position, vertical raised finger, thumb pose, formation timing and horn priority are preserved. Offline tests pass. This follow-up is installed and uncommitted. ATS launch requested; the elbow mesh visual check is pending.

The user reported the 50/50 split looks pretty good and requested a little more upward wrist tilt, slightly less finger curl, and the thumb turning toward the driver. Version 0.8.8 adds a 10-degree upward wrist tilt after the same forearm split, reduces finger curls to 90/105/75 degrees, and turns the thumb base modestly toward the driver while keeping the distal curl. The confirmed wrist position and vertical raised finger are preserved. Checks pass. Installed with ATS closed and launch requested; this refinement is uncommitted and visually unvalidated.

Checkpoint: the user confirmed the refined 0.8.8 pose looks good. Recorded and committed before the next fingertip-curl and lower thumb-wrap adjustment.

Version 0.8.9 further eases curled-finger tips (finger curls 90/100/55 degrees) and aims the thumb base toward the folded index finger middle segment. Gesture transitions run at 1.5x speed: reach 0.30 seconds, formation 0.20 seconds, opening 0.08 seconds and withdrawal about 0.233 seconds. Horn timing is unchanged. Checks pass. Installed with ATS closed and launch requested. The previously approved 0.8.8 checkpoint is committed; these changes are uncommitted and await visual validation.

Version 0.8.10 responds to the user screenshot showing thumb/finger intersection. A NumPy-only bounded joint fitter now adjusts all thumb joints, using simplified finger capsule distances to avoid passing through folded fingers and targeting contact near the folded index middle segment. Its final sampled proxy clearance is approximately 21.5 mm. This is not a rendered mesh collision guarantee; the final and transitional thumb clearance require a VR check. The forearm/wrist split, hand position, raised finger and 1.5x gesture speed remain unchanged. Builder and native checks pass. Installed after ATS closed and launch requested; visual clipping check pending.

The user confirmed the 0.8.10 thumb looks good, but reported curled fingertips intersecting the palm. Version 0.8.11 preserves the exact serialized thumb rotations and eases the folded finger joints from 90/100/55 to 90/90/35 degrees. The hand position, raised finger, rotation split and faster timing remain unchanged. Pose and native checks pass. Installed after ATS exited and launch requested; actual mesh clearance must still be checked visually.

Version 0.8.12 follows the user screenshot: the index finger is further eased to 80/75/20-degree curls, the whole hand turns 8 degrees left to reduce its rightward lean, and the middle finger base compensates to preserve a vertical raised finger. Confirmed thumb joint rotations remain unchanged relative to the hand. The user accepts slight index/thumb overlap. The hand target, other fingers and faster timing are preserved. Pose and native tests pass; installed after ATS exited and launch requested. This refinement is uncommitted and awaits visual confirmation.

Final checkpoint: the user confirmed the 0.9.2 direct-return version works great. The latest plugin, three-keyframe live pose file, generated targets, source and validation records are saved together for commit. Earlier release files remain as diagnostic checkpoints.

### Native driver pedal restoration

Restored accelerator, staged brake transfer, and independent clutch animation after the accessory-free body transition. Pedal rotation deltas are extracted from the previously validated clip and applied only to the native driver's leg bones, retaining the active vehicle's resting pose. No accessory is required. All seven native suites pass, including neutral-pose preservation, independent gas/clutch movement, brake transfer, and isolation from other bones. The user confirmed the installed restoration works in VR.

### Foot setup and live controller control

Added wrist-style LEFT FOOT and RIGHT FOOT setup boxes, with per-vehicle resting offsets, backward-compatible box profiles, and Undo/Redo. Outside Setup, a targeted trigger press engages the corresponding foot with relative controller position and rotation; double-trigger releases it. Release blends back to the saved resting position and current pedal pose over 0.35 seconds. Tracking loss, Setup entry, and vehicle changes clear live foot ownership. All seven native suites pass, covering migration, persistence, leg-only IK, calibration, rotation, and the return transition. The user confirmed the installed controls and smooth return work in VR.

### Soft pointer-target highlights

Outside Setup, the nearest box hit by either cabin laser receives a soft cyan volume highlight. Three crossed stacks sample a feathered density field with fading boundaries; no outline or cube faces are shown. Duplicate targets draw once, unavailable joints and stale tracking hide highlights, and Setup and menus suppress them. The effect highlights the box volume rather than clipping to vehicle geometry. The renderer uses transparency without depth writes and preserves the game's graphics state. All seven native suites pass, including stereo hover output and density falloff. The user confirmed the installed soft highlight looks good in VR.

### Setup highlight controls and smoother volumes

Setup Help now starts hidden, Setup radial menus use a distinct purple palette, and the right Setup radial offers highlight visibility at the top, color cycling at the right, and opacity cycling at the bottom. Preferences save globally across vehicles and restarts in config/highlights.ini. Highlight volumes now use 33 layers per axis and 16-bit floating-point density textures; the renderer prefers floating-point swapchains when supported and retains the earlier fallback. All seven native suites pass, including preference persistence, malformed-edit recovery, radial visibility, density falloff, and stereo floating-point texture rendering. The user validated the installed changes in VR and requested commit and push.
