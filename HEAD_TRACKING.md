# Live VR head and torso tracking

Validated release: 0.16.4. The user confirmed torso tracking inside and outside and exterior head animation. The final runtime snapshot recorded more than ten thousand torso updates and more than seven thousand exterior head updates with no rejected solves; the last hand-target error was below a millimetre. See head_tracking_validation.json.

## Tracked pose and calibration

A guarded read-only observer runs after the game's local-space OpenXR xrLocateViews call. It averages the two eye positions and hemisphere-normalizes orientation, preserving tracking-space identity and predicted display time. The view-relative query is excluded because its pose describes eye offsets rather than room movement. Successful result, stereo count, structure layout, finite values, quaternion norms and eye separation are checked. Torso application additionally requires valid and tracked position/orientation flags, fresh controls and recent private-body observation.

A seated baseline is captured in the interior camera. Press F8 once while seated neutrally to calibrate (SPF action HeadTracking.Recenter, remappable). Switching to an exterior view retains calibration. On world/tracking-space change, exterior mode waits for interior calibration or explicit recenter. Unknown cameras disable application. After ATS's own VR recenter, recalibrate with F8; following the native action is the next requested change.

## Body and head behavior

Head translation drives a restrained three-spine-joint lean, and one fifth of head yaw drives a torso turn capped at ten degrees before gain. Inputs smooth over approximately a tenth of a second. Hips and legs remain seated. Analytic arm IK preserves the current wrist targets and orientations, including steering re-grabs and active horn, gestures, signals or ignition. Unreachable combinations reduce lean transactionally instead of stretching the arms. Calibrated elbow guides still apply afterward.

In exterior views, calibrated headset rotation drives neck and head joints, with a world-space target compensating torso lean rather than applying its rotation twice. Correction is distributed 65 percent to neck, with the residual at the head. Total head rotation is bounded to 85 degrees. First-person head/neck hiding and the guarded player exterior replacement remain intact. Lost tracking fades toward neutral. This approximates torso motion from head tracking rather than reconstructing full-body pose.

## Live configuration

Copy both files from releases to the plugin config directory. They reload twice a second outside the animation hook. Missing, invalid, oversized or incomplete edits retain the last valid values.

`head_tracking_gain.txt` accepts one number from 0 to 1; the approved value is `1.0`. Zero fades motion to neutral.

`head_tracking_limits.txt` contains these ordered entries:

```
forward_m 0.30
max_lean_degrees 28
lateral_m 0.24
```

Forward range accepts 0.05–0.40 metres, lean angle 5–35 degrees and symmetric lateral range 0.05–0.35 metres. Older two-line files default lateral range to 0.24 metres. Backward input remains capped at 0.08 metres and vertical input at ±0.08 metres before gain. The limits constrain input rather than guarantee equal physical torso displacement: hips stay fixed and spine motion distributes the lean.

## Validation and references

Regression tests cover observer frame/argument/result forwarding, stereo midpoint, quaternion signs, validity/separation checks, all steering frames, forward/back/side lean, hand position and orientation preservation, horn/gestures/signals/ignition, untouched hips/legs/fingers, calibration, exterior baseline retention, recenter, space changes, live-setting parsing and exterior yaw/pitch/roll without double torso rotation. The installed DLL and head configuration files match the release artifacts. The body archive and existing live arm poses are retained.

Structure layouts follow the [Khronos OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK/blob/main/include/openxr/openxr.h); the existing game query is described in the [xrLocateViews reference](https://registry.khronos.org/OpenXR/specs/1.1/man/html/xrLocateViews.html). The eye midpoint is a head-centre proxy, not an anatomical neck joint.
