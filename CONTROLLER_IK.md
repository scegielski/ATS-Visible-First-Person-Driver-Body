# Controller hand IK

The user validated both hands in the Mustang with the 0.18.4 build. An early OpenXR API layer registers Oculus Touch thumbstick actions before ATS attaches its session action sets. The SPF plugin reads those actions and polls SteamVR controller poses through OpenVR, then applies runtime arm IK after torso compensation. All components run inside ATS; there is no companion process. Other OpenXR applications are passed through by the layer. This route is validated with SteamVR and Oculus Touch controllers only.

Press each thumbstick to calibrate and enable that hand. Hold the controller at the intended wheel grip: its position/orientation and headset heading align with the current animated wrist without jumping. Later clicks toggle the hand; holding for 0.7 seconds recalibrates and enables it. Re-enabling calibrates again. World changes disable both hands and clear calibration. Activation/release blend over 0.18 seconds. Lost tracking fades back to steering. Targets clamp to the arm envelope without stretching bones; fingers retain their animations. Existing gestures, horn, signal and ignition temporarily take priority on their respective arms.

## Installation and launch

With ATS closed, install the matching plugin DLL, spf_controller_openvr.dll and the license files into the existing SPF plugin folder. Install controller-layer/spf_controller_xr_layer.dll and its JSON manifest together there. Register the manifest's absolute path as a DWORD value of zero under HKCU\SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit. Retain other registered layers.

Install controller-layer/openxr_loader.dll in the ATS bin/win_x64 directory and set o_openxr_external_loader to 1 in the game config, backing up both beforehand. The official loader is Khronos 1.1.63. Keep the existing body archive and calibrated live pose files.

The working startup explicitly enables XR_APILAYER_SPF_driver_controller_input and supplies the Steam application identity. Use Launch-ATS-ControllerHands.ps1 with Steam already running. Normal Steam launch did not activate the bridge in the comparison; routine Steam launch integration remains pending. The launcher affects only the child ATS process. To roll back, set the external-loader option to 0 and disable/remove only this layer registration. Local backups are in backups/controller-ik.

## Validation and limitation

Regression tests pass for all steering frames, both arms, calibration, toggle/hold edges, mapped wrist displacement, unchanged other joints and unreachable target clamping. Layer tests preserve ATS action sets and synchronization inputs, verify per-hand boolean states and reject unfocused input. Runtime logs captured over eight thousand left-hand and six thousand right-hand solves, with zero rejected solves and zero caught pose faults. The user confirmed both controller hands work.

The current elbow guide uses the animated elbow plus a small inward/downward offset. The user reports intermittent wrong-way bends. This is the working checkpoint before elbow-stability changes. Finger tracking is not implemented.

References: [Valve pose query](https://github.com/ValveSoftware/openvr/wiki/IVRSystem::GetDeviceToAbsoluteTrackingPose) and [Khronos loader layer documentation](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/specification/loader/api_layer.adoc).

## Elbow-stability checkpoint (0.18.5)
The new solver uses a torso-relative down/out/back guide and transports the prior elbow bend along the changing shoulder-to-wrist ray. It limits guide rotation to 180 degrees per second and retains continuity when the guide is nearly parallel to the reach. The controller hand target and wrist orientation are preserved. config/controller_elbow_guides.txt reloads every half second; left/right lines contain finite nonzero model-space guide vectors. Tests cover continuous reach sweeps across all steering frames, wrist position/orientation preservation, and no instantaneous pole reversal. The user requested this checkpoint be committed after using it. Runtime health shows both hands active, thousands of solves and zero rejected solves or caught pose faults. Forearm twist sharing is a separate next change.

## Axial twist sharing checkpoint (0.18.6)
Controller hand roll about the forearm axis is decomposed relative to the calibrated wrist. Half is moved into the forearm's local rotation and the inverse applied at the wrist, preserving the hand's model-space position and orientation. The share fades with controller activation/release. The angle is unwrapped across half turns to avoid discontinuity. Only forearm/wrist local rotations change; finger rotations remain unchanged. config/controller_forearm_twist_share.txt reloads twice a second and accepts 0 through 1, initially 0.50. Zero leaves all roll at the wrist; one places all axial roll in the forearm. Tests cover both arms and all steering poses with continuous 0-to-200-degree rolls, unchanged wrist targets and exact half share. The user requested this version be committed after the runtime test. Health recorded 7245 left and 6763 right solves with zero rejected solves and zero caught pose faults.
