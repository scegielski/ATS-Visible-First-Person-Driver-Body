# Independent Live Elbow Clearance

Version 0.11.5 is installed and visually validated. Per-frame two-joint IK applies inward elbow-guide offsets across steering, horn, gestures and ignition, preserving each hand's current position and orientation.

- `left_elbow_inset.txt`: 0.06 metres, positive values guide the elbow right/inward.
- `right_elbow_inset.txt`: 0.04 metres, positive values guide the elbow left/inward.

Each file contains one decimal value in metres, accepts 0 to 0.15, and reloads twice a second. Zero disables that side's correction. Invalid edits keep the last valid value. A guide offset is not a guaranteed equal elbow displacement: the result depends on the shoulder-to-hand geometry and available bend. File reads stay outside the skeleton hook.

Runtime checks require fresh controls telemetry, unit scales, finite positions, reachable targets, and wrist error below one millimetre. Failed solves leave the pose unchanged. Tests cover stock steering frames and both hands' gestures/ignition. The latest validated session recorded 5,427 updates per elbow, zero rejections, and zero caught pose faults. The user confirmed the final appearance.
