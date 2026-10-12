# Driver Body Animation Roadmap

Ordered from most feasible to most uncertain, based on the current validated Mustang implementation. This is a planning list, not a commitment to begin implementation.

1. **Vehicle-specific animation calibrations, where needed.** Add profiles for different cars and trucks to align steering grips, pedals, horn targets, and gestures with their cabins. Existing editable poses provide a starting point; automatic profile selection and support for different rigs still need investigation.
2. **Ignition animation — implemented and visually validated.** When the engine-start button is pressed, the driver releases the wheel with the right hand, drops the hand down and forward to the ignition, turns the key, and returns to the current steering grip. Reuse the arm pose system, identify a reliable game engine-start trigger, and calibrate the key position for each supported vehicle.
3. **Head-driven seated torso movement.** Use calibrated VR head movement to estimate restrained upper-body leaning and turning, keeping the hips seated. Arm IK should preserve wheel or controller hand targets. Head tracking access and runtime torso control need validation.
4. **Motion-controller hands.** Read tracked controller poses, calibrate VR space to the cabin, and solve arm IK each frame. Keep physical-wheel driving animations available.
5. **Toggle between controller tracking and driving animations.** Provide a configurable SPF action with a smooth return to the current steering grips, an optional instant snap, and a tracking-loss fallback. Depends on controller-hand support.
6. **Exit and re-enter the parked vehicle.** Coordinate a full-body exit and entry sequence, doorway clearance, body visibility, and camera movement. The vehicle stays parked throughout.
7. **Walk around the parked vehicle in VR.** Investigate whether the service/vehicle-browser walk camera can be used in the main game world and in VR. Movement, collision, body placement, and return to the seat remain unverified.

## Feasibility notes

The order is provisional. Access to VR tracking, runtime skeleton control, and the walk camera could change it. A visual exit animation and a freely controllable walking mode are separate milestones.

