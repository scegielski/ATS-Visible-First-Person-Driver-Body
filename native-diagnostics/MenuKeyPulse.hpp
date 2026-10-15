#pragma once
#include <cstdint>
struct MenuKeyPulse {
 bool down=false;uint64_t started=0;
 int Step(bool active,bool fire,uint64_t now){
  if(down&&(!active||now<started||now-started>=80)){down=false;return -1;}
  if(active&&fire&&!down){down=true;started=now;return 1;}
  return 0;
 }
};
