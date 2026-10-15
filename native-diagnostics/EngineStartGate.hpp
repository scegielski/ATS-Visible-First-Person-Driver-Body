#pragma once
#include <cstdint>
struct DeferredEngineStart {
 uintptr_t vehicle=0;uint64_t generation=0,created=0,sequence=0;
};
struct EngineStartGate {
 enum Outcome {None,KeyReached,Timeout,Cancelled};
 DeferredEngineStart pending{};
 bool Queue(uintptr_t vehicle,uint64_t generation,uint64_t now,uint64_t sequence) noexcept {
  if(pending.vehicle)return false;pending={vehicle,generation,now,sequence};return true;
 }
 bool Cancel() noexcept {bool active=pending.vehicle!=0;pending={};return active;}
 Outcome Poll(uintptr_t currentVehicle,uint64_t generation,uint64_t now,bool fresh,uint32_t state,uint64_t reached,DeferredEngineStart& output) noexcept {
  output={};if(!pending.vehicle)return None;
  if(!fresh || currentVehicle!=pending.vehicle || generation!=pending.generation || state!=0){Cancel();return Cancelled;}
  if(reached==pending.sequence || now-pending.created>=2000){output=pending;bool key=reached==pending.sequence;pending={};return key?KeyReached:Timeout;}
  return None;
 }
};
inline bool IsManualEngineStartCaller(uintptr_t base,uintptr_t returnAddress) noexcept {
 // The game's player-control engine/electricity bindings. Automatic/forced callers pass.
 return returnAddress==base+0x9f81ac || returnAddress==base+0x9f832c || returnAddress==base+0x5e1cee;
}
