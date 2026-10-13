# Head-directed hand gesture prototype

0.17.1 lets either engaged middle-finger gesture follow calibrated headset yaw and pitch. The upper-arm rotation changes at the shoulder, moving the whole arm/hand around that pivot without changing local finger joints or segment lengths. A leftward look swings the active gesture left, and a rightward look swings it right. The response is weighted by the gesture's smooth reach blend, so release returns to the current steering pose. Looking up or down swings the gesture arm vertically; roll is excluded. Pitch is applied about the yaw-directed horizontal shoulder axis.

The shoulder adjustment is applied after the live gesture pose and before torso IK. Torso compensation therefore preserves the updated hand target, rather than holding it at the previous fixed gesture point. Existing horn/signal/ignition priority and opposite-arm steering remain. Both camera views use the same calibrated head pose. Stale head tracking uses the existing fade toward neutral.

`config/gesture_head_follow_gain.txt` reloads twice a second and accepts a finite value from 0 to 1. Default 1.0 follows the full calibrated yaw and pitch; 0 disables following. Invalid/missing edits retain the last accepted value. The head and torso strength/limits retain their current settings. Tests cover both arms through every steering frame, positive/negative yaw, shoulder-pivot position, arm lengths, unchanged other local joints/fingers, release and torso IK retaining the moving hand target. The yaw response has been visually validated.

The user validated yaw follow before requesting pitch. Combined yaw/pitch tests now cover both arms and the full steering range, untouched local finger joints and torso IK retaining the new targets. The combined build is installed and regression-tested; a separate visual confirmation of pitch was not recorded before the user requested commit and push.

