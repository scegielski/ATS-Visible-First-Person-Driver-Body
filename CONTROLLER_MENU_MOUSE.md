# Controller menu pointing checkpoint

Menus show controller axes, straight forward beams, and hit dots. Point either controller at the captured VR menu surface without holding grip. A fresh trigger press selects that controller for cursor ownership; holding trigger drags and the thumbstick scrolls. Ownership stays stable through brief misses. Cabin interactions pause during menu control.

The ray uses the final native menu transform with its per-eye translation removed, supports flat and curved panels, and is expressed relative to the headset. Beam and intersection use the same raw controller forward axis. Quest tip pitch compensation is disabled at the user's request.

Cursor movement currently uses smoothed relative mouse input. Direct Windows cursor positioning was tested and did not move ATS's menu cursor. Relative gain starts at 1.5 on each axis. The installed config file `controller_menu_cursor_gain.txt` contains horizontal and vertical multipliers, for example `1.5 1.5`; valid edits reload every half second, invalid values preserve the last valid pair. Accepted values are finite, positive, and at most 10. This is a tuning workaround; direct internal ATS cursor positioning remains unimplemented.

The grip-and-rotate air mouse remains a fallback when the native menu surface is unavailable. Version-specific native hooks retain their executable and signature guards. Controller input reads do not add OpenXR synchronization calls.

Validation: both regression suites pass, including ray/surface geometry, eye-offset removal, pointer ownership, stationary movement, independent gain, reverse travel, and invalid edits. The user reports the relative-pointer version is almost working, but exact cursor/laser alignment remains unresolved. No claim of pixel-exact positioning or complete click coverage is made. Plugin 0.34.8 is installed with matching release binary.

## Native absolute cursor (0.35.1)

Supported ATS builds now use the native input cursor and absolute-position event queue. The guarded path checks the executable signature, input object vtable, window, and viewport before writing position and submitting the event. Relative input remains a fallback. Native positioning uses the game's actual viewport rather than desktop dimensions and bypasses relative gain.

The curved panel follows the native mesh generator and renderer: angle is `2*(u-.5)/radius`, X is scaled by half width, Y by half height, and Z by full width. Its cross section is elliptical. Tests reconstruct rendered points across horizontal/vertical coordinates and several radii, then recover their UVs. The user confirmed alignment looks good in VR.

Startup limitation: the observed launch began at 09:21:48; SPF preload finished at 09:22:19, SDK initialization began at 09:23:59, and our first menu surface appeared at 09:24:01. Native pointer updates followed within one second. Early menu startup is a separate follow-up.

## Early menu startup (0.36.2)

The existing in-process OpenXR bridge loads the adjacent driver DLL after successful instance creation, outside its input mutex. The DLL validates the executable hash and menu/cursor instruction bytes, then installs its two menu observation hooks using vendored MinHook 1.3.4. The bridge retains the DLL reference while SPF probes plugin manifests.

Before SPF activation, a background thread handles controller poses, overlays, and menu input. The menu render callback only publishes the surface and consumes queued native cursor coordinates. Surface publication and cursor consumption do not wait on the worker. Cursor commands expire after 100ms. The sync callback only records focus, and input reads never add an OpenXR sync. SPF activation reuses the early-owned hooks and takes over polling; the worker exits. Explicit plugin unload stops early polling.

The launcher prefers the installed bridge directory over development copies. Normal launch enables controller input. The diagnostic `-DisableControllerBridge` switch removes the explicit layer request and path and sets the manifest disable flag for that child process only.

The user confirmed early pointers and clicks work and left-eye lag/tearing are absent after a full SteamVR restart with the latest paired binaries. Earlier comparisons without restarting SteamVR were inconclusive; one attempted bridge-disabled run still loaded the bridge because an explicit layer request remained. A corrected diagnostic run was verified bridge-free. The final enabled run was verified to load both DLLs from the installed directory. This establishes the tested working state, not a definitive cause for the previous stereo issue. If stereo timing remains disturbed across ATS restarts, fully exit and restart SteamVR before comparing builds.

The early startup log is `logs/controller_menu_early.log`. In the final test, early host readiness, surface capture, and native cursor updates occurred before SPF plugin activation. All three regression suites pass, covering native geometry, cursor command freshness/consumption, bridge sync and focus behavior, and menu hook installation/restoration. The unsupported-host guard was also checked. Cabin behavior after loading a save still needs separate coverage for this startup revision.
