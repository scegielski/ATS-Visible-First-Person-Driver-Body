#pragma once
#include "RuntimeArmIK.hpp"
#include <sstream>
struct MenuLaserSurface {DriverIK::V center{};DriverIK::Q rotation{1,0,0,0};float width=0,height=0,radius=0;};
struct MenuLaserHit {bool hit=false;float u=0,v=0,distance=0;DriverIK::V point{};};
inline MenuLaserHit IntersectMenuLaser(const MenuLaserSurface& s,DriverIK::V origin,DriverIK::V direction){
 using namespace DriverIK;MenuLaserHit out;if(!std::isfinite(s.width)||!std::isfinite(s.height)||!std::isfinite(s.radius)||s.width<=0||s.height<=0)return out;
 auto inv=Inverse(s.rotation);auto o=Rotate(inv,Sub(origin,s.center)),d=Rotate(inv,Unit(direction));float t=-1,u=0;
 if(s.radius<=0){if(std::abs(d[2])<1e-6f)return out;t=-o[2]/d[2];if(t<=0)return out;u=.5f+(o[0]+d[0]*t)/s.width;}
 else {float r=std::max(s.radius,.63661977f),R=r*s.width;float a=d[0]*d[0]+d[2]*d[2],b=2*(o[0]*d[0]+(o[2]-R)*d[2]),c=o[0]*o[0]+(o[2]-R)*(o[2]-R)-R*R,disc=b*b-4*a*c;if(a<1e-8f||disc<0)return out;for(float candidate:{(-b-std::sqrt(disc))/(2*a),(-b+std::sqrt(disc))/(2*a)}){if(candidate<=0)continue;auto h=Add(o,Mul(d,candidate));float candidateU=.5f+std::atan2(h[0],R-h[2])*r;if(candidateU>=0 && candidateU<=1 && (t<0||candidate<t)){t=candidate;u=candidateU;}}}
 if(t<=0 || t>10 || u<0 || u>1)return out;float v=.5f-(o[1]+d[1]*t)/s.height;if(v<0||v>1)return out;return {true,u,v,t,Add(origin,Mul(Unit(direction),t))};
}
void PublishControllerMenuSurface(const float* transform,const float* size,float radius);

inline int SelectMenuLaserOwner(int owner,bool down,const bool (&hit)[2],const bool (&press)[2]){if(!down && (press[0]||press[1]))return press[1]?1:0;if(owner<0)return hit[1]?1:hit[0]?0:-1;return owner;}

using MenuPoseMatrix=std::array<std::array<float,4>,3>;
inline MenuPoseMatrix ComposeMenuAim(const MenuPoseMatrix& device,const MenuPoseMatrix& tip){MenuPoseMatrix out{};for(unsigned r=0;r<3;++r){for(unsigned c=0;c<3;++c)for(unsigned k=0;k<3;++k)out[r][c]+=device[r][k]*tip[k][c];out[r][3]=device[r][3];for(unsigned k=0;k<3;++k)out[r][3]+=device[r][k]*tip[k][3];}return out;}

struct MenuLocalRay {DriverIK::V origin,direction;};
inline MenuLocalRay MenuRayInHeadSpace(DriverIK::V controller,DriverIK::Q aim,DriverIK::V head,DriverIK::Q headRotation){auto inv=DriverIK::Inverse(headRotation);return {DriverIK::Rotate(inv,DriverIK::Sub(controller,head)),DriverIK::Rotate(DriverIK::Product(inv,aim),{0,0,-1})};}

inline void RemoveMenuEyeOffset(float* pose,const float* eye){for(unsigned i=0;i<3;++i)pose[i]+=eye[i];}

struct MenuCursorMotion {bool anchored=false;int owner=-1;float u=0,v=0,rx=0,ry=0;int width=0,height=0;
 struct Output{bool anchor=false;int x=0,y=0,dx=0,dy=0;};
 Output Step(bool valid,int hand,float nextU,float nextV,int w,int h,float dt,float gainX=1,float gainY=1){Output out;if(!valid||!std::isfinite(nextU)||!std::isfinite(nextV)||w<2||h<2){anchored=false;owner=-1;rx=ry=0;return out;}
 nextU=std::clamp(nextU,0.f,1.f);nextV=std::clamp(nextV,0.f,1.f);
 if(!anchored||owner!=hand||w!=width||h!=height){anchored=true;owner=hand;width=w;height=h;u=nextU;v=nextV;rx=ry=0;out.anchor=true;out.x=static_cast<int>(u*(w-1));out.y=static_cast<int>(v*(h-1));return out;}
 float alpha=1-std::exp(-std::clamp(dt,0.f,.05f)/.012f);float du=(nextU-u)*alpha,dv=(nextV-v)*alpha;u+=du;v+=dv;
 rx+=du*(w-1)*gainX;ry+=dv*(h-1)*gainY;out.dx=static_cast<int>(rx);out.dy=static_cast<int>(ry);rx-=out.dx;ry-=out.dy;return out;}

};

inline bool ParseMenuCursorGain(const std::string& text,float& x,float& y){std::istringstream input(text);float a=0,b=0;std::string extra;if(!(input>>a>>b)||(input>>extra)||!std::isfinite(a)||!std::isfinite(b)||a<=0||b<=0||a>10||b>10)return false;x=a;y=b;return true;}
