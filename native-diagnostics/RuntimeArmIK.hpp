#pragma once
#include "DriverRig.hpp"
#include "HornPose.hpp"
namespace DriverIK {
using V=std::array<float,3>;using Q=std::array<float,4>;using Pose=std::array<Q,61>;using Positions=std::array<V,61>;
inline V Add(V a,V b){return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
inline V Sub(V a,V b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
inline V Mul(V a,float s){return {a[0]*s,a[1]*s,a[2]*s};}
inline float Dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
inline V Cross(V a,V b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
inline float Length(V v){return std::sqrt(Dot(v,v));}
inline V Unit(V v){float n=Length(v);return n>1e-7f?Mul(v,1/n):V{0,1,0};}
inline Q Product(Q a,Q b){return {a[0]*b[0]-a[1]*b[1]-a[2]*b[2]-a[3]*b[3],a[0]*b[1]+a[1]*b[0]+a[2]*b[3]-a[3]*b[2],a[0]*b[2]-a[1]*b[3]+a[2]*b[0]+a[3]*b[1],a[0]*b[3]+a[1]*b[2]-a[2]*b[1]+a[3]*b[0]};}
inline Q Inverse(Q q){return {q[0],-q[1],-q[2],-q[3]};}
inline V Rotate(Q q,V v){V t=Mul(Cross({q[1],q[2],q[3]},v),2);return Add(v,Add(Mul(t,q[0]),Cross({q[1],q[2],q[3]},t)));}
inline Q Between(V a,V b){a=Unit(a);b=Unit(b);float d=std::clamp(Dot(a,b),-1.f,1.f);if(d<-.9999f){V axis=Unit(Cross(a,std::abs(a[0])<.8f?V{1,0,0}:V{0,1,0}));return {0,axis[0],axis[1],axis[2]};}V c=Cross(a,b);Q q{1+d,c[0],c[1],c[2]};float n=0;for(float x:q)n+=x*x;for(float& x:q)x/=std::sqrt(n);return q;}
struct World {Positions p{};Pose q{};};
inline World FK(const Pose& pose,const Positions& positions){World w;for(unsigned i=0;i<61;++i){unsigned p=kDriverParents[i];if(p==255){w.p[i]=positions[i];w.q[i]=pose[i];}else{w.p[i]=Add(w.p[p],Rotate(w.q[p],positions[i]));w.q[i]=Product(w.q[p],pose[i]);}}return w;}
inline bool Solve(Pose& pose,const Positions& positions,unsigned upper,unsigned lower,unsigned hand,V target,Q orientation,V pole){
 auto w=FK(pose,positions);V root=w.p[upper],elbow=w.p[lower],wrist=w.p[hand];float l1=Length(Sub(elbow,root)),l2=Length(Sub(wrist,elbow));V delta=Sub(target,root);float d=Length(delta);
 if(l1<.001f || l2<.001f || d<.001f || d>l1+l2-1e-5f || d<std::abs(l1-l2)+1e-5f)return false;
 V direction=Mul(delta,1/d);V bend=Sub(Sub(pole,root),Mul(direction,Dot(Sub(pole,root),direction)));
 if(Length(bend)<1e-5f)bend=Cross(direction,std::abs(direction[1])<.9f?V{0,1,0}:V{1,0,0});bend=Unit(bend);
 float along=(l1*l1-l2*l2+d*d)/(2*d);V next=Add(root,Add(Mul(direction,along),Mul(bend,std::sqrt(std::max(0.f,l1*l1-along*along)))));
 Q upperWorld=Product(Between(Sub(elbow,root),Sub(next,root)),w.q[upper]);
 Q lowerWorld=Product(Between(Sub(wrist,elbow),Sub(target,next)),w.q[lower]);
 pose[upper]=Product(Inverse(w.q[kDriverParents[upper]]),upperWorld);
 pose[lower]=Product(Inverse(upperWorld),lowerWorld);pose[hand]=Product(Inverse(lowerWorld),orientation);return true;
}
// Transactional: never leave a torso lean applied without both hands compensated.
inline bool LeanAndCompensate(Pose& pose,const Positions& positions,float amount,float& usedDegrees,float& maxError,float forwardMeters=0.f,V pivotOffset={0,0,0}){
 usedDegrees=0;maxError=0;if(!std::isfinite(amount) || !std::isfinite(forwardMeters) || forwardMeters<0 || forwardMeters>.2f)return false;amount=std::clamp(amount,0.f,1.f);if(amount==0)return true;
 for(const auto& p:positions)for(float x:p)if(!std::isfinite(x) || std::abs(x)>10)return false;
 for(float v:pivotOffset)if(!std::isfinite(v) || std::abs(v)>.25f)return false;
 auto original=pose;auto before=FK(original,positions);
 V rightTarget=Add(Add(before.p[28],V{0,0,-forwardMeters*amount}),pivotOffset);
 for(unsigned attempt=0;attempt<8;++attempt){
  auto candidate=original;float degrees=forwardMeters>0?(25.f+attempt*3.f)*amount:10.f*amount*std::pow(.65f,static_cast<float>(attempt));float a=-degrees*3.14159265358979323846f/180.f/3.f;
  Q lean{std::cos(a/2),std::sin(a/2),0,0};for(unsigned bone=1;bone<=3;++bone){auto current=FK(candidate,positions);Q desired=Product(lean,current.q[bone]);candidate[bone]=Product(Inverse(current.q[kDriverParents[bone]]),desired);}
  auto leaned=FK(candidate,positions);
  // Move the elbow guide with the shoulder, retaining the original anatomical bend.
  V leftPole=Add(leaned.p[5],Sub(before.p[6],before.p[5]));
  V originalGuide=Sub(before.p[27],before.p[26]);
  V rightPole=Add(leaned.p[26],Add(Mul(originalGuide,1-amount),Mul(V{.15f,-.25f,.08f},amount)));
  if(!Solve(candidate,positions,5,6,7,before.p[7],before.q[7],leftPole) || !Solve(candidate,positions,26,27,28,rightTarget,before.q[28],rightPole))continue;
  auto after=FK(candidate,positions);float error=std::max(Length(Sub(after.p[7],before.p[7])),Length(Sub(after.p[28],rightTarget)));
  if(!std::isfinite(error) || error>.001f)continue;
  for(unsigned bone: {1u,2u,3u,5u,6u,7u,26u,27u,28u}){auto& q=candidate[bone];float norm=0;for(float x:q)norm+=x*x;if(!std::isfinite(norm) || norm<.98f || norm>1.02f)return false;for(float& x:q)x/=std::sqrt(norm);}
  pose=candidate;usedDegrees=degrees;maxError=error;return true;
 }
 return false;
}
inline bool InsetLeftElbow(Pose& pose,const Positions& positions,float meters,float& handError){
 handError=0;if(!std::isfinite(meters) || meters<0 || meters>.15f)return false;if(meters==0)return true;
 for(const auto& p:positions)for(float x:p)if(!std::isfinite(x) || std::abs(x)>10)return false;
 auto before=FK(pose,positions);auto candidate=pose;
 V guide=Add(before.p[6],V{meters,0,0});
 if(!Solve(candidate,positions,5,6,7,before.p[7],before.q[7],guide))return false;
 auto after=FK(candidate,positions);handError=Length(Sub(after.p[7],before.p[7]));
 if(!std::isfinite(handError) || handError>.001f || after.p[6][0]<before.p[6][0]-.001f)return false;
 pose=candidate;return true;
}
inline bool InsetRightElbow(Pose& pose,const Positions& positions,float meters,float& handError){
 handError=0;if(!std::isfinite(meters) || meters<0 || meters>.15f)return false;if(meters==0)return true;
 for(const auto& p:positions)for(float x:p)if(!std::isfinite(x) || std::abs(x)>10)return false;
 auto before=FK(pose,positions);auto candidate=pose;
 V guide=Add(before.p[27],V{-meters,0,0});
 if(!Solve(candidate,positions,26,27,28,before.p[28],before.q[28],guide))return false;
 auto after=FK(candidate,positions);handError=Length(Sub(after.p[28],before.p[28]));
 if(!std::isfinite(handError) || handError>.001f || after.p[27][0]>before.p[27][0]+.001f)return false;
 pose=candidate;return true;
}

}


