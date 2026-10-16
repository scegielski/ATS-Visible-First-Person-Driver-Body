#pragma once
#include <cstdint>

#include <array>
#include <cstring>
inline bool IsCabinBodyAnchor(const char* path) {
 return path && std::strstr(path,"/interior/") && !std::strstr(path,"/accessory/");
}
// Borrow only within the native view callback; never retain an engine model.
// Deduplicate interior submodels independently for every stereo/mirror view.
struct NativeBodyViewLedger {
 uint32_t frame=0;uint64_t generation=0;std::array<uintptr_t,16> views{};
 bool Take(uint32_t currentFrame,uint64_t world,uintptr_t view) {
  if(!view)return false;
  if(frame!=currentFrame||generation!=world){frame=currentFrame;generation=world;views={};}
  for(auto prior:views)if(prior==view)return false;
  for(auto& slot:views)if(!slot){slot=view;return true;}
  return false;
 }
};
