# Turn Signal Animations

Version 0.12.1 adds left-hand gestures for both turn directions using SPF truck telemetry's selected lblinker/rblinker states. It follows ATS bindings rather than watching separate keys. Flashing lamp states do not replay the gesture; hazards and ambiguous both-side states suppress it. A loaded active signal establishes a baseline without replay. Switching directly between directions triggers the new direction. Cancellation, including automatic cancellation, does not trigger a separate gesture.

The hand passes through a slightly open approach waypoint, closes into the user-approved index/middle/thumb pinch at the stalk, moves down for left or up for right, then returns directly to the current steering grip. The final reach is 2 inches forward, 2 inches down and 1 inch left of the neutral stock wrist. Actuation stroke is 2.5 cm. The waypoint opens the fingers 25 percent toward the bind pose. Right-arm and pedal animations continue independently; global elbow clearance remains active. Horn interrupts the stalk gesture. Pending requests older than two seconds are discarded; stale telemetry returns the hand.

## Install and live editing

With ATS closed, install `releases/SPF_DriverAnimationDiagnostics-0.12.1-signal-waypoint-test.dll` under the existing plugin DLL name. Copy `signal_left_pose.txt` and `signal_right_pose.txt` into its config folder. Preserve all other pose and elbow files and the tested body archive.

The version-2 pose file's three quaternion groups are approach waypoint, final reach and actuation. Both files reload twice a second and retain the last valid pose after invalid edits. Geometry and finger changes are live; sequence or trigger logic changes require a plugin update.

## Validation

Build, geometry and regression tests pass. Checks cover arm lengths, unchanged right arm/pedals, full steering-range elbow compatibility, no repeat while selected, hazards, world reset, staged actuation and stale data. The latest runtime recorded both directions, 17 selection events, 1,048 pose updates and zero caught pose faults. Installed files match the release copies byte-for-byte.

The user approved the fingertip pose and requested this commit after the waypoint test. Final waypoint-specific visual feedback was not explicitly recorded; that pose remains editable live. Alignment is calibrated to the Mustang, with other vehicles unvalidated.

The 0.15.0 synchronization prototype delays normal toggle bindings until stalk actuation; see [TURN_SIGNAL_DELAY.md](TURN_SIGNAL_DELAY.md). Momentary lane-change bindings retain their native timing. The user approved the synchronized delay and final live calibration.
