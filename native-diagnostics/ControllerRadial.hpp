#pragma once
#include <cmath>
#include <cstdint>
inline int ControllerRadialDirection(float x,float y){
 if(!std::isfinite(x)||!std::isfinite(y)||std::hypot(x,y)<.45f)return -1;
 constexpr float sector=.78539816339f;return (static_cast<int>(std::floor(std::atan2(x,y)/sector+.5f))+8)%8;
}
struct ControllerRadialConfirm {
 bool tracking=false,down=false;int selected=-1;uint64_t flashUntil=0;
 bool Step(bool valid,bool pressed,int highlighted,uint64_t now){
  if(!valid){*this={};return false;}
  if(!tracking){tracking=true;down=pressed;return false;}
  bool edge=pressed&&!down;down=pressed;
  if(!edge||highlighted<0||highlighted>7)return false;
  selected=highlighted;flashUntil=now+350;return true;
 }
 bool Flash(int highlighted,uint64_t now)const{return selected==highlighted&&now<flashUntil;}
};
