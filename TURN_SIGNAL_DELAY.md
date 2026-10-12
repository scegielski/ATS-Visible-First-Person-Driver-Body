# Synchronized turn signals

Version 0.15.0 delays normal player turn-signal toggles while the left hand approaches the stalk. A guarded input-handler hook holds the request and later injects one edge at the same site. The original game block performs signal selection, blink-phase setup, click event and automatic-cancellation setup. The plugin does not write signal-state fields directly. Both directions passed the user test.

Repeated pending toggles cancel; the opposite direction replaces the pending request. Hazards, vehicle/world changes and an externally changed selection discard it. Already-selected signals turn off immediately. If the animation cannot acknowledge arrival, a two-second fallback releases the valid request. Momentary/lane-change bindings remain native and are not delayed; automatic cancellation is not intercepted.

The first test exposed duplicate animations because replay suppression was consumed by an old telemetry sample. The corrected guard waits for matching selection acknowledgement, bounded to half a second. Regression tests cover old samples, matching/opposite selections, hazards and expiry.

## Live settings

`config/signal_activation_delay.txt` contains seconds after the hand animation begins, from 0 to 0.45. The approved value is `0.22`. Smaller values activate earlier. Waiting for another left-arm gesture does not consume this delay.

`config/signal_actuation_seconds.txt` contains the duration of the up/down stalk movement, from 0.05 to 0.60. The approved value is `0.26`. The reach and return retain their 0.22-second durations, and the hold remains 0.12 seconds.

Both files reload twice per second outside the animation hook. Missing, invalid, incomplete or oversized edits retain the last accepted values. No restart is needed for subsequent edits. Game sound and lamp latency still need visual/audio calibration when changing the activation value.

## Pose calibration and validation

The approved live poses move the grab and actuation targets half an inch lower and half an inch left relative to the previous committed poses. A trial half-inch forward move was undone. Hand orientation, finger shape and the approach waypoint are retained. Install the release pose files: the compiled fallback retains the previous calibration.

Build and regression tests pass for request queueing, single replay, cancellation, opposite-direction replacement, timeout, hazard/vehicle/world changes, stale-data pass-through, mid-function register forwarding, replay acknowledgement, live-setting validation and independent stalk speed. The final runtime health snapshot reports 37 releases with zero timeout fallbacks and zero caught pose faults. Cancellation counters also include resets; this does not establish coverage of every cancellation path. Installed release and live files match the packaged artifacts. The user approved the final state for commit.
