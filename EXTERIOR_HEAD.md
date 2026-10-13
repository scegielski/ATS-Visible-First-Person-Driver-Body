# Intact tracked driver head

Validated release: plugin 0.16.13 with the unchanged 0.7.0 full-driver archive. The archive restores the original installed driver mesh and its unchanged 61-bone skeleton. Animation/attachment files and installed game material/texture references remain intact.

The approved final state keeps the full head and neck visible and tracked in both interior and exterior camera views, with zero interior offset and no collapse. The user confirmed no head hiding workaround is needed in first-person VR. The existing guarded player exterior replacement still hides only the original player driver outside, leaving traffic unmatched.

## Approved live configuration

Copy these files from releases into the existing plugin config folder:

- `interior_head_mode.txt`: `visible`
- `interior_head_backshift.txt`: `0.00`

Both reload twice a second outside the skeleton callback. Head orientation follows the calibrated headset pose in both views, with world-space correction accounting for torso lean. The head's default shape and position are preserved.

Optional diagnostic controls remain available: `collapse` maps the head and neck to the actual neck origin, and a signed offset moves them along model Z (positive rearward, negative forward). These controls affect only interior rendering. Invalid/missing/incomplete edits retain the last valid settings. The final approved configuration leaves both workarounds off.

## Verified render-palette layout

The live buffer and original assembly routine establish row-major affine matrices with translations at elements 3, 7 and 11. Previous tests incorrectly used column-major translation entries 12–14, which actually belong to the homogeneous row. That caused offsets to change W rather than XYZ and made collapsed head vertices fall to zero, producing the reported stretched polygons. An earlier root-axis change did not resolve this layout mistake.

The render hook sees world bone transforms before subsequent inverse-bind multiplication. Collapse uses the neck matrix origin directly rather than transforming its bind position again. Corrected head collapse and intact offsets retain a (0,0,0,1) homogeneous row. Whole-body palette collapse also uses the verified translation entries and correct root origin.

## Validation

Regression tests include row-major translated/rotated fixtures, homogeneous-row preservation, unchanged other bones, signed model-axis offsets, invalid input without mutation, and vertex transforms after inverse-bind multiplication. These prove collapsed vertices remain at the neck with W=1 and intact offsets affect the intended translation.

The user confirmed the corrected collapse looked better, then confirmed the intact head moved with a ten-centimetre rearward offset. Render logging recorded exactly (0,0,0.10) for that test and (0,0,0) after resetting it. The user selected visible mode, zero offset and tracking in both views as the final state. The installed plugin and two live head settings match the release files. See interior_head_validation.json.
