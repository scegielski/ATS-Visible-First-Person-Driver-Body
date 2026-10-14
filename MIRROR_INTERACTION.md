# Controller mirror adjustment prototype

Three setup boxes select left, center, and right mirrors. Either enabled controller hand or disabled-hand controller axis may overlap one; that hand's thumbstick adjusts the mirror horizontally and vertically, with a 0.2 deadzone. Leave the box or release the stick to stop. Within a mirror box the stick does not toggle finger assignment. When both hands overlap boxes, the larger stick deflection wins (left on ties). No mirror actions run during setup.

F9 exposes all five labeled boxes. Move/rotate with one grip; resize one local dimension with two grips. Release to save. Version 3 of interaction_boxes.txt includes all five boxes and accepts older two-box files without losing the ignition/horn calibration. Mirror default locations are initial Mustang estimates and need VR alignment using setup mode. Boxes remain invisible in normal play.

## Native adjustment dependency

The installed SPF_MirrorControls plugin from scegielski/SPF_AdjustmentHotkeys owns the native hooks. A small exported controller-input lease reuses its existing selector, direction branches, native setter, limits, and refresh. The driver plugin registers no competing mirror hooks. The mirror plugin must remain enabled. Requests expire after 150 ms, and the native plugin's pause/focus/version guards remain in force. Left/center/right map to physical slots 0/4/2, as verified for the Mustang in the reference plugin; other vehicle mappings remain unverified.

The matching reference-repository patch is mirror-controller-bridge.patch; copy native-diagnostics/MirrorControllerBridge.hpp to that repository before applying/building the patch. Both modified source files are also in the adjacent local SPF_AdjustmentHotkeys checkout. The staged releases/SPF_MirrorControls-controller-bridge.dll supplies the required export. The OpenXR input bridge also needs the paired update to expose both stick axes.

Validation: driver and controller-layer regressions cover box migration/round trip, mirror overlap/slot routing, and both stick axes. Reference mirror regressions cover actual detour selection/direction scope, one-call forwarding, neutral/released and expired requests. Physical mirror-box placement and adjustment direction still need an in-game check.

When hand tracking is disabled, a controller-centered collision box operates mirrors and horn directly, with right-controller trigger operating ignition. No hand calibration is required for this mode. Setup displays these boxes around the axes; collision wireframes remain hidden outside setup. Enabled hands retain their calibrated palm boxes. Tracking loss and stale scene frames invalidate both modes.

Controller mirror adjustment uses the reference plugin servo.wav loop and its servo_sound setting. The same short input lease selects stereo routing and movement: left/center/right routing, silence in the stick deadzone, and stop on contact loss, setup, pause, or expired input.

Axis mode now emits a two-metre solid scene laser along each controller's local negative Z axis. The nearest rotated/scaled interaction box wins, and the beam ends at that box. Mirror stick adjustment, horn targeting, and right-trigger ignition use the ray hit; enabled hand mode keeps palm contact. Setup displays beams but disables actions. These rays test the interaction boxes only, without world-geometry raycasting. Ray tests cover nearest target, backward/parallel misses, inside origin, rotated and nonuniform boxes. Visual pointing direction needs VR validation.

Horn activation now requires the targeting controller trigger above 0.7, in both palm-contact and laser modes. Either controller may operate it. Releasing the trigger or losing the target stops that controller contribution; keyboard/wheel horn forwarding remains unchanged.

## Seat box
A sixth labeled SEAT box starts between the legs at a Mustang estimate and is movable, rotatable and scalable in F9 setup. Both hand contact and axis-mode laser targeting route the controller stick to native seat mode 6 in Adjustment Hotkeys. Stick up/down adjusts height; left/right adjusts forward/back using that plugin's established mapping, native vehicle defaults/limits and apply routine. Its existing centered servo plays during adjustment. Version 4 of the saved box format accepts the older two- and five-box formats while preserving the new seat default. The horn-trigger requirement is included in this update. Seat placement, direction and in-game camera behavior require a VR check.
