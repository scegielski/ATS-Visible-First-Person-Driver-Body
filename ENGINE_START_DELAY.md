# Engine Start Synchronization Prototype

Version 0.14.0 holds eligible player engine-start requests until the ignition hand reaches the key. The original request routine was first observed unchanged: two normal starts arrived with start=1, immediate=0 and prior engine state off, then the game entered starting; two stops arrived with start=0, immediate=0 and prior state running, then the game entered stopping.

A queued request starts the existing ignition reach while the engine stays off. After the pose is applied successfully at full reach with runtime IK, the animation publishes its matching request sequence. SPF's update callback replays the original engine-control routine once. The subsequent starting-state transition does not trigger a duplicate hand animation.

The queue requires the guarded player vehicle, engine off, electricity enabled, fresh controls and a recently observed private body. It is restricted to identified player input caller sites. Unknown caller paths, immediate/forced requests, and non-player requests pass to the game unchanged. Automatic-start path exclusion still needs runtime testing.

A stop request or electricity being turned off cancels the queued start and withdraws the hand. Vehicle changes, world reload, invalid/stale data or another engine state discard the queue. If the animation cannot report arrival while the vehicle remains valid and powered, a two-second timeout releases the start rather than blocking it indefinitely. Repeated eligible requests do not restart the delay.

Build and regression checks pass for queue/release once, duplicate requests, stop cancellation, vehicle/world changes, stale data, timeout, manual caller guards, untouched argument forwarding and suppressed-call branching. The replay uses the observed three arguments; static callers do not consume its return. Previous body/head, signal and elbow features remain unchanged.

Runtime synchronization passed the user test: three requests were released at key arrival, each entering the starting state, with no timeout fallback. The observed manual caller was the guarded toggle routine. The user approved the result and requested this commit. Quick cancellation and automatic-start behavior have not been observed in game; their intended handling is covered by regression tests and caller restrictions. The installed plugin is backed up before replacement; the body archive and calibrated files are preserved.
