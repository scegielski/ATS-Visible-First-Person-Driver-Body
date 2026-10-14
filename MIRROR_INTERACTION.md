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

## Window boxes
Two additional labeled L WINDOW and R WINDOW targets begin near the doors and support the same F9 setup placement, rotation and scaling. Target with enabled hand contact or disabled-hand laser; stick up requests close, down requests open, and neutral releases input. The game retains its own electrical gating, motion, limits and sound. A native entry detour forwards once and changes only open/close flags for exact player window-state identities; manual input takes priority. No other vehicle or window-state object is changed. Input is disabled in setup, pause, focus loss and stale tracking. Version 5 saves eight targets and accepts older saved two/five/six-target files. Initial positions and runtime behavior need in-game validation.

## Steering-wheel box
A ninth WHEEL target supports hand contact and laser pointing. Stick left/right retracts/extends the column and up/down increases/decreases its tilt using the reference plugin's existing native modes 7 and 8. A combined leased mode 9 applies both axes independently, preserving the other axis and enforcing native defaults and limits. Centered servo feedback follows stick movement. F9 placement, rotation, nonuniform resize, and automatic saving are shared. Version 6 saves nine targets and reads all prior two/five/six/eight-target files. Initial wheel-box placement and column behavior need an in-game check.

## Lights box
The tenth LIGHTS target starts twelve centimetres left of the saved ignition box until placed in setup. Either enabled-hand contact or axis laser targeting permits one native light-cycle event per fresh trigger press; held triggers do not repeat. The guarded player input-site hook uses ATS's ordinary light-cycle branch and preserves its keyboard/wheel input. Setup, pause, focus loss and stale data suppress controller cycles. Version 7 saves ten targets and reads prior formats. Trigger and actual assembly forwarding tests pass; the native state cycle still needs in-game validation.

## Wipers box
The eleventh WIPERS target starts ten centimetres above LIGHTS until authored in setup. Either hand contact or controller laser plus a fresh trigger press sends one event through the game's normal wipers-cycle branch. The game chooses its supported speeds and handles motion/sound. Native keyboard input is preserved; holding trigger does not repeat. Version 8 saves eleven targets and accepts earlier formats. Assembly forwarding and target persistence regressions pass; runtime cycle still requires a visual check.

Controller wipers now explicitly wrap from highest supported speed to off, using the native vehicle-dependent state count and setter. Keyboard increase/decrease behavior remains untouched. A simultaneous native key press takes priority to avoid double advancement. The state-count and setter addresses are gated by unique signatures on the inspected executable.

## Radio box
A twelfth RADIO target starts near the center dashboard and shares setup editing, hand contact and laser targeting. Left/right stick nudges request previous/next through the native player source selection; neutral rearms the next nudge. Held up/down changes radio volume with the native clamp and frame timing. Current player source and station availability follow ATS (music mode can select tracks). Native keybinds are preserved, and setup/focus/pause/stale tracking stop controller input. Version 9 saves twelve targets and migrates older placements. Regressions cover stick edges, held volume, losing target, serialization and the actual observation stub; radio operation and positioning need in-game validation.

Controller station tuning now selects the next/previous entry of the full native internet-radio list, wrapping at both ends, rather than invoking the favorites-filtered hotkey helper. The exact internet-radio vtable and selector signature guard access, with count/current bounds validated before the native selector call. Selection logs show before/requested/after indices and count. Favorites and stream URLs remain unchanged. Volume behavior is retained. Native selector execution still needs an in-game check.

An original synthesized short radio-tuning sound plays once after a native station selection is accepted (verified index), providing immediate feedback during stream buffering. Playback is queued to the ordinary plugin update, uses the existing controller audio output, and does not depend on the internet stream. No continuous playback while the stick is held.

Radio trigger: a fresh press while touching/pointing at RADIO toggles the native pause bit on the existing stream, preserving the selected station. If no stream exists, it starts the selected station (first entry if none selected). Trigger hold does not repeat; setup, tracking validity, pause and focus guards apply. The native pause routine has its own unique signature check. Volume/station-stick controls remain unchanged. Runtime play/pause needs a visual/audio check.

Radio first-use requests load the native station list before selecting a station or starting playback. Accepted trigger play/pause actions queue a short click (`controller_radio_click.wav`) outside the animation hook. The click build is packaged; installation and its audible check are pending.

Ignition accepts independent fresh trigger presses from either hand or pointer. Radio trigger feedback uses `controller_radio_on.wav` for play/resume and `controller_radio_off.wav` for pause, based on native playback state. The directional-feedback build has passed regressions but is not installed yet.
