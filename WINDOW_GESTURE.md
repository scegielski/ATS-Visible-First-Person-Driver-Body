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

Version 0.8.7 halves the forearm axial roll and puts the remaining half into the wrist, preserving the previous full-gesture hand world orientation. The total rotation is about 98.66 degrees, split about 49.33 degrees each. Hand position, vertical raised finger, thumb pose, formation timing and horn priority are preserved. Offline tests pass. This follow-up is installed and uncommitted. ATS launch requested; the elbow mesh visual check is pending.

The user reported the 50/50 split looks pretty good and requested a little more upward wrist tilt, slightly less finger curl, and the thumb turning toward the driver. Version 0.8.8 adds a 10-degree upward wrist tilt after the same forearm split, reduces finger curls to 90/105/75 degrees, and turns the thumb base modestly toward the driver while keeping the distal curl. The confirmed wrist position and vertical raised finger are preserved. Checks pass. Installed with ATS closed and launch requested; this refinement is uncommitted and visually unvalidated.

Checkpoint: the user confirmed the refined 0.8.8 pose looks good. Recorded and committed before the next fingertip-curl and lower thumb-wrap adjustment.

Version 0.8.9 further eases curled-finger tips (finger curls 90/100/55 degrees) and aims the thumb base toward the folded index finger middle segment. Gesture transitions run at 1.5x speed: reach 0.30 seconds, formation 0.20 seconds, opening 0.08 seconds and withdrawal about 0.233 seconds. Horn timing is unchanged. Checks pass. Installed with ATS closed and launch requested. The previously approved 0.8.8 checkpoint is committed; these changes are uncommitted and await visual validation.

Version 0.8.10 responds to the user screenshot showing thumb/finger intersection. A NumPy-only bounded joint fitter now adjusts all thumb joints, using simplified finger capsule distances to avoid passing through folded fingers and targeting contact near the folded index middle segment. Its final sampled proxy clearance is approximately 21.5 mm. This is not a rendered mesh collision guarantee; the final and transitional thumb clearance require a VR check. The forearm/wrist split, hand position, raised finger and 1.5x gesture speed remain unchanged. Builder and native checks pass. Installed after ATS closed and launch requested; visual clipping check pending.

The user confirmed the 0.8.10 thumb looks good, but reported curled fingertips intersecting the palm. Version 0.8.11 preserves the exact serialized thumb rotations and eases the folded finger joints from 90/100/55 to 90/90/35 degrees. The hand position, raised finger, rotation split and faster timing remain unchanged. Pose and native checks pass. Installed after ATS exited and launch requested; actual mesh clearance must still be checked visually.

Version 0.8.12 follows the user screenshot: the index finger is further eased to 80/75/20-degree curls, the whole hand turns 8 degrees left to reduce its rightward lean, and the middle finger base compensates to preserve a vertical raised finger. Confirmed thumb joint rotations remain unchanged relative to the hand. The user accepts slight index/thumb overlap. The hand target, other fingers and faster timing are preserved. Pose and native tests pass; installed after ATS exited and launch requested. This refinement is uncommitted and awaits visual confirmation.

Final checkpoint: the user confirmed the 0.9.2 direct-return version works great. The latest plugin, three-keyframe live pose file, generated targets, source and validation records are saved together for commit. Earlier release files remain as diagnostic checkpoints.
