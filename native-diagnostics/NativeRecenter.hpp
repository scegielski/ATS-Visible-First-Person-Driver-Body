#pragma once
#include <cstdint>
struct NativeRecenterGate {
 uint64_t seen=0,pending=0,requested=0;
 void Observe(uint64_t sequence,uint64_t tick){if(sequence!=seen){seen=sequence;pending=sequence;requested=tick;}}
 bool Consume(uint64_t now,uint64_t poseTick,bool fresh){
  if(!pending)return false;if(now<requested || now-requested>2000){pending=0;return false;}
  if(!fresh || poseTick<=requested)return false;pending=0;return true;
 }
};

inline uint32_t MergeControllerHeadReset(uint32_t nativeRequest,uint64_t controllerTick,uint64_t now){
 return controllerTick&&now>=controllerTick&&now-controllerTick<=500?nativeRequest|1u:nativeRequest;
}
