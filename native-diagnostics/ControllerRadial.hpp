#pragma once
#include <cmath>
#include <array>
#include <cstdint>
inline int ControllerRadialDirection(float x,float y){
 if(!std::isfinite(x)||!std::isfinite(y)||std::hypot(x,y)<.45f)return -1;
 constexpr float sector=.78539816339f;return (static_cast<int>(std::floor(std::atan2(x,y)/sector+.5f))+8)%8;
}
struct ControllerRadialConfirm {
 bool tracking=false,down=false,open=false;int selected=-1;uint64_t flashUntil=0;
 bool Step(bool valid,bool pressed,int highlighted,uint64_t now){
  if(!valid){*this={};return false;}
  if(!tracking){tracking=true;down=pressed;return false;}
  bool press=pressed&&!down,release=!pressed&&down;down=pressed;
  if(press)open=true;
  if(!release||!open)return false;
  open=false;
  if(highlighted<0||highlighted>7)return false;
  selected=highlighted;flashUntil=now+350;return true;
 }
 bool Flash(int highlighted,uint64_t now)const{return selected==highlighted&&now<flashUntil;}
};
struct ControllerFingerSetupMode {
 bool enabled=false;
 bool Select(unsigned side,int segment){if(side!=0||segment!=6)return false;enabled=!enabled;return true;}
};
struct ControllerRadialNote {
 int previous=-1;
 bool Step(bool open,int segment){if(!open||segment<0||segment>7){previous=-1;return false;}if(previous==segment)return false;previous=segment;return true;}
};
inline int ControllerRadialAction(unsigned side,int segment){
 if(side==0&&segment==0)return 0;
 if(side==1&&segment==6)return 1;
 if(side==1&&segment==0)return 2;
 if(side==1&&segment==2)return 3;
 if(side==1&&segment==4)return 4;
 if(side==1&&segment==5)return 5;
 if(side==1&&segment==1)return 6;
 return -1;
}

inline bool ControllerRadialSetupSelection(unsigned side,int segment){return side==0&&segment==2;}

inline bool ControllerRadialRecenterSelection(unsigned side,int segment){return side==0&&segment==4;}

inline int ControllerRadialResetSelection(unsigned side,int segment,bool setup){if(!setup||side!=0)return -1;return segment==6?0:segment==4?1:-1;}

inline bool ControllerRadialSegmentVisible(unsigned side,int segment,bool setup){if(segment<0||segment>7||side>1)return false;return !setup||(side==0?(segment==0||segment==2||segment==4||segment==6||segment==7):(segment==0||segment==1||segment==2||segment==4||segment==7));}

inline std::array<unsigned char,3> ControllerRadialTint(bool setup,bool selected,bool flash,bool fingerEnabled=false){if(selected&&flash)return {255,255,255};if(setup)return selected?std::array<unsigned char,3>{185,90,245}:std::array<unsigned char,3>{150,80,190};return selected?std::array<unsigned char,3>{70,220,255}:fingerEnabled?std::array<unsigned char,3>{70,210,255}:std::array<unsigned char,3>{210,210,255};}

inline int ControllerRadialVisibilitySelection(unsigned side,int segment,bool setup){if(!setup||side!=1)return -1;return segment==7?0:segment==1?1:-1;}

inline bool ControllerRadialCameraSelection(unsigned side,int segment,bool setup){return setup&&side==0&&segment==7;}
