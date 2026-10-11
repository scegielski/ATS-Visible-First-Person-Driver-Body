# Driver animation diagnostics

## Experimental throttle version: 0.3.0

Paired with the 0.4.0 throttle-test archive, this version also selects a right-ankle pose from raw accelerator input, independent of steering. The expanded clip has 1551 frames (33 throttle blocks of 47 steering frames); the clip guard recognizes its 64.583333-second duration. Missing/stale pedal input releases the ankle. The verified 47-frame steering clip remains supported. See THROTTLE_INTEGRATION.md for validation, testing and rollback details. The user confirmed the first VR foot test works; runtime override logs report no caught faults. See live_throttle_validation.json.

## Current version: 0.2.0 live steering test

The sections below describe the earlier observation-only versions. Version 0.2.0 adds a targeted pose override for `/model/spf_stock_driver/driver.pmd` only when its clip has 47 frames and a duration of 1.916667 seconds. Use it with the 0.3.0 steering-test archive. It resets/enables that animation and replaces advancement with the stock steering-derived position. Missing/stale telemetry selects neutral. The executable hash/signature guards remain. Tests cover both unchanged forwarding and float replacement while preserving remaining arguments and returns. First-person visual steering validation is still pending.

To roll back, close ATS and restore both files from `work/pre-steering-backup`: the previous diagnostic DLL and the 0.2.4 body archive, using their original installed filenames. Disabling the plugin alone leaves the experimental arm clip playing without steering control.

This observation-only SPF plugin records the stock exterior driver's animation advancement together with steering telemetry. It does not animate the interior body, replace the body mod, or change any animation arguments.

## Build and evidence

Build with MSVC x64 and CMake from native-diagnostics. Headers come from the adjacent SPF-Framework-1.2.5 checkout. Two game entry signatures are unique in the fingerprinted installed executable. Hooks refuse registration if its SHA-256 or expected entry locations differ. Assembly observers preserve integer/floating-point arguments, flags and stack arguments, tail-call the original trampoline, and have Windows x64 unwind metadata. Regression tests exercise forwarding and return values, sample bounds, invalid values and reset behavior.

Static inspection identifies the driver update at RVA 0x9e1c40. It resets/enables the actor animation, obtains its duration and advances it with a steering-derived value at a call returning to RVA 0x9e1d92. The advancement entry at RVA 0x15deed0 takes the model in RCX, animation index in EDX and advancement in XMM2. Its return ABI is deliberately not guessed: the observer tail-jumps to the original. This entry advances animation state; it is not yet a validated arbitrary absolute-pose setter.

Runtime evidence is pending. Test success means forwarding works in the synthetic harness, not that the game hooks or player actor are runtime-validated.

## In-game validation

The installed plugin is SPF_DriverAnimationDiagnostics under the game's bin/win_x64/plugins/spfPlugins folder. Restart ATS to load a newly installed plugin. Keep the existing working body selected. Park the Mustang, hold center, sweep left, return to center, sweep right, then return to center. Allow several seconds at each position. Check the exterior driver's hands and switch to first-person VR to check normal body behavior. No hand movement is expected from the interior actor in this diagnostic.

Plugin logs report hook registration, actual installation, driver-update counts, matched advancement counts and SAMPLE records. Each sample has controller/model identifiers, clip index, duration, requested advancement, normalized fraction, current user/effective steering and telemetry/sample ages. The snapshots are asynchronous: ages must be checked when interpreting correlation. Several exterior actors may be present; the player actor must be identified by repeated steering correlation. Requests are only observed from the verified driver's call site, not from unrelated model-animation calls.

Success requires actual installed-hook messages, nonzero matching calls, a repeatedly correlated player actor, stable center behavior, and unchanged normal driving across view changes and a vehicle/world reload. Missing records or mismatch logs are diagnostic failures, not evidence of a working bridge. No internal actor pointers are dereferenced later from the log ledger.

## Removal

Remove SPF_DriverAnimationDiagnostics.dll from its own plugin folder while ATS is closed if the DLL is locked. This leaves the body mod and other plugins intact.

The first runtime sweep was repeated after SPF activation. Both hooks installed and 1,214 driver advancement calls were captured with no rejections. The interior actor is still unchanged. Version 0.1.1 adds MODEL_SAMPLE identity records for the stock human driver and the private spf_stock_driver model; readable identity paths still require runtime confirmation.
