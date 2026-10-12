# Ignition Animation and Runtime Arm IK

Version 0.11.5 is built, installed, and visually validated by the user. The engine start signal follows ATS's own vehicle state and therefore its configured bindings. The read-only query uses the same guarded player-vehicle owner as the horn; observed transitions were off, starting, running, stopping, and off. Automatic starts can also trigger the animation. Loading an already-running vehicle does not trigger it.

The right hand follows an intermediate reach waypoint, reaches the calibrated key target, turns 70 degrees in the key-on direction, then returns to the current steering grip. Runtime wrist translation holds the thumb's distal bone as the pivot throughout the turn. The editable ignition pose file reloads twice a second, retaining the last valid pose after an invalid edit.

During ignition, runtime two-joint arm IK compensates for a forward torso lean. The left hand keeps its current steering or gesture target and orientation. The right key target gains another 15.24 cm forward reach. The torso starts at 25 degrees and increases if required, up to 46 degrees. Hips and legs stay seated; fingers retain the current animation. Failed solves leave the prior pose intact. A five-second bound prevents a stuck start state from holding the gesture indefinitely. Ignition takes priority over the right-hand gesture, which returns first.

## Install

With ATS closed, install `releases/SPF_DriverAnimationDiagnostics-0.11.5-both-elbows-test.dll` under the existing plugin DLL name. Copy `ignition_pose.txt`, `left_elbow_inset.txt`, and `right_elbow_inset.txt` into its config folder. Preserve existing left/right gesture pose files and the tested body archive. The executable fingerprint and private actor identity guards remain in place.

## Validation

Regression tests cover the complete 47-frame stock steering animation, staged ignition reach, turn and direct return, unit-scale checks, invalid input, unreachable targets and unchanged unrelated bones. Both wrist position/orientation and thumb-pivot error are checked across the turn. Tested hand/pivot position error is below one millimetre. The latest runtime session recorded two ignition starts, 308 torso-IK updates, zero IK rejections, and zero caught pose faults. The user reported the final version looks good.

The installed plugin and three new live files match the release copies byte-for-byte. Ignition alignment is calibrated for the Mustang. Other vehicles and rigs are not yet validated. Startup load delays were reported; logs showed IK inactive while loading, so their cause has not been isolated.
