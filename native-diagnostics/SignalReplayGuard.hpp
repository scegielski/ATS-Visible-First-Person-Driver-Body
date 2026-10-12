#pragma once
#include <cstdint>
inline bool KeepSignalReplay(int expected,uint64_t released,uint64_t now,int observed,bool hazards) noexcept {
 return expected!=0 && released!=0 && now>=released && now-released<=500 && !hazards && (observed==0 || observed==expected);
}
