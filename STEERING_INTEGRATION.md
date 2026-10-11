# Verified live steering result

The user confirmed the 0.3.0 archive and 0.2.0 plugin work in first-person VR, with good alignment and natural hand repositioning on the wheel. Runtime logs show nonzero interior overrides and zero caught engine-call faults. See live_steering_validation.json. The entries below retain the discovery and pre-test history; statements about pending validation describe those earlier stages.

# Steering integration investigation — October 11, 2026

## Verified baseline

The starting checkout was clean at 9306a1f (0.2.4). The PMA header regression passes using the bundled Python runtime. The installed archive named 0.1.0 and the repository's 0.2.4 release have the same SHA-256: db8d2f9d53b0a61900bbdc466290f4b972b4769c674d476ad435861a45d5b317. Neither package assets nor the installed archive were changed during this investigation. The user's prior VR visibility/placement confirmation remains the only runtime confirmation; no new in-game steering test was performed.

Run audit_animation_integration.py for a read-only audit. steering_integration_audit.json records both skeletons, varying tracks in the original driver clip, definition inspection and archive hashes. The stock rig has 61 bones; the Mustang animated interior has 46. Original clip variation includes arms, fingers, torso and feet, so replaying all 47 frames as a loop would introduce unrelated movement. Component variation alone does not establish the purpose, steering range or neutral midpoint of those frames.

## Compatibility correction

The older 1.39 interior driver's merged rig is useful as a geometry/rig reference, but its input binding is obsolete. SCS's 1.42 migration guide says steering wheel animation was removed from accessory_interior_data and wheel_anim became obsolete:

https://modding.scssoft.com/wiki/Games/ETS2/Modding_guides/1.42

The current shared truck-interior animation reference also explicitly labels wheel_anim obsolete in 1.42:

https://modding.scssoft.com/wiki/Documentation/Engine/Truck_Interior_Animations_and_IDs

The extracted Mustang animations.sui contains no wheel_anim assignment. Its steering wheel is an interchangeable accessory. Adding driver bones to the interior and creating a wheel_anim PMA therefore does not establish a supported steering input binding on ATS 1.61. Do not ship that as live steering support.

## Integration decision

Preserve the animated seated actor as the shipping baseline. Before implementing a merged-rig build, establish a current engine-supported way to sample an arbitrary arm animation from steering input. None was established by this investigation. If testing an undocumented bone/attachment behavior, use a separate diagnostic package and require actual left/center/right runtime evidence before investing in rig merging. Attaching the whole body to the rotating wheel would rotate the torso and legs; it does not meet the desired animated seated-body behavior.

An external runtime bridge would need both input acquisition and a verified API to set the actor's animation sample. Telemetry by itself supplies no proven actor-animation control. This is a separate unresolved architecture, not an implemented fallback.

The current official interior animation table supplies no dedicated throttle/brake/clutch pedal animation entries. brake_pressure_anim is an air-brake pressure gauge signal, not a general pedal-input binding for this Mustang. Foot tracks exist in the source clip, but live pedal synchronization remains unverified and unimplemented. Revisit it after the actor control path is established.

## Next useful experiment

Identify a documented or experimentally confirmed current actor-animation input bridge. Then isolate arm tracks, reconstruct their poses relative to the actual wheel, establish the neutral frame and steering endpoints, and retain the existing hold pose for torso/legs. Only merge and remap the interior rig if that bridge requires it. A successful PMA build/conversion remains separate from runtime input synchronization.

## Native bridge investigation

The adjacent SPF-Framework-1.2.5 source registers user and effective steering telemetry in src/Telemetry/SCSTelemetryService.cpp and stores it in ControlsProcessor.cpp. This establishes a source-level input acquisition path, not runtime verification of our body integration. Its GameCameraDebugAnimation interface controls camera animation only; inspection of include/ and src/ found no actor skeleton or PMA sample control API.

The local SCS SDK telemetry initialization structure exposes event/channel callback registration, not actor animation playback control. The official SDK description likewise covers vehicle telemetry and basic input devices:
https://modding.scssoft.com/wiki/Documentation/Engine/SDK/Telemetry

The newly documented 1.61 vehicle_animation_data types are suspension and ignored_anim. That interface does not document a steering-driven arbitrary actor clip:
https://modding.scssoft.com/wiki/Documentation/Engine/Units/vehicle_animation_data

Consequently a native plugin implementing this feature still requires discovering and validating an internal actor animation sampling hook for the installed ATS build. No actor address, layout, calling convention or safe playback hook has been established. Do not substitute the camera animation API or guess memory offsets. This investigation added no DLL, package replacement or claimed steering implementation.

## Offline hook discovery — first pass

Added discover_animation_hook.py and export_animation_candidates.py. Dependencies are isolated under ignored work/discovery-deps (capstone 5.0.9, pefile 2024.8.26). The scanner reads the installed executable, fingerprints it, finds relevant ASCII anchors and candidate RIP-relative LEA/MOV references, and associates references with PE unwind ranges. It performs no process attachment, memory writes or game replacement.

First pass: 709 matching strings and 800 candidate references. Results are in work/animation_hook_candidates.json; selected disassembly is in work/animation_candidate_disassembly.txt. These include false leads such as sound/graphics drivers, so anchor relevance must be reviewed. PE unwind ranges can describe split function fragments rather than complete functions; do not use every range start as a hook entry.

