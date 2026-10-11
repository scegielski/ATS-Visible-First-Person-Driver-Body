#pragma once
#include "HornMotion.hpp"
#include "HornPoseTargets.hpp"
#include <array>
inline std::array<float,4> HornSlerp(std::array<float,4> a,std::array<float,4> b,float t) noexcept {
 float dot=0;for(unsigned i=0;i<4;++i)dot+=a[i]*b[i];
 if(dot<0){dot=-dot;for(float& v:b)v=-v;}
 dot=std::clamp(dot,0.f,1.f);t=std::clamp(t,0.f,1.f);
 float first=1-t,second=t;
 if(dot<.9995f){const float angle=std::acos(dot),den=std::sin(angle);first=std::sin((1-t)*angle)/den;second=std::sin(t*angle)/den;}
 std::array<float,4> result{};float norm=0;
 for(unsigned i=0;i<4;++i){result[i]=a[i]*first+b[i]*second;norm+=result[i]*result[i];}
 if(norm>0)for(float& v:result)v/=std::sqrt(norm);
 return result;
}
inline void BlendHornPose(std::array<std::array<float,4>,61>& rotations,const HornMotion& motion) noexcept {
 if(motion.reach<=0)return; // Byte-exact live steering pose when fully released.
 const float reach=motion.reach*motion.reach*(3-2*motion.reach);
 for(const auto& target:kHornTargets){
  const auto desired=HornSlerp(target.hover,target.pressed,motion.press);
  rotations[target.bone]=HornSlerp(rotations[target.bone],desired,reach);
 }
}
