# ATS Visible First Person Driver Body

The latest built archive is **0.6.1 pedal alignment test**, paired with the **0.5.0 SPF steering/pedals plugin**. Steering, right-foot gas/brake motion and left-foot clutch animation were confirmed working in VR with the prior 0.6.0 archive. The final alignment reduces brake reach to 14 cm, keeps the left resting leg 6 cm left, and has passed offline checks; it has not been installed or visually tested, per the user's request to commit without relaunching. Version 0.2.4 remains the validated static seated baseline.

The selectable **SPF Stock Driver Body (animated prototype)** carpet accessory uses the game's male driver avatar, retaining its 61-bone skeleton. Head-related triangles are hidden while hands remain. The steering release reuses the original Mustang shoulder/arm/hand tracks while freezing torso and leg tracks at the neutral seated pose. The SPF plugin applies the stock steering mapping to the private interior actor. Raw accelerator input selects a downward right-ankle pose independently of steering. Braking lifts the right ankle, moves the leg left, hovers and presses, then returns to gas on release. Clutch input independently presses the left ankle at its shifted resting position. Stock materials and textures are referenced from the installed game rather than bundled. A separate procedural body remains selectable as fallback. Diagnostic A/B entries are retained in this checkpoint.

## Install

With ATS closed, copy releases/ATS-VisibleDriverBody-Mustang-0.6.1-pedal-alignment-test.scs into the ATS user mod directory. Install releases/SPF_DriverAnimationDiagnostics-0.5.0-clutch-test.dll, renamed SPF_DriverAnimationDiagnostics.dll, in bin/win_x64/plugins/spfPlugins/SPF_DriverAnimationDiagnostics/ under the game installation and enable it in SPF�s Plugins panel. Both files are required for steering and all pedal animations. Existing installation uses the older filename ATS-VisibleDriverBody-Mustang-0.1.0.scs to preserve profile references; replace that file rather than enabling both archives. Enable the mod, load the Mustang and select SPF Stock Driver Body (animated prototype) under carpet accessories. Restore the original carpet to remove it.

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

The installed and user-validated **0.9.2 direct return test** plugin adds a held middle-finger gesture above the steering wheel. Default F10 can be reassigned through SPF's existing Gestures.LeftWindowBird action. The hand reaches with the middle finger down, raises it after arrival, and returns directly to the current steering grip on release. Gesture transitions run 1.5 times faster than the first prototype. The game-state horn takes priority, while steering and pedals remain independent.

The current pose shares axial rotation between forearm and wrist, keeps a natural bend in the raised finger, and uses the user-confirmed thumb pose. The user confirmed the final sequence works great. See WINDOW_GESTURE.md and the validation records.

The plugin watches config/gesture_pose.txt in its own directory and hot-reloads valid saves within about half a second. The version-2 file contains reach, intermediate finger-down and final held quaternions for 21 left-arm/finger bones. Invalid or incomplete edits retain the last valid pose. No rebuild or game restart is needed for pose edits. Timing logic changes still require a plugin update. See LIVE_POSE.md for format, scope and validation.

Install releases/SPF_DriverAnimationDiagnostics-0.9.2-direct-return-test.dll as SPF_DriverAnimationDiagnostics.dll in the existing SPF plugin directory with ATS closed, and copy releases/gesture_pose.txt into its config directory. Preserve customized pose files when upgrading. The current body archive remains unchanged.
