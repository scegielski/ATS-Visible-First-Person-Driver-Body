SPF Visible First Person Driver Body - Setup

Close ATS before setup or uninstall. Steam can remain open. SPF Framework and a
working ATS OpenXR VR launch configuration are prerequisites. Steam launch options
are never read or changed. Only the tested ATS executable is supported.

The main plugin includes mirror, seat and wheel adjustment code. Setup assumes no
standalone adjustment plugin is installed; it never examines, migrates, backs up,
disables, or removes that separate plugin. SPF, SteamVR and the Mustang vehicle
mod are not included.

Plugin DLLs, licenses and sounds use the dedicated SPF plugin directory. General settings are compiled into the DLL. Live poses use config/mustang_poses.ini
and box placements use config/mustang_boxes.ini; SPF owns config/settings.json.
Vehicle sections migrate from the retired driver_body.ini before it is removed.
Sounds are installed directly into sounds/; old sound layouts are not migrated
and existing WAV files are not backed up.

The body archive uses your ATS mod directory. The external OpenXR loader DLL goes
beside amtrucks.exe only if missing. Setup adds the system-wide OpenXR registration
and enables the external-loader cvar. Other games can load the small layer DLL,
but its controller features activate only for ATS.

Payload extraction and failure-recovery copies are temporary and removed after
setup. There is no separate Program Files manager folder, ProgramData state folder,
or permanent payload copy. A small .setup directory inside the plugin holds the
uninstaller, its helper and install-record.json. Its optional backups directory
contains only originals actually replaced or consolidated. Reinstalling does not
accumulate transaction history. Re-run this installer to update or repair files.

Uninstall restores replaced files where unchanged, reverses its registration and
cvar changes, and retains calibration and subsequent user edits. Backups that could
not be restored without overwriting later edits remain available beside the record.

On a new profile, enable the body archive in ATS Mod Manager and select its body
accessory in the Mustang. Calibrate head tracking using the game's recenter action
then point at or touch a hand grab box and press trigger to engage that hand.
Double-press trigger to release. Setup mode edits the wrist-attached grab boxes. The desktop shortcut uses the
normal Steam game launch. SteamVR and the headset must be ready.

The installer is unsigned. Automated native and isolated installer tests pass;
the latest split configuration and integrated adjustments need a complete VR check.

