# Stock Mustang driver — animated interior prototype

Select **SPF Stock Driver Body (animated prototype)** in Mustang carpet accessories after restarting ATS. The procedural body remains selectable as a fallback.

Uses the installed male driver model/rig, default Lukas appearance, with a private PMD/PMG path. Head-weighted triangles are made invisible while hands and the original 61-bone skeleton are retained. Stock materials and textures are referenced directly in base_share.scs; no copies of those textures are packaged.

The game animation system drives a two-frame hold clip derived from the neutral frame of the original Mustang animation. This is a rigged actor, not baked rigid geometry. The full original animation contains steering movement; this first milestone holds the neutral seated pose to avoid unrelated looping hand motion. Live wheel/pedal synchronization is not implemented.

The wrapper compensates for the carpet’s 180-degree Y rotation and subtracts the stock cabin interior origin, so the original vehicle-space pose is placed in interior coordinates. Software checks reconstruct the bind skeleton, skin the neutral pose, validate head removal while retaining hand faces, and verify stock dependency paths. Official SCS conversion of the mount succeeds. ATS actor loading, VR camera clearance and wheel fit still require an in-game test.

To test: restart, load Mustang, select the new stock-driver carpet entry, look down and around the wheel in VR. If it is invisible or remains in T-pose, report that and inspect game.log.txt for the hookup or animation load error. Re-select the procedural prototype or original carpet to revert.

Build source: build_stock_variant.py; diagnostic rig parser: diagnose_stock_rig.py; mount source: tools/base/model/spf_stock_driver/mount.pim and mount.pit. Validation: stock_driver_validation.json.

## Runtime confirmation — October 11, 2026

The user tested installed 0.2.4 in Mustang VR and confirmed the body is visible and in the perfect position. This closes the visibility/placement milestone. Live steering and pedal synchronization remain unimplemented.
