#pragma once
#include "RuntimeArmIK.hpp"
// Bend poles change the elbow/knee plane without moving the palm/foot endpoint.
inline unsigned ApplyIKBendTargets(DriverIK::Pose& pose,const DriverIK::Positions& positions,const DriverIK::V (&targets)[4],bool allowLeft=true,bool allowRight=true){
 using namespace DriverIK;unsigned mask=0;for(unsigned target=0;target<4;++target){if(target==0&&!allowLeft||target==1&&!allowRight)continue;bool valid=true;for(float v:targets[target])if(!std::isfinite(v))valid=false;if(!valid)continue;unsigned upper=target<2?(target?26:5):(target==2?48:54),lower=upper+1,end=upper+(target<2?2:3);auto before=FK(pose,positions);auto candidate=pose;if(Solve(candidate,positions,upper,lower,end,before.p[end],before.q[end],targets[target])){pose=candidate;mask|=1u<<target;}}return mask;
}
