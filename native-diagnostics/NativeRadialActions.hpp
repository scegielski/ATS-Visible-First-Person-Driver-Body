#pragma once
#include <cstdint>
inline unsigned MergeNativeRadialAction(unsigned original,uint64_t requested,uint64_t now){return requested&&now>=requested&&now-requested<=500?2u:original;}
