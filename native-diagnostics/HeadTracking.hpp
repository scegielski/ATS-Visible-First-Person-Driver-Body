#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
struct ObservedXRView {int32_t type;uint32_t padding;uintptr_t next;std::array<float,4> orientation;std::array<float,3> position;std::array<float,4> fov;};
struct ObservedXRLocate {int32_t type;uint32_t padding;uintptr_t next;int32_t configuration;uint32_t padding2;int64_t displayTime;uintptr_t space;};
static_assert(sizeof(ObservedXRView)==64 && offsetof(ObservedXRView,position)==32);
static_assert(sizeof(ObservedXRLocate)==40 && offsetof(ObservedXRLocate,space)==32);
struct ObservedHead {std::array<float,3> position{};std::array<float,4> orientation{};uintptr_t space=0;int64_t displayTime=0;uint64_t tick=0,flags=0;};
inline bool HeadFromStereo(const std::array<ObservedXRView,2>& views,uint64_t flags,const ObservedXRLocate& locate,ObservedHead& output) noexcept {
 if((flags&3)!=3 || locate.type!=6 || !locate.space || locate.displayTime<=0)return false;
 for(const auto& view:views){
  if(view.type!=7)return false;float norm=0;
  for(float x:view.orientation){if(!std::isfinite(x))return false;norm+=x*x;}
  if(norm<.9f || norm>1.1f)return false;
  for(float x:view.position)if(!std::isfinite(x) || std::abs(x)>100)return false;
 }
 float distance=0,dot=0;
 for(unsigned i=0;i<3;++i){float d=views[0].position[i]-views[1].position[i];distance+=d*d;}
 if(distance<.0004f || distance>.04f)return false;
 for(unsigned i=0;i<4;++i)dot+=views[0].orientation[i]*views[1].orientation[i];
 ObservedHead head;float norm=0;
 for(unsigned i=0;i<3;++i)head.position[i]=(views[0].position[i]+views[1].position[i])*.5f;
 for(unsigned i=0;i<4;++i){head.orientation[i]=views[0].orientation[i]+(dot<0?-1.f:1.f)*views[1].orientation[i];norm+=head.orientation[i]*head.orientation[i];}
 if(norm<.001f)return false;for(float& x:head.orientation)x/=std::sqrt(norm);
 head.space=locate.space;head.displayTime=locate.displayTime;head.flags=flags;output=head;return true;
}
