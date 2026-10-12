#pragma once
#include <cstdint>
struct SignalRequestGate {
 uint64_t sequence=0,created=0,generation=0;uintptr_t vehicle=0;int direction=0;unsigned char initial=0;
 void Cancel() noexcept {vehicle=0;direction=0;sequence=0;}
 // Result: 0 unchanged, 1 queued, 2 cancelled, 3 arrived, 4 timeout.
 int Step(uintptr_t owner,uint64_t world,uint64_t now,bool fresh,unsigned char flags,int side,bool edge,uint64_t ready,uint64_t next) noexcept {
  if(vehicle && (owner!=vehicle || world!=generation || (flags&0x1c)!=initial)){Cancel();}
  if(edge && vehicle && side==direction){Cancel();return 2;}
  if(edge && fresh && !(flags&0x10) && (flags&0xc)!=(side==1?4:8)){
   vehicle=owner;generation=world;created=now;direction=side;initial=flags&0x1c;sequence=next;return 1;
  }
  if(vehicle && side==direction && fresh && (ready==sequence || now-created>=2000)){
   int result=ready==sequence?3:4;Cancel();return result;
  }
  return 0;
 }
};
