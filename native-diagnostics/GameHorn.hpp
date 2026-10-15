#pragma once
#include <cstdint>
#include <cstddef>
// Same player horn field used by the engine's query_variable("horn") branch.
// Exact executable fingerprint is checked before this function is used.
template<class Read> bool ReadPlayerHorn(uintptr_t base,Read read,bool& held) noexcept {
 held=false;uintptr_t game=0,owner=0,vtable=0,query=0,vehicle=0,horn=0;unsigned char value=0;
 if(!read(base+0x36badd0,&game,8) || !game ||
    !read(game+0xaa0,&owner,8) || !owner ||
    !read(owner,&vtable,8) || !vtable ||
    !read(vtable+0x1b8,&query,8) || query!=base+0x4d9cf0 ||
    !read(owner+0x31b0,&vehicle,8) || !vehicle ||
    !read(vehicle+0x1d38,&horn,8) || !horn ||
    !read(horn+0xf0,&value,1) || value>1)return false;
 held=value!=0;return true;
}
