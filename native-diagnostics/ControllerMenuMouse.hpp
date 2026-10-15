#pragma once
#include <cmath>
#include <algorithm>
struct MenuMouseOutput {float dx=0,dy=0;int button=0,wheel=0;};
struct ControllerMenuMouse {
 bool tracking=false,down=false,armed=false;float yaw=0,pitch=0,scroll=0;
 MenuMouseOutput Step(bool valid,bool grip,float y,float p,float trigger,float stick,float dt){
  MenuMouseOutput out;
  if(!valid || !grip || !std::isfinite(y)||!std::isfinite(p)||!std::isfinite(trigger)||!std::isfinite(stick)) {if(down)out.button=-1;tracking=down=armed=false;scroll=0;return out;}
  if(tracking){float delta=std::remainder(y-yaw,6.283185307f);if(std::abs(delta)<.35f && std::abs(p-pitch)<.35f){out.dx=delta*1500;out.dy=-(p-pitch)*1500;}}
  else {tracking=true;armed=trigger<.2f;}
  yaw=y;pitch=p;
  if(trigger<.2f){armed=true;if(down){down=false;out.button=-1;}}
  if(trigger>.7f && armed && !down){down=true;armed=false;out.button=1;}
  if(std::abs(stick)>.4f){scroll+=stick*std::clamp(dt,0.f,.05f)*8;out.wheel=static_cast<int>(scroll);scroll-=out.wheel;}else scroll=0;
  return out;
 }
};
