# 0.2.3 diagnostic comparison — not a confirmed visibility fix

The 0.2.2 log confirms mounted version and stock carpet selection, without a stock actor/model load error. Tifa workshop item 3810525140 is the working passenger reference. The two added entries isolate the rendering path; no Tifa assets are copied or modified.

Test A: choose Diagnostic A - Tifa actor on carpet. It references the exact installed tifa_seat animated hookup, compensates for the carpet transform, and moves the known toyseat pose sideways onto the driver position. Check for the conspicuous Tifa character. This tests whether any known working actor can spawn from the carpet wrapper.

Test B: return carpet to original, then in passenger/toyseat accessories select Diagnostic B - Stock driver on toyseat. This uses our same headless stock model, skeleton and neutral animation through the known toyseat category; the mount compensates for that attachment to place the driver in the original intended location. This temporarily replaces the selected passenger accessory; reselect Tifa to restore it afterward.

Report A visible yes/no and B visible yes/no (T-pose also counts as visible, but report it).

A no / B yes points to the carpet actor path and supports moving to direct animated-interior integration. A yes / B no points to our stock actor registration/model/clip rather than nested carpet support. Both no leaves registration/instantiation or placement unresolved and warrants an actual engine observer rather than another fit tweak. Both yes means the combination or original hookup instance needs inspection. These are diagnostic outcomes, not definitive causes without runtime log corroboration.

The existing procedural and SPF Stock Driver Body options remain intact. No test alters the rig or bakes the body into static geometry.
