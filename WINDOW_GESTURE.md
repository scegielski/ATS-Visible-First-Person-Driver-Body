# Left-window middle-finger gesture

Installed prototype: SPF_DriverAnimationDiagnostics 0.8.0.

Hold F10 to release the left steering grip, reach toward the left window, curl the other fingers while extending the middle finger, then hold the pose. Releasing first opens the fingers, then returns the arm to the current steering grip. The right hand keeps steering and the pedals remain independent.

In SPF's keybind settings for SPF_DriverAnimationDiagnostics, reassign Gestures.LeftWindowBird to the desired key or button. F10 is the initial default and is consumed by this action. The action polls physical hold/release state, regardless of the configured logical toggle behavior. The horn still reads ATS's game horn state and has priority; the window gesture returns before the horn reaches, and resumes afterward if its key remains held.

The builder uses two-joint IK to author a wrist target at (-1.02, 1.02, 0.17) meters in the driver model's coordinates, preserving arm lengths. The back of the hand faces the left side and the middle finger points upward. The runtime uses compact pose blending at the previously validated private skeleton hook. No animation archive expansion is required.

Pose checks cover arm lengths, normalized quaternions, curled versus extended fingers, and unchanged torso/right arm/pedals. Native tests cover hold/release, complete gesture formation, horn priority, resume behavior, no competing arm blends and byte-exact released poses. All pass.

The target is an initial geometric estimate. Visual window reach, door/glass clearance, elbow position and finger appearance are pending a VR test. Use an open left window for the first test. ATS was left closed after installation; launch it when ready to test. No commit or push has been made for this prototype.

## Wheel-position revision

The user confirmed the first gesture was pretty good, then requested moving it in front of the steering wheel and slightly above it, with the thumb wrapped around the outside of the curled fingers. Version 0.8.1 sets the wrist target to the neutral wrist midpoint plus 10 cm up and 8 cm toward the driver. The back of the hand faces the driver and the middle finger remains upward. Two-joint thumb IK positions the thumb across the outer curled-finger knuckles rather than inside the palm. The existing SPF action name and F10 default remain unchanged to preserve bindings. All checks pass; the revised position and thumb require visual confirmation. Installed with ATS closed and launch requested.

Version 0.8.3 responds to VR feedback confirming hand placement but requesting the palm face the driver and less thumb intersection. The wrist flips around the vertical finger direction without moving its target. The full-thumb IK wrap is replaced by small base/middle curls (10 and 15 degrees) and a larger distal curl (65 degrees). This aims to use the tip rather than laying the whole thumb across the fingers. Pose and native tests pass, but thumb mesh clearance still requires visual confirmation. Installed with ATS closed; launch requested.

Version 0.8.4 moves the wrist 8 cm forward and 4 cm up relative to the previous pose, tilts the palm down 45 degrees, and increases the non-middle finger curl. The gesture now forms during the reach (0.30-second formation alongside 0.45-second reach), instead of afterward. A farther initial target failed the preserved-arm-length check, so the final target stays within natural reach. All pose and native checks pass; revised alignment and curl still require visual confirmation. Installed with ATS closed and launch requested.

Version 0.8.5 reverses the palm tilt upward following the user correction, moves the wrist another 8 cm forward and 4 cm up, and uses clavicle rotation to extend naturally without stretching arm bones. Gesture formation remains concurrent with the reach. Pose and native regression checks pass. Installed after ATS closed; launch requested. Visual alignment is pending.

Version 0.8.6 preserves the user-confirmed hand target and changes orientation through forearm axial roll rather than an extra wrist rotation. The wrist uses its stock local quaternion. The middle finger base joint aims the extended finger vertically; the other finger curls increase to 95/115/85 degrees. Assertions verify the unchanged wrist angle, vertical finger direction, exact hand target and preserved bone lengths. All checks pass. Installed after ATS closed and launch requested; appearance still needs visual validation.

The user confirmed the current 0.8.6 position/finger pose looks good and requested committing it as a checkpoint, while identifying excessive forearm roll that twists the elbow mesh. This known visual issue remains in the checkpoint. The next prototype will distribute the same total hand orientation evenly between forearm and wrist.
