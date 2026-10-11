#pragma once
#include <array>
#include <cmath>
#include <cstdint>
struct DriverSample {
 uintptr_t controller=0, model=0;
 uint32_t index=0;
 float duration=0, requested=0;
 uint64_t calls=0, tick=0;
};
inline bool ValidSample(const DriverSample& sample) {
 return sample.controller && sample.model && sample.index<4096 &&
  std::isfinite(sample.duration) && sample.duration>0 && sample.duration<3600 &&
  std::isfinite(sample.requested) && sample.requested>=0 &&
  sample.requested<=sample.duration*1.001f;
}
// Bounded ledger; callers provide synchronization. Do not dereference retained pointers.
struct SampleLedger {
 std::array<DriverSample,32> slots{};
 uint64_t dropped=0;
 bool Record(DriverSample sample) {
  if(!ValidSample(sample)) return false;
  for(auto& slot:slots) if(slot.controller==sample.controller && slot.model==sample.model && slot.index==sample.index) {
   sample.calls=slot.calls+1; slot=sample; return true;
  }
  for(auto& slot:slots) if(!slot.controller) { sample.calls=1; slot=sample; return true; }
  ++dropped; return false;
 }
};
