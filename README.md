# ATS Visible Driver Body — Mustang

Version **0.2.4** is confirmed working by the user in ATS 1.61.3.1 Mustang VR: the stock driver body is visible and in the correct position.

The selectable **SPF Stock Driver Body (animated prototype)** carpet accessory uses the game's male driver avatar, retaining its 61-bone skeleton. Head-related triangles are hidden while hands remain. The game animation system holds the neutral pose derived from the original Mustang animation. Live steering and pedal synchronization are not implemented. Stock materials and textures are referenced from the installed game rather than bundled. A separate procedural body remains selectable as fallback. Diagnostic A/B entries are retained in this checkpoint.

## Install

With ATS closed, copy releases/ATS-VisibleDriverBody-Mustang-0.2.4.scs into the ATS user mod directory. Existing installation uses the older filename ATS-VisibleDriverBody-Mustang-0.1.0.scs to preserve profile references; replace that file rather than enabling both archives. Enable the mod, load the Mustang and select SPF Stock Driver Body (animated prototype) under carpet accessories. Restore the original carpet to remove it.

## Source and validation

- build_stock_variant.py reads installed stock driver assets, hides head faces, writes a private descriptor and creates a neutral animation clip.
- make_locator_only_mount.py writes the empty animated actor mount with attachment-transform compensation.
- stock_assets.py is the read-only SCS archive reader used for asset lookup.
- generate_body.py creates the independent procedural fallback.
- diagnose_stock_rig.py reconstructs stock bind matrices and pose transforms for analysis only; it does not bake the shipping body into static geometry.
- test_stock_animation.py validates PMA v5 pointers, bone mapping, neutral frames and rejection of the prior malformed header.
- stock_driver_validation.json records checks and the user's successful 0.2.4 VR test.

The main visibility defect was the generated PMA header: data offsets start at byte 64, but the earlier writer edited byte 72 onward. The corrected hold clip declares arrays at 84/92/153 and preserves the original pose/rig mapping. See ANIMATION_FIX.md and TIFA_COMPARISON.md for the diagnostic evidence.

## Build prerequisites

Python with NumPy is needed for rig analysis. The existing workspace provides extracted Ford assets at ../extracted-ford and stock-reference/driver.pmg plus ford_mustang_67_ani.pma for diagnostics. These game/reference files, downloaded SCS conversion tools and Blender reference checkout are excluded from Git. SCS Conversion Tools 2.21 converts the original fallback source in tools/base/model/spf_driver_body. Run build_stock_variant.py afterward to regenerate the private stock avatar and locator-only mount; run test_stock_animation.py to check the clip. This checkpoint includes the exact verified package and generated model outputs, rather than depending on an untested rebuild.

No external reference mod is installed by these scripts. The repo has no remote configured at this checkpoint.
