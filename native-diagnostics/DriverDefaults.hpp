#pragma once
// General driver defaults are compiled into the plugin.
inline constexpr char kDriverDefaults[]=R"SPFDEFAULTS(
# horn_left_hand_box_offset: left palm collision box, shared by all controls.
# ignition_hand_box_offset: right palm collision box, shared by all controls.
# Live driver settings and poses. Preserve section names.

[body_visibility]
replace-exterior

[controller_elbow_guides]
left -0.12 -0.35 0.12
right 0.12 -0.35 0.12

[controller_finger_mode_blend_seconds]
0.18

[controller_forearm_twist_share]
0.50

[controller_menu_cursor_gain]
1.5 1.5

[gesture_head_follow_gain]
1.0

[head_tracking_gain]
1.0

[head_tracking_limits]
forward_m 0.70
max_lean_degrees 40
lateral_m 0.60

[horn_left_hand_box_offset]
0.14 0 0

[ignition_hand_box_offset]
-0.14 -0.02 0

[interior_head_backshift]
0.00

[interior_head_mode]
visible

[left_elbow_inset]
0.06

[right_elbow_inset]
0.04

[signal_activation_delay]
0.22

[signal_actuation_seconds]
0.26

)SPFDEFAULTS";
