# Restored Exterior Head Prototype

The archive 0.7.0 restores the original installed game's full driver.pmg rather than generating a new head. Its 61-bone skeleton is byte-identical to the existing body. All tested animation and attachment files are copied unchanged from the installed working archive; only the driver mesh and package version are changed.

Plugin 0.13.2 keeps that full head visible outside. In the interior camera, or when camera identification is unavailable, the private body's rendered head/neck matrices (46 and 47) collapse at a shared neck anchor. Other bone palette entries and animation inputs are unchanged. Returning outside lets the game's normal skeleton assembly regenerate the original matrices. This remains a render-palette prototype and requires a visual check of the neck seam and VR view: some seam vertices carry weights outside the head/neck pair.

The validated player-only exterior replacement guard remains in place. The original player driver is hidden outside; traffic instances do not match the required controller/model pair. Both visibility modes remain selectable through body_visibility.txt.

Install the matching 0.13.2 plugin and 0.7.0 archive together with ATS closed. Keep the existing installed archive filename to preserve profile references. The prior working files are backed up before installation. Archive checks verify original full mesh, unchanged skeleton, unchanged other assets and matching installed source. Build and regression tests pass; exterior head/interior clipping visual validation passed in the user's VR/exterior test.

Final validation: the user confirmed the head restoration works. The installed archive and plugin match the release files byte-for-byte, and runtime visibility writes recorded zero caught faults.
