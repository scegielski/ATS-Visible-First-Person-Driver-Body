# SPF Visible First Person Driver Body

A visible, animated driver body and VR controller interactions for American Truck Simulator, calibrated for the Mustang.

The plugin combines steering and pedal animations, head and torso tracking, controller hand IK, animated Quest controller models, menu laser pointing, and editable cabin controls. Mirror, seat and steering-wheel adjustment code is integrated; a separate adjustment plugin is not required.

## Install

Use the [Windows installer (0.39.59)](https://raw.githubusercontent.com/scegielski/ATS-Visible-First-Person-Driver-Body/main/releases/installer/SPF-Visible-First-Person-Driver-Body-Setup-0.39.59.exe) with ATS closed. Steam can remain open. SPF Framework and a working ATS OpenXR VR launch configuration are prerequisites. Setup leaves Steam launch options unchanged.

The current body archive is [ATS-VisibleDriverBody-Mustang.scs](releases/ATS-VisibleDriverBody-Mustang.scs); versioned test archives are historical.

See the [installer guide](installer/README.md) for dependencies, file locations, update and uninstall behavior. On a new profile, enable the included body archive in ATS Mod Manager and select its body accessory in the Mustang. Launch through Steam normally after installation.

## Use

The [controls and configuration guide](docs/USAGE.md) covers hand calibration, finger controls, menu pointers and the controller cabin-box editor.

General driver settings are compiled into the DLL. Mustang poses and box placements remain editable in two INI files. SPF manages its own settings and keybindings separately.

## Development and validation

- [Build and architecture](native-diagnostics/README.md)
- [Installer sources and tests](installer/README.md#build-and-validation)
- [Current validation record](installer/validation.json)
- [Historical development notes](docs/DEVELOPMENT_HISTORY.md)

Six native regression suites and installer lifecycle tests pass. The user confirmed the split configuration works in VR. The latest change removes the temporary menu axes fallback; its startup appearance still needs a visual check. Tests do not replace in-game validation.

Native hooks support the fingerprinted ATS executable only. A game update may require a signature audit. The Mustang is the supported vehicle calibration; automatic vehicle-profile selection is not implemented. If persistent stereo lag or tearing occurs, fully restarting SteamVR was necessary during earlier testing.

## Project scope

See the [future-work plan](docs/FUTURE_WORK.md) for controller Driving Mode, safe input transitions, vehicle profiles selected by game identity, essential controls, and a headset-only acceptance walkthrough.

Future work includes additional vehicle calibrations and investigating parked-vehicle exit and walking. Those features are not implemented. Historical experimental DLLs remain under `releases/`; asset-building scripts and checkpoint JSON reports are grouped under `development/scripts/` and `development/evidence/`. These are development resources; use the current installer for installation.
