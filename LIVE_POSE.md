# Runtime gesture pose editing

The installed 0.9.0 plugin watches config/gesture_pose.txt in its own SPF_DriverAnimationDiagnostics directory. Save the file while ATS runs: validated targets become active within about half a second, including while F10 is held. One game restart is needed to install this plugin; subsequent pose edits do not need rebuilding, restarting or toggling the plugin.

The file begins with SPF_GESTURE_POSE 1. Each bone row has its index, then four open/reach quaternion values and four held-gesture quaternion values, in w x y z order. Comment lines identify bone names. All 21 left-arm and finger rows must remain present. Current F10 controls, formation timing, horn priority and pedal animation are unchanged.

Only these left-arm bones are allowed: shoulder 4, upper arm 5, forearm 6, hand 7, finger joints 8–22 and twist bones 23–24. Missing/duplicate bones, extra values, invalid numbers, zero/non-unit quaternions and files larger than 64 KiB are rejected. Small normalization drift is corrected. Failed or partial edits retain the last valid pose. If no valid file has loaded, the compiled pose remains active. Removing the file retains the last valid loaded pose.

File reading and parsing occur outside the animation hook. A validated immutable snapshot is published atomically so the hook sees a complete pose. Log messages beginning POSE_RELOAD identify accepted generations or rejected edits.

build_window_gesture.py exports releases/gesture_pose.txt as well as the fallback C++ targets. To apply future generated geometry changes without rebuilding the plugin, copy the generated pose file to the watched installed config file. Manual quaternion edits can be saved directly. This first implementation live-reloads the middle-finger gesture, not the horn or pedals.

Parser and snapshot tests cover valid publication, existing-snapshot immutability, partial/duplicate/unknown bone rejection, invalid quaternions, size limits, normalization, comments and the live gesture blend. Runtime watcher validation passed: initial load and a comment-only save were accepted while ATS stayed running. The first live geometry edit then added 5 degrees at each curled index joint; visual feedback remains pending.

The user confirmed the live index curl update works. A subsequent runtime file update turns the wrist palm modestly toward the driver while preserving the hand target and compensating the middle finger to remain vertical. Pose checks pass; visual orientation feedback is pending.

## Three-keyframe pose file

Version 0.9.1 adds a distinct intermediate pose. The current file header is SPF_GESTURE_POSE 2. Each bone row now contains three quaternion groups in w x y z order: reach/open, intermediate finger-down fist, and final held gesture. All three groups can be edited live. Version-1 two-pose files remain readable for compatibility.

During reach the middle finger stays down while the other fingers form the fist. Only after the hand reaches its target does it transition from intermediate to the final raised middle finger. On release the finger folds down first, then the fingers open and the arm withdraws. The existing faster reach timing, hand target, current joint refinements and horn priority are retained. Tests cover the intermediate arrival state, no early extension, incomplete version-2 rejection and all existing regressions. Installed with ATS closed and launch requested; visual sequence validation remains pending.

The user confirmed the arrival sequence looks good but observed an unwanted open-hand stage on release. Version 0.9.2 changes release to begin arm return immediately, fading the current gesture into the current steering grip as it moves. The intermediate finger-down pose remains on entry. Tests verify immediate return without the extra exit pose. This changes compiled timing logic, so installation needs one restart; pose-file editing remains live afterward. Built, tested and installed after ATS exited. The existing live pose file was preserved and launch requested; release visual validation is pending.

## Final confirmation

The user confirmed the final finger-down entry, held pose and direct return work great. The latest runtime logged 1405 gesture pose updates with 0 caught pose-write faults. The installed pose file matches the committed release file. Runtime reloads and live joint edits were confirmed during the earlier tuning session.
