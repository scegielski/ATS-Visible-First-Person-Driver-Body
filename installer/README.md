# Compact Windows installer

The uniquely versioned setup executable under `releases/installer/`
packages the integrated 0.39.0 body/controller plugin, OpenXR bridge/manifest,
OpenVR helper, tested body archive, embedded general defaults and Mustang configurations, sounds and licenses.
SteamVR controller models are loaded at runtime, not bundled.

Close ATS, then run Setup. Steam may remain open. SPF Framework, the supported ATS
executable and an already working OpenXR VR launch setup are prerequisites. Setup
does not read or modify Steam launch options. A new profile still needs the body
mod enabled and its accessory selected; save games are not rewritten.

## Installed layout

- The main plugin's dedicated SPF folder contains DLLs, licenses, sounds and config.
- General tracking, timing and controller defaults are embedded in the DLL.
- `config/mustang_poses.ini` holds live hand/finger/gesture poses, and
  `config/mustang_boxes.ini` holds live box positions, rotations and sizes.
  SPF owns `config/settings.json`. No `driver_body.ini` is installed.
- All WAVs live under `sounds/`, without legacy audio migration or stored WAV backups.
- The body archive uses the selected ATS user-data mod folder and stable filename.
- Only a missing external OpenXR loader is installed beside amtrucks.exe.
- The system-wide OpenXR layer entry and external-loader cvar enable normal Steam
  launching. The bridge activates controller/rendering features only for ATS.

## Small uninstall footprint

There is no separate setup-manager directory or ProgramData state tree. Package
files extract to Inno Setup's temporary directory, which is removed at exit.
A `.setup/` directory inside the plugin contains the PowerShell uninstall helper,
`install-record.json`, and Inno's uninstaller and log. `backups/` exists only when
original non-audio files are actually replaced or consolidated. Existing WAVs
are never backed up. Identical files are not
backed up, and repeat setup does not create transaction history. Recovery copies
used to undo a failed installation are temporary.

Re-run Setup to update or repair missing/corrupt files. There is no persistent
rollback browser or separate Repair application. Uninstall reverses the layer and
cvar changes, removes newly installed files where unchanged, restores replaced
originals where safe, and retains calibration and later user edits. If user edits
prevent restoration, corresponding originals and their record are retained.

## Integrated adjustments

Mirror, seat and steering-wheel adjustment code from
https://github.com/scegielski/SPF_AdjustmentHotkeys is compiled into the main DLL.
The main runtime calls it directly, sharing SPF lifecycle, config, keybinds and
hook ownership. Hash/signature guards, input leases, native seat UI-tail guards,
pause/focus checks and servo output are retained. The native source and imported
regression fixtures are in `native-diagnostics/adjustments`.

The installer assumes the standalone adjustment plugin does not exist. It does
not detect, back up, disable, remove, or migrate that separate plugin.

## Live settings and preservation

Setup migrates existing vehicle sections from the retired main INI before removing it. Existing Mustang files take precedence and remain mutable across updates and uninstall. General values come from the compiled defaults. The native reader retains the last structurally valid live pose document, with per-pose validation. Setup saves box placement atomically and rejects concurrent external edits.

## Build and validation

Use official Inno Setup 6.7.3 and Python:

```powershell
cmd /c work\build_driver_diag.cmd
powershell.exe -NoProfile -ExecutionPolicy Bypass -File installer\Test-Setup.ps1
python installer\build_installer.py --iscc path\to\ISCC.exe
```

Six native suites pass. Isolated installer fixtures cover install/uninstall,
reinstall repair, replaced-files-only backups, temporary failure rollback,
calibration consolidation and no sound backups, existing INI precedence, existing-loader preservation,
malformed/tampered input rejection, and no changes to Steam or the standalone
adjustment plugin. The builder verifies an explicit SHA-256 payload manifest.

The unsigned installer was run against the live game. Follow-up fixes enabled the saved SPF plugin state and avoid shutting down the shared SteamVR client on plugin unload. The latest split configuration and integrated adjustments still need a complete live VR regression check.
Normal Steam loading via system-wide registration was validated separately on the
prior build; compatibility with other OpenXR games has not been established.

Each build automatically advances the installer patch version. Pass `--version X.Y.Z` to start a newer release series. Existing or older versions are rejected; the filename, Windows product version, installed package version and payload receipt use the same version. Keep previously distributed installers when building releases.

After a successful installer build, the builder updates the main README download label and direct executable URL to the same version automatically, including explicit `--version` releases. Missing or duplicate README download links fail validation before compilation. Failed builds leave the README unchanged. Run the link-update tests with `python -m unittest discover -s installer -p "test_build_installer.py"`.
