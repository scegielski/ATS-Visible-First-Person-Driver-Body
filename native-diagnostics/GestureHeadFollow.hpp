#pragma once
#include "RuntimeArmIK.hpp"
inline float GestureLookYaw(DriverIK::Q q) noexcept {
 return std::atan2(2*(q[0]*q[2]+q[1]*q[3]),1-2*(q[2]*q[2]+q[1]*q[1]));
}
inline float GestureLookPitch(DriverIK::Q q) noexcept {
 return std::asin(std::clamp(DriverIK::Rotate(q,{0,0,-1})[1],-1.f,1.f));
}
inline bool FollowGestureHead(DriverIK::Pose& pose,const DriverIK::Positions& positions,bool right,float reach,float yaw,float gain,float pitch=0.f) {
 if(!std::isfinite(pitch) || !std::isfinite(reach) || !std::isfinite(yaw) || !std::isfinite(gain) || gain<0 || gain>1)return false;
 reach=std::clamp(reach,0.f,1.f);if(reach==0 || gain==0)return true;
 for(const auto& p:positions)for(float x:p)if(!std::isfinite(x))return false;
 const unsigned upper=right?26:5;const auto before=DriverIK::FK(pose,positions);
 float weight=gain*reach*reach*(3-2*reach),angle=yaw*weight,tilt=pitch*weight;DriverIK::Q yawTurn{std::cos(angle/2),0,std::sin(angle/2),0},pitchTurn{std::cos(tilt/2),std::sin(tilt/2),0,0};
 const auto turn=DriverIK::Product(yawTurn,pitchTurn);
 const auto desired=DriverIK::Product(turn,before.q[upper]);auto local=DriverIK::Product(DriverIK::Inverse(before.q[kDriverParents[upper]]),desired);
 float norm=0;for(float x:local)norm+=x*x;if(!std::isfinite(norm) || norm<.98f || norm>1.02f)return false;
 for(float& x:local)x/=std::sqrt(norm);pose[upper]=local;return true;
}
