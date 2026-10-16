#pragma once
#include "RuntimeArmIK.hpp"
struct ControllerFoot {
 bool enabled=false,returning=false,havePose=false;float returnSeconds=0;DriverIK::Q releasedPose[3]{{1,0,0,0},{1,0,0,0},{1,0,0,0}};DriverIK::V origin{},anchor{},knee{};DriverIK::Q originRotation{1,0,0,0},anchorRotation{1,0,0,0};
 void Calibrate(const DriverIK::Pose& pose,const DriverIK::Positions& positions,bool right,DriverIK::V point,DriverIK::Q rotation){auto world=DriverIK::FK(pose,positions);unsigned foot=right?57:51;origin=point;originRotation=rotation;anchor=world.p[foot];anchorRotation=world.q[foot];knee=world.p[right?55:49];enabled=true;returning=false;returnSeconds=0;unsigned upper=right?54:48;releasedPose[0]=pose[upper];releasedPose[1]=pose[upper+1];releasedPose[2]=pose[foot];havePose=true;}
 bool Apply(DriverIK::Pose& pose,const DriverIK::Positions& positions,bool right,DriverIK::V point,DriverIK::Q rotation){
  using namespace DriverIK;if(!enabled)return false;for(float v:point)if(!std::isfinite(v))return false;float norm=0;for(float v:rotation){if(!std::isfinite(v))return false;norm+=v*v;}if(norm<.98f||norm>1.02f)return false;
  unsigned upper=right?54:48,lower=upper+1,foot=upper+3;auto world=FK(pose,positions);Q change=Product(rotation,Inverse(originRotation)),orientation=Product(change,anchorRotation);V target=Add(point,Rotate(change,Sub(anchor,origin))),ray=Sub(target,world.p[upper]);float distance=Length(ray),l1=Length(positions[lower]),l2=Length(positions[foot]),minimum=std::abs(l1-l2)+.002f,maximum=l1+l2-.002f;if(distance<1e-6f||maximum<=minimum)return false;target=Add(world.p[upper],Mul(ray,std::clamp(distance,minimum,maximum)/distance));auto candidate=pose;if(!Solve(candidate,positions,upper,lower,foot,target,orientation,knee))return false;pose=candidate;releasedPose[0]=pose[upper];releasedPose[1]=pose[lower];releasedPose[2]=pose[foot];havePose=true;return true;
 }
 void Release(){if(enabled){enabled=false;returning=havePose;returnSeconds=0;}}
 bool Return(DriverIK::Pose& pose,bool right,float seconds){
  if(enabled||!returning)return false;if(!std::isfinite(seconds)||seconds<0)return false;returnSeconds+=std::min(seconds,.05f);float t=std::clamp(returnSeconds/.35f,0.f,1.f);if(t>=1){returning=false;return false;}t=t*t*(3-2*t);unsigned upper=right?54:48;unsigned bones[]{upper,upper+1,upper+3};for(unsigned i=0;i<3;++i)pose[bones[i]]=t==0?releasedPose[i]:HornSlerp(releasedPose[i],pose[bones[i]],t);return true;
 }
};
