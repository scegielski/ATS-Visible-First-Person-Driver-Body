#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstddef>
struct NativeMenuCursorEvent {uint64_t id=0;uint32_t type=3;float x=0,y=0,z=0;uint64_t tail[2]{};};
static_assert(sizeof(NativeMenuCursorEvent)==40 && offsetof(NativeMenuCursorEvent,x)==12);
inline bool BuildNativeMenuCursorEvent(float u,float v,const float* rect,NativeMenuCursorEvent& out){
 if(!std::isfinite(u)||!std::isfinite(v))return false;for(unsigned i=0;i<4;++i)if(!std::isfinite(rect[i]))return false;
 float width=rect[1]-rect[0],height=rect[2]-rect[3];if(width<=0||height<=0||width>32768||height>32768)return false;
 out={};out.type=3;out.x=std::clamp(u,0.f,1.f)*width;out.y=std::clamp(v,0.f,1.f)*height;return true;
}
