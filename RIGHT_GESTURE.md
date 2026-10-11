# Right-hand gesture prototype

The installed 0.10.0 plugin adds an independent right-hand middle-finger gesture above the wheel. Hold F11; release to return directly to the current steering grip. Change the binding in SPF under Gestures.RightHandBird. The left-hand F10 binding remains intact.

The right hand reaches with its middle finger down, raises it only after arrival, and uses the same faster transition timing as the left hand. Its live pose file is config/right_gesture_pose.txt, with the same version-2 reach/intermediate/held quaternion format. The two loaders enforce distinct bone sets and retain independent last-valid snapshots. Editing one hand's file cannot target the other hand's bones.

The builder mirrors the validated left pose through stock bind frames and retargets the right wrist with two-joint IK. It preserves arm lengths and leaves left-arm, torso and pedal tracks unchanged. The initial above-wheel wrist target matches the left gesture. The right finger-down and raised stages pass geometric checks.

The native tests cover side-specific pose-file validation, immutable last-valid state, normalized right-hand blending, preservation of left hand/torso/pedals, and independence from the left gesture and horn. All tests pass. The right gesture can run while the left horn operates; the existing left gesture/horn priority remains unchanged.

The plugin and right pose file are installed, the left file is preserved, and ATS launch was requested. The right wrist, thumb/finger appearance, and transition alignment still need a VR check. No commit or push has been made for this prototype.

## Final validation

The user confirmed the right gesture and both final offsets look good: left 3 inches left and right 3 inches right. Both installed live files match their release files. Runtime recorded 8084 left gesture updates, 7582 right gesture updates and 0 caught pose-write faults. The right target offset is specified independently of the left target for future tuning. Ship both live files with the plugin; the original compiled fallback targets are centered. The final offset files are user-validated.
