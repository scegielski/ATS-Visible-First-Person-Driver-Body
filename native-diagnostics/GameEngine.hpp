#pragma once
#include <cstdint>
#include <cstddef>
// Read the same engine-state enum as the engine's query_variable("engine") branch.
// Candidate from static analysis; runtime validation required before pose control.
enum class PlayerEngineState : uint32_t { Off=0,Starting=1,Running=2,Stopping=3 };
template<class Read> bool ReadPlayerEngine(uintptr_t base,Read read,PlayerEngineState& state) noexcept {
 state=PlayerEngineState::Off;uintptr_t game=0,owner=0,vtable=0,query=0,vehicle=0;uint32_t value=0;
 if(!read(base+0x36badd0,&game,8) || !game ||
    !read(game+0xaa0,&owner,8) || !owner ||
    !read(owner,&vtable,8) || !vtable ||
    !read(vtable+0x1b8,&query,8) || query!=base+0x4d9cf0 ||
    !read(owner+0x31b0,&vehicle,8) || !vehicle ||
    !read(vehicle+0x1b8,&value,4) || value>3)return false;
 state=static_cast<PlayerEngineState>(value);return true;
}
