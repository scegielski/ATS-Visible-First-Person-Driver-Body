#pragma once
struct SetupHelpState {bool inSetup=false,visible=false;bool Step(bool setup){bool entered=setup&&!inSetup;if(entered)visible=false;if(!setup)visible=false;inSetup=setup;return entered;}void Toggle(){if(inSetup)visible=!visible;}};
inline constexpr const wchar_t* SetupHelpLines[]{L"SETUP CONTROLS",L"MOVE BOX  -  point or touch, hold GRIP",L"RESIZE BOX  -  BOTH GRIPS, camera adjustment off",L"MOVE HAND  -  target hand box, hold TRIGGER",L"UNDO / REDO  -  left stick left / right (no click)",L"ALIGN VIEW  -  camera adjustment on, BOTH GRIPS",
 L"ROTATE VIEW  -  camera adjustment on, hold TRIGGER",L"Move controllers together; release to save",L"LEFT RADIAL  -  hold thumbstick click, tilt, and release",L"Changes save separately for the current vehicle"};
