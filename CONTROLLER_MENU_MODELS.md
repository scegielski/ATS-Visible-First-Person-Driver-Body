# SteamVR controller models and shared pointers

Version 0.37.4 loads each controller's SteamVR render-model name, component meshes and textures at runtime. No Quest assets are distributed. Models appear in menus and in the cabin while that hand's IK is disabled; axes remain a fallback when model rendering is unavailable.

The bridge draws textured geometry in a separate D3D11 OpenXR stereo projection layer using runtime eye poses and FOVs. It retains the original frame layers and display time, restores the game graphics state, and introduces no additional input synchronization. Typed render-target views support SteamVR's typeless swapchain textures.

Both menu and cabin targeting use SteamVR's tip component transform. Both share white overlay beams and hit dots. Beam width stays at 1.5 millimetres regardless of hit distance; texture aspect changes to preserve beam length without widening misses. Cabin collision queries remain in the existing scene frame, with their hit distance passed to the shared pointer display.

All four regression suites pass, including software D3D11 stereo rendering, parallax, graphics-state restoration, inactive-layer behavior and original frame forwarding. In VR the user confirmed the Quest models, menu pointing/clicking, cabin models and aiming, and final white fixed-width pointer appearance. This visual check does not establish long-term timing behavior across all runtime sessions.

Installed plugin and bridge match the release artifacts. Diagnostics are in controller_models.log and MENU_MODEL entries in the plugin and early-menu logs.

Version 0.37.5 feeds the native controller bridge inputs into SteamVR component state: stick axes/click, X/A, Y/B, analog trigger and grip. The mapping follows the installed Quest render-model definitions. It falls back to legacy controller state when native input is unavailable. All four suites pass, including mapping and neutral-release checks. Installed for the user test; the user requested commit and push afterward without a separate detailed component-by-component visual report.
