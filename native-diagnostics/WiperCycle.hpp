#pragma once
#include <cstdint>
inline int NextControllerWiperState(uint32_t current,uint64_t count){return count>0 && count<=32 && current<count?static_cast<int>((current+1)%count):-1;}
template<class Set> inline bool ApplyControllerWiperCycle(uintptr_t actor,uint32_t current,uint64_t count,Set set){int next=NextControllerWiperState(current,count);if(!actor || next<0)return false;set(actor,static_cast<uint32_t>(next));return true;}
