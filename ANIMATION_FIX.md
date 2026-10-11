# 0.2.4: malformed seated clip corrected

Runtime differential: user confirms Diagnostic A (Tifa on carpet) visible and Diagnostic B (stock driver on toyseat) invisible. This disproves carpet-only spawning as the principal failure.

Comparison with the working Tifa PMA and original Mustang PMA exposed our clip-writer bug: PMA version 5 has five data-pointer/size fields at offsets 64..80. The writer had incorrectly begun editing at offset 72. The two-frame file therefore declared bone indices at 272 instead of 92 and transforms at 84 instead of 153. Those addresses pointed into unrelated arrays. This was a confirmed invalid clip, regardless of absent log errors.

The corrected 6985-byte clip declares time array 84, bone indices 92, transforms 153, end 6985 and expanded size 7009. It retains the original 61 bone-index mapping and duplicates the exact original Mustang neutral frame. The model, rig, head suppression, material paths and mount are unchanged. Regression tests use header-declared pointers and reject the earlier malformed header; finite transform/quaternion checks pass. ATS visibility remains unverified until the user runs this correction.

Restart ATS; restore original carpet if Diagnostic A was selected, then choose SPF Stock Driver Body (animated prototype). If Diagnostic B remains selected in passenger accessories, restore Tifa there to avoid two copies. No further attachment experiment is needed for this test.

## Runtime confirmation — October 11, 2026

The user tested installed 0.2.4 in Mustang VR and confirmed the body is visible and in the perfect position. This closes the visibility/placement milestone. Live steering and pedal synchronization remain unimplemented.
