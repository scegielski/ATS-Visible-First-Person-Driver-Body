# Controller menu pointing checkpoint

Menus show controller axes, straight forward beams, and hit dots. Point either controller at the captured VR menu surface without holding grip. A fresh trigger press selects that controller for cursor ownership; holding trigger drags and the thumbstick scrolls. Ownership stays stable through brief misses. Cabin interactions pause during menu control.

The ray uses the final native menu transform with its per-eye translation removed, supports flat and curved panels, and is expressed relative to the headset. Beam and intersection use the same raw controller forward axis. Quest tip pitch compensation is disabled at the user's request.

Cursor movement currently uses smoothed relative mouse input. Direct Windows cursor positioning was tested and did not move ATS's menu cursor. Relative gain starts at 1.5 on each axis. The installed config file `controller_menu_cursor_gain.txt` contains horizontal and vertical multipliers, for example `1.5 1.5`; valid edits reload every half second, invalid values preserve the last valid pair. Accepted values are finite, positive, and at most 10. This is a tuning workaround; direct internal ATS cursor positioning remains unimplemented.

The grip-and-rotate air mouse remains a fallback when the native menu surface is unavailable. Version-specific native hooks retain their executable and signature guards. Controller input reads do not add OpenXR synchronization calls.

Validation: both regression suites pass, including ray/surface geometry, eye-offset removal, pointer ownership, stationary movement, independent gain, reverse travel, and invalid edits. The user reports the relative-pointer version is almost working, but exact cursor/laser alignment remains unresolved. No claim of pixel-exact positioning or complete click coverage is made. Plugin 0.34.8 is installed with matching release binary.
