#pragma once
#include "RuntimeArmIK.hpp"
#include "HeadTracking.hpp"
#include <sstream>
#include <string>
struct HeadTrackingLimits {float forward=.30f,degrees=28.f,lateral=.24f;};
inline bool ParseHeadTrackingLimits(const std::string& text,HeadTrackingLimits& limits){
 if(text.size()>256)return false;std::istringstream input(text);std::string key;HeadTrackingLimits candidate;
 if(!(input>>key>>candidate.forward) || key!="forward_m" || !(input>>key>>candidate.degrees) || key!="max_lean_degrees")return false;
 if(input>>key){if(key!="lateral_m" || !(input>>candidate.lateral) || (input>>key))return false;}
 if(!std::isfinite(candidate.forward) || !std::isfinite(candidate.degrees) || !std::isfinite(candidate.lateral) || candidate.forward<0 || candidate.degrees<0 || candidate.lateral<0)return false;
 limits=candidate;return true;
}
struct HeadTorsoDrive {DriverIK::V delta{};DriverIK::Q headRotation{1,0,0,0};float yaw=0;uint64_t tick=0;};
inline float HeadYaw(const ObservedHead& head){const auto& q=head.orientation;return std::atan2(2*(q[3]*q[1]+q[0]*q[2]),1-2*(q[1]*q[1]+q[0]*q[0]));}
struct SeatedHeadCalibration {
 ObservedHead baseline{};uint64_t generation=0;HeadTorsoDrive filtered{};
 void Reset(){baseline={};filtered={};}
 bool Step(const ObservedHead& pose,uint64_t world,uint64_t now,bool fresh,bool recenter,float dt,float gain,float forwardLimit=.15f,float lateralLimit=.12f,bool allowCalibration=true) {
  if(!std::isfinite(gain) || gain<0 || gain>1 || !std::isfinite(forwardLimit) || forwardLimit<0 || !std::isfinite(lateralLimit) || lateralLimit<0)return false;
  bool calibrated=false;
  if(fresh && (!baseline.space || baseline.space!=pose.space || generation!=world || recenter)){
   if(allowCalibration || recenter){baseline=pose;generation=world;filtered={};calibrated=true;}
   else Reset();
  }
  DriverIK::V desired{};DriverIK::Q desiredHead{1,0,0,0};float yaw=0;
  if(fresh && baseline.space){
   const float neutralYaw=HeadYaw(baseline),c=std::cos(neutralYaw),s=std::sin(neutralYaw);
   auto d=DriverIK::Sub(pose.position,baseline.position);
   desired={std::clamp(c*d[0]-s*d[2],-lateralLimit,lateralLimit)*gain,std::clamp(d[1],-.08f,.08f)*gain,std::clamp(s*d[0]+c*d[2],-forwardLimit,.08f)*gain};
   float angle=HeadYaw(pose)-neutralYaw;while(angle>3.14159265f)angle-=6.2831853f;while(angle<-3.14159265f)angle+=6.2831853f;
   yaw=std::clamp(angle*.2f,-.17453293f,.17453293f)*gain;
   DriverIK::Q current{pose.orientation[3],pose.orientation[0],pose.orientation[1],pose.orientation[2]},neutral{baseline.orientation[3],baseline.orientation[0],baseline.orientation[1],baseline.orientation[2]};
   DriverIK::Q seatedYaw{std::cos(neutralYaw/2),0,std::sin(neutralYaw/2),0};
   desiredHead=DriverIK::Product(DriverIK::Product(DriverIK::Inverse(seatedYaw),DriverIK::Product(current,DriverIK::Inverse(neutral))),seatedYaw);
   if(desiredHead[0]<0)for(float& x:desiredHead)x=-x;
   // Bound total head rotation to 85 degrees, then apply the live gain.
   float headAngle=2*std::acos(std::clamp(desiredHead[0],0.f,1.f));float fraction=gain*(headAngle>1.48352986f?1.48352986f/headAngle:1.f);
   float half=std::acos(std::clamp(desiredHead[0],0.f,1.f)),sinHalf=std::sin(half);
   if(std::abs(sinHalf)>1e-6f){float scale=std::sin(half*fraction)/sinHalf;desiredHead={std::cos(half*fraction),desiredHead[1]*scale,desiredHead[2]*scale,desiredHead[3]*scale};}

  }
  const float seconds=std::isfinite(dt)?std::clamp(dt,0.f,.05f):0.f,alpha=1-std::exp(-seconds/.10f);
  for(unsigned i=0;i<3;++i)filtered.delta[i]+=(desired[i]-filtered.delta[i])*alpha;
  float sign=0;for(unsigned i=0;i<4;++i)sign+=desiredHead[i]*filtered.headRotation[i];float norm=0;
  for(unsigned i=0;i<4;++i){filtered.headRotation[i]+=(desiredHead[i]*(sign<0?-1.f:1.f)-filtered.headRotation[i])*alpha;norm+=filtered.headRotation[i]*filtered.headRotation[i];}
  if(norm>1e-6f)for(float& x:filtered.headRotation)x/=std::sqrt(norm);
  filtered.yaw+=(yaw-filtered.yaw)*alpha;filtered.tick=now;return calibrated;
 }
};
namespace DriverIK {
inline Q Fraction(Q q,float fraction){if(q[0]<0)for(float& x:q)x=-x;float a=std::acos(std::clamp(q[0],-1.f,1.f));float s=std::sin(a);if(std::abs(s)<1e-6f)return {1,0,0,0};float scale=std::sin(a*fraction)/s;return {std::cos(a*fraction),q[1]*scale,q[2]*scale,q[3]*scale};}
inline bool ApplyTrackedHead(Pose& pose,const Positions& positions,Q relative,Q neutralWorld){
 for(float x:relative)if(!std::isfinite(x))return false;
 float norm=0;for(float x:relative)norm+=x*x;if(norm<.98f || norm>1.02f)return false;
 auto candidate=pose;auto before=FK(candidate,positions);Q desired=Product(relative,neutralWorld);
 Q correction=Product(desired,Inverse(before.q[47]));Q neckWorld=Product(Fraction(correction,.65f),before.q[46]);
 candidate[46]=Product(Inverse(before.q[kDriverParents[46]]),neckWorld);
 auto adjusted=FK(candidate,positions);candidate[47]=Product(Inverse(adjusted.q[kDriverParents[47]]),desired);
 for(unsigned bone:{46u,47u}){float n=0;for(float x:candidate[bone])n+=x*x;if(!std::isfinite(n) || n<.98f || n>1.02f)return false;for(float& x:candidate[bone])x/=std::sqrt(n);}
 pose=candidate;return true;
}
inline bool TrackHeadTorso(Pose& pose,const Positions& positions,V delta,float yaw,float& applied,float& error,float maxLeanDegrees=14.f){
 applied=error=0;if(!std::isfinite(maxLeanDegrees) || maxLeanDegrees<0)return false;for(float x:delta)if(!std::isfinite(x))return false;if(!std::isfinite(yaw) || std::abs(yaw)>.5f)return false;
 for(const auto& p:positions)for(float x:p)if(!std::isfinite(x) || std::abs(x)>10)return false;
 if(Length(delta)<1e-5f && std::abs(yaw)<1e-5f)return true;
 const auto original=pose;const auto before=FK(original,positions);
 const V neck=Sub(before.p[46],before.p[1]);if(Length(neck)<.1f)return false;
 Q lean=Between(neck,Add(neck,delta));float angle=2*std::acos(std::clamp(std::abs(lean[0]),0.f,1.f));const float limit=maxLeanDegrees*.0174532925f;if(angle>limit)lean=Fraction(lean,limit/angle);
 for(unsigned attempt=0;attempt<9;++attempt){
  float strength=std::pow(.65f,static_cast<float>(attempt));auto candidate=original;
  Q bend=Fraction(lean,strength/3),turn{std::cos(yaw*strength/6),0,std::sin(yaw*strength/6),0};Q step=Product(turn,bend);
  for(unsigned bone=1;bone<=3;++bone){auto w=FK(candidate,positions);candidate[bone]=Product(Inverse(w.q[kDriverParents[bone]]),Product(step,w.q[bone]));}
  auto leaning=FK(candidate,positions);
  V leftPole=Add(leaning.p[5],Sub(before.p[6],before.p[5])),rightPole=Add(leaning.p[26],Sub(before.p[27],before.p[26]));
  if(!Solve(candidate,positions,5,6,7,before.p[7],before.q[7],leftPole) || !Solve(candidate,positions,26,27,28,before.p[28],before.q[28],rightPole))continue;
  auto after=FK(candidate,positions);float residual=std::max(Length(Sub(after.p[7],before.p[7])),Length(Sub(after.p[28],before.p[28])));
  if(!std::isfinite(residual) || residual>.001f)continue;
  bool valid=true;for(unsigned bone:{1u,2u,3u,5u,6u,7u,26u,27u,28u}){float norm=0;for(float x:candidate[bone])norm+=x*x;if(!std::isfinite(norm) || norm<.98f || norm>1.02f)valid=false;}
  if(!valid)return false;pose=candidate;applied=strength;error=residual;return true;
 }
 return false;
}
}
