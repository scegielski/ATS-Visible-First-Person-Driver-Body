# Controller menu mouse prototype

In ATS menus or while paused, hold either controller grip and rotate that controller to steer the existing mouse cursor. Squeeze the trigger for left click or hold it to drag; release to end the click. Tilt the thumbstick up/down to scroll. Release grip to reposition without moving the cursor. The first gripping controller owns the cursor until released.

This uses relative controller rotation, not a laser intersecting the floating VR UI panel. Direct panel-aligned pointing is not implemented. Input is limited to foreground ATS and menu/pause detection. Cabin interactions are suspended while menu control is active. Tracking loss, grip release, menu exit and plugin unload release a held mouse button. A trigger already held when acquiring the cursor must be released before clicking.

Menu polling is independent of version-specific animation hook activation. Unsupported executables retain their native-hook guard; only the menu prototype may run while the framework reports a paused game. Controller input reads use the game's synchronized action state without adding OpenXR synchronization calls.

Validation: driver and controller-layer regressions pass, covering rotation, held-input suppression, tracking loss and yaw wrap. The user confirmed menu control works. The separately committed ATS compatibility fix restores driving features.
