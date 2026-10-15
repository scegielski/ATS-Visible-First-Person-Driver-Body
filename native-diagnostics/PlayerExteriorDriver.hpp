#pragma once
#include <cstdint>
#include <cstddef>
// Player vehicle uses driver controller +0x1df0; traffic updater uses +0x2c8.
// Only eligible after CaptureDriver observes this exact controller/model pair.
template<class Read> bool ReadPlayerExteriorDriver(uintptr_t base,Read read,uintptr_t& controller,uintptr_t& model) noexcept {
 controller=model=0;uintptr_t game=0,owner=0,vtable=0,query=0,vehicle=0,candidate=0;
 if(!read(base+0x36badd0,&game,8) || !game ||
    !read(game+0xaa0,&owner,8) || !owner ||
    !read(owner,&vtable,8) || !vtable ||
    !read(vtable+0x1b8,&query,8) || query!=base+0x4d9cf0 ||
    !read(owner+0x31b0,&vehicle,8) || !vehicle ||
    !read(vehicle+0x1df8,&candidate,8) || !candidate)return false;
 controller=vehicle+0x1df0;model=candidate;return true;
}
inline bool IsConfirmedPlayerDriver(uintptr_t controller,uintptr_t model,uintptr_t expectedController,uintptr_t expectedModel) noexcept {
 return controller && model && expectedController && expectedModel && controller==expectedController && model==expectedModel;
}
