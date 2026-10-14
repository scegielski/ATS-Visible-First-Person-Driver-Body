#pragma once
#include <cmath>
#include <algorithm>
struct ControllerRadioCommand {int station=0;float volume=0;bool toggle=false;};
struct ControllerRadioStick {int horizontal=0;ControllerRadioCommand Step(float x,float y,bool valid){if(!valid || !std::isfinite(x)||!std::isfinite(y)){horizontal=0;return {};}int next=x>.65f?1:x<-.65f?-1:0;ControllerRadioCommand result;result.station=next && next!=horizontal?next:0;if(std::abs(x)<.25f || next)horizontal=next;result.volume=std::abs(y)>.25f?std::clamp(y,-1.f,1.f):0;return result;}};

inline int NextControllerRadioStation(uint64_t current,uint64_t count,int direction){if(!count || count>10000 || !direction)return -1;if(current>=count)return direction>0?0:static_cast<int>(count-1);return static_cast<int>(direction>0?(current+1)%count:(current+count-1)%count);}

inline int ControllerRadioPlaybackAction(bool hasStream,uint64_t current,uint64_t count){if(hasStream)return -2;if(!count || count>10000)return -1;return static_cast<int>(current<count?current:0);}

// Match the native Next action: lazily load an empty list, then refresh selection.
template<class Initialize,class Refresh> bool EnsureControllerRadioList(uint64_t& count,Initialize initialize,Refresh refresh){if(count)return count<=10000;if(!initialize() || !refresh())return false;return count>0 && count<=10000;}
