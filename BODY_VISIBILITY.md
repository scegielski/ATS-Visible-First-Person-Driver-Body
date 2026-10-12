# Camera-Conditioned Driver Visibility

Plugin 0.13.2 is installed and visually validated with the 0.7.0 body archive. SPF supplies the active camera type; type 2 is the current interior/VR camera. Unknown camera state leaves the exterior stock driver visible and hides the private head as a first-person fallback.

## Live modes

`body_visibility.txt` accepts:

- `replace-exterior` (current): our animated body remains visible; outside the interior camera, the player's stock driver is hidden after its exact controller/model identity is observed.
- `interior`: our body is hidden in recognized non-interior cameras, leaving the stock exterior driver visible.

The setting reloads twice a second; invalid edits keep the prior mode. In either mode, the private head/neck is hidden in the interior view and restored outside through normal palette regeneration.

## Identity and render guards

The same guarded player-owner/vehicle chain used by the game horn and engine queries nominates the driver controller at vehicle+0x1df0 and its model at +0x1df8. The player updater's static caller uses this controller; traffic uses +0x2c8. Hiding requires CaptureDriver to observe the exact nominated controller/model pair with stock model identity and a fresh timestamp. A shared asset name alone is insufficient. No shared resource or global traffic model is modified.

The skeleton post hook verifies executable fingerprint, 61-bone count, private/player instance identity, output palette pointer and finite matrices. It collapses only the selected instance's rendered bone palette to a finite point after the original hierarchy assembly. Animation inputs remain intact, and the original matrices regenerate on the next call. Write faults stop further visibility writes.

Regression tests cover caller arguments, stack alignment through normal calls, integer/floating return preservation, post-callback ordering, finite matrix collapse, head-only bone changes, wrong owner and traffic/different-model rejection. The user confirmed both visibility directions and the final exterior-head version. Runtime recorded 12,312 player-controller matches, 445 stock-hide frames, 3,326 interior head-hide frames and zero visibility write faults. Installed DLL, archive and mode file match their release copies byte-for-byte.

This is a guarded render-palette implementation, not a public engine visibility flag. It preserves the engine's bounds; general rigs, other vehicles and separate mirror-camera behavior are not validated.
