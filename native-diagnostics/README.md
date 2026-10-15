# Native plugin development

The current plugin is `SPF_VisibleFirstPersonDriverBody`. User instructions are in the [controls guide](../docs/USAGE.md); superseded diagnostics and hook investigations are in [development history](../docs/DEVELOPMENT_HISTORY.md).

## Build

Use Windows x64, MSVC with MASM, CMake and Ninja. SPF API headers are expected in the adjacent `../../SPF-Framework-1.2.5/include/SPF/SPF_API` checkout relative to this directory. Configure and build this CMake project, then run CTest with output on failure.

The local `work/build_driver_diag.cmd` helper automates the configured workstation paths but is not distributed. The six CTest targets cover driver animation, OpenXR action forwarding, early-menu hook ownership, stereo controller rendering, integrated adjustments and settings handling. Build the installer separately using the [installer instructions](../installer/README.md#build-and-validation).

## Runtime structure

- `DriverAnimationDiagnostics.cpp`: SPF lifecycle, guarded native hooks, animation, head tracking and menu surface capture.
- `ControllerRuntime.cpp`: tracked hands, calibration, fingers, cabin targeting, controller model poses and menu mouse control.
- `ControllerLayer.cpp`: early OpenXR API layer, action registration and frame forwarding.
- `ControllerModelsD3D11.cpp`: stereo controller mesh rendering.
- `adjustments/`: integrated native mirror, seat and steering-wheel controls and feedback.
- `DriverDefaults.hpp`: compiled general settings.
- `DriverSettings.hpp`: live Mustang INI loading and atomic box saves, with compatibility reads for older layouts.

## Lifecycle and compatibility

The early bridge and SPF share one loaded main DLL. Menu hooks can start before SPF; activation hands polling to SPF. Plugin unload must not shut down the shared SteamVR client while ATS still submits frames. The connection remains alive for process lifetime.

Game hooks are guarded by executable hash and verified signatures. After an ATS update, audit the call chain and supporting globals before enabling new signatures. Synthetic passing tests establish only their modeled behavior; capture runtime hook activation and perform a VR check before claiming game compatibility.

SteamVR models and game reference assets are loaded from their installations, not redistributed as controller assets. Imported code licenses are under `adjustments/licenses` and `vendor/minhook`.
