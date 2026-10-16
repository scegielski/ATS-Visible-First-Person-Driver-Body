#pragma once
#include <cmath>
struct ControllerUpNudge {bool armed=false;bool Step(float y,bool active){if(!active||!std::isfinite(y)){armed=false;return false;}if(y<.25f)armed=true;if(y>.65f&&armed){armed=false;return true;}return false;}};

struct ControllerHistoryNudge {bool armed=false;int Step(float x,float y,bool active){if(!active||!std::isfinite(x)||!std::isfinite(y)){armed=false;return 0;}if(std::hypot(x,y)<.25f){armed=true;return 0;}if(armed&&std::hypot(x,y)>.65f){armed=false;if(std::abs(x)>.65f&&std::abs(y)<.45f)return x<0?-1:1;}return 0;}};
