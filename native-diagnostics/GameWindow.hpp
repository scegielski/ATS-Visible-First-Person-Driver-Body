#pragma once
#include <cstdint>
#include <cmath>
inline int NativeWindowSide(uintptr_t actor,uintptr_t state,uintptr_t player){if(!player || actor!=player)return -1;return state==actor+0x434?0:state==actor+0x44c?1:-1;}
template<class Fn> inline bool ForwardControllerWindow(Fn original,uintptr_t actor,uintptr_t state,float dt,unsigned char open,unsigned char close,unsigned char electricity,uintptr_t player,bool allowed,float direction){
 if(allowed && NativeWindowSide(actor,state,player)>=0 && std::isfinite(dt) && dt>=0 && dt<=.1f && std::isfinite(direction) && !open && !close){open=direction<-.2f;close=direction>.2f;}
 return original(actor,state,dt,open,close,electricity);
}
