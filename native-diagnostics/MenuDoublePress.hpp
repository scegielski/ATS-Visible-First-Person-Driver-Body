#pragma once
#include <cstdint>
struct MenuDoublePress {
 bool tracking=false,previous=false,waiting=false;uint64_t first=0;
 bool Step(bool valid,bool pressed,uint64_t now){
  if(!valid){*this={};return false;}
  if(!tracking){tracking=true;previous=pressed;return false;}
  bool edge=pressed&&!previous;previous=pressed;
  if(waiting&&(now<first||now-first>500))waiting=false;
  if(!edge)return false;
  if(waiting){waiting=false;return true;}
  waiting=true;first=now;return false;
 }
};
