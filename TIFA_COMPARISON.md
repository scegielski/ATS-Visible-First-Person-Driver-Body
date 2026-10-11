# 0.2.2 runtime investigation

Latest ATS log confirms version 0.2.1 mounted and spfstock.ford.mustang_67.carpet selected. It has no stock body model/animation load error; user reports no body. Absence of errors does not prove actor instantiation.

Working installed Tifa passenger reference: Steam Workshop item 3810525140, universal/unit/hookup/vehicle/tifa_seat.sii. Mustang definition uses accessory_addon_int_data; animated_hookup references a skinned model and PMA. Its compiled wrapper has zero pieces/materials, two selectable parts/locators and empty-geometry bounds (FLT_MAX / -FLT_MAX). Our wrapper had a tiny triangle and tiny finite bounds at the carpet floor. This mismatch provides a culling hypothesis, not a confirmed engine trace.

Version 0.2.2 replaces our wrapper with an independently written, zero-piece/material, locator-only PMG v21 / PMD v4 with matching empty-geometry structure. It retains our driver placement transform and one visible part/locator. This removes the placeholder geometry/culling variable. Stock 61-bone model, head suppression, neutral animation clip, private paths, hookup registration, interior accessory class and procedural fallback are unchanged. No Tifa meshes, animation or textures were copied.

Minimal writer: make_locator_only_mount.py, called by build_stock_variant.py. Structural checks validate offsets, byte lengths, locator membership, variant visibility and dependency paths. Runtime rendering still needs confirmation: restart ATS and reselect the stock body option after selecting original carpet. If it remains invisible, the carpet-category actor instantiation path versus the known toyseat path remains to be traced; direct interior-rig integration is an alternative rather than proof that this attempt works.