Two stock driver model-path references occur at preferred-image addresses 0x1409e179f and 0x1409e1b30. Both enclosing code paths call 0x1409e1e40 after formatting the driver model path. Offline disassembly of this common routine shows calls into the model subsystem (including 0x1415d9ff0 and 0x1415e3200). This is a candidate model setup lead, not an identified animation sampler. Work files include driver_model_setup_window.txt. All addresses are build-specific preferred-image addresses and require relocation for live debugging.

Next: establish the correct actor from model setup, inspect its animation/controller references and trace the per-frame playback update. Confirm correlation against left/center/right steering while stationary and against an idle control case. Establish the full ABI and actor lifetime before registering a detour. An observation-only plugin cannot yet be built honestly because its target function ABI remains unknown.

At discovery time ATS was not running and no debugger was found on the command path. No live actor observation, steering trace, animation override or in-game validation has been performed.

## User runtime evidence — exterior driver

The user reports that the exterior Mustang driver's hands move with steering and that the exterior model overlaps the new interior actor. This confirms user-observed runtime steering behavior of the exterior actor in the current setup. It does not establish that our hookup responds to steering. The camera/view in which the overlap was observed remains to be clarified; visibility in first-person VR could make reusing the already driven actor a materially different option from creating a new playback hook.

Continued offline inspection of the shared driver model-setup routine found model subsystem calls and a constructed auxiliary object. Its type and role remain unidentified; do not label it an animation controller without further evidence. Disassembly is retained under work/driver_controller_candidates.txt (candidate filename only).

The user clarified that the overlap is visible only using an exterior camera. There is no evidence that the exterior actor is rendered in first-person VR. Reusing its steering controller would therefore still require either enabling appropriate first-person rendering with head suppression or transferring its animation sample to our independently rendered actor. Do not treat the exterior-camera observation as first-person body steering success.

The extracted Mustang data.sii explicitly sets driver_animation to /asset/character/human/driver/$driver_type$/ford_mustang_67_ani.pma. This connects the source clip used by our seated hold to the exterior driver definition. The executable also has a nearby /vehicle/driver/%s/driving.pma path reference at preferred-image address 0x1409e146b; further analysis should trace that neighboring animation setup path before selecting any detour.

## Runtime observation result

After enabling the plugin in SPF's Plugins panel (new plugins default to disabled), both observation hooks installed. The repeated user steering sweep produced 1,214 matched advancement calls, no rejected samples and no ledger overflow. One controller/model pair was observed through the full telemetry range -1 to +1. The clip duration was 1.916667 seconds, matching the original 47-frame driver clip. These are genuine runtime hook observations, distinct from compilation and the synthetic forwarding tests.

steering_runtime_validation.json summarizes the captured evidence. The raw copied sweep log is retained under work/steering_sweep_observer.log. Fresh records agree with the static piecewise mapping: negative steering corresponds approximately to its negation, positive steering approximately to 1 minus steering, and center to the start/end seam. Median discrepancy was 0.006546 normalized units; the snapshots are asynchronous, so this is evidence of correlation rather than exact same-frame error measurement. Do not use the common (steering + 1) / 2 mapping for this source clip.

No interior animation was changed. Version 0.1.1 of the diagnostic adds bounded, read-only identity observations of model instances using the model resource name chain inferred from game error-reporting code. Its purpose is to identify the private interior body by path before attempting animation control. The identity chain itself remains pending runtime confirmation. ATS had been closed when preparing this follow-up; no stale pointer from the previous process was reused.
# Live steering prototype, October 11

Runtime identity validation found separate exterior and private interior model instances passing through the same animation advancement entry. Both paths were readable, with no rejected steering samples. Evidence is saved in interior_identity_validation.json; raw logs remain under work/.

Version 0.2.0 of SPF_DriverAnimationDiagnostics now applies the exterior driver's reset-to-frame-zero / enable / advance sequence to the private interior model only. It validates the animation array bounds, 47-frame count and 1.916667-second duration. The exact executable SHA-256 and original entry signatures remain mandatory. Fresh effective steering determines playback position using the observed stock mapping. Stale or invalid telemetry selects neutral. A caught engine-call fault disables further overrides. Assembly preserves arguments and return behavior while replacing only the third floating-point argument; forwarding and replacement regression tests pass.

The 0.3.0 steering-test archive contains a private arm-only copy of the stock Mustang animation. The shoulder/arm descendants retain their original tracks; all other tracks are frozen at the existing neutral seated frame. The rig, geometry, attachment and neutral frame are unchanged. Only the archive manifest and animation reference differ from the validated 0.2.4 archive, plus the new clip. build_steering_prototype.py generates it without changing the seated-hold package sources.

The paired plugin/archive are installed with byte verification and rollback copies under work/pre-steering-backup. SPF's existing enabled state is retained. This is ready for an in-game test, but actual first-person arm steering has not yet been validated. Test parked in first-person VR: center, left, center, right, center; hold each briefly, then change views and confirm the seated torso/legs remain stable. Inspect STEERING override/fault records alongside the visual result.
