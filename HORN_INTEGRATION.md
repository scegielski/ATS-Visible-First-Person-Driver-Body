# Left-hand horn prototype

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

## Final validation

The user confirmed the final 0.6.6 pose is good in VR and authorized committing it. The corrected hand clearance, open palm and wrist orientation are visually validated. The latest runtime log recorded 2092 horn overrides and 0 caught horn faults. Keyboard H is the validated input; wheel-button support remains pending.
