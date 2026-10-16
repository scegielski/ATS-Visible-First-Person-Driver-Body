#pragma once
#include <cmath>
struct ControllerUpNudge {bool armed=false;bool Step(float y,bool active){if(!active||!std::isfinite(y)){armed=false;return false;}if(y<.25f)armed=true;if(y>.65f&&armed){armed=false;return true;}return false;}};
