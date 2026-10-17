#pragma once
#include "InteractionBoxEditor.hpp"
inline std::string GearHudNumber(int gear){return std::to_string(gear);}
inline std::string SerializeGearHud(const InteractionBox& b,bool enabled=true){std::ostringstream out;out<<std::setprecision(9)<<"GEAR_HUD 2\n"<<(enabled?1:0)<<'\n';for(float v:b.center)out<<v<<' ';for(float v:b.rotation)out<<v<<' ';out<<b.scale;for(float v:b.stretch)out<<' '<<v;out<<'\n';return out.str();}
inline bool ParseGearHud(const std::string& text,InteractionBox& output,bool* enabled=nullptr){std::istringstream in(text);std::string tag,extra;unsigned version;InteractionBox b=output;if(!(in>>tag>>version)||tag!="GEAR_HUD"||(version!=1&&version!=2))return false;int visible=1;if(version==2&&(!(in>>visible)||(visible!=0&&visible!=1)))return false;for(float& v:b.center)if(!(in>>v)||!std::isfinite(v)||std::abs(v)>50)return false;float n=0;for(float& v:b.rotation){if(!(in>>v)||!std::isfinite(v))return false;n+=v*v;}if(n<.81f||n>1.21f)return false;for(float& v:b.rotation)v/=std::sqrt(n);if(!(in>>b.scale)||!std::isfinite(b.scale)||b.scale<=0||b.scale>10)return false;for(float& v:b.stretch)if(!(in>>v)||!std::isfinite(v)||v<=0||v>100)return false;if(in>>extra)return false;output=b;if(enabled)*enabled=visible!=0;return true;}

struct GearHudTrackingAnchor {
 bool valid=false;DriverIK::V reference{},translation{};DriverIK::Q referenceQ{1,0,0,0},rotation{1,0,0,0};
 void Update(DriverIK::V neutral,DriverIK::Q neutralQ,DriverIK::V xrHead,DriverIK::Q xrQ,DriverIK::V steamHead,DriverIK::Q steamQ){
  if(valid&&reference==neutral&&referenceQ==neutralQ)return;reference=neutral;referenceQ=neutralQ;rotation=DriverIK::Product(steamQ,DriverIK::Inverse(xrQ));translation=DriverIK::Sub(steamHead,DriverIK::Rotate(rotation,xrHead));valid=true;
 }
 DriverIK::V Position(DriverIK::V point)const{return DriverIK::Add(translation,DriverIK::Rotate(rotation,point));}
 DriverIK::Q Orientation(DriverIK::Q q)const{return DriverIK::Product(rotation,q);}
};
