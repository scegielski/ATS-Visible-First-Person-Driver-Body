#pragma once
#include "NativePedalSamples.hpp"
#include "PedalMotion.hpp"
// Apply only pedal deltas, retaining the active vehicle's native resting pose.
inline void ApplyNativePedals(DriverIK::Pose& pose,const PedalMotion& motion,float clutch){
 float row=motion.transfer<=0?PedalValue(motion.press)*32:motion.transfer>=1?50+PedalValue(motion.press)*32:33+PedalValue(motion.transfer)*16;
 unsigned lo=static_cast<unsigned>(row),hi=std::min(lo+1,82u);float blend=row-lo;
 for(unsigned i=0;i<3;++i){auto delta=HornSlerp(NativeRightPedals[lo][i],NativeRightPedals[hi][i],blend);pose[NativePedalBones[i]]=DriverIK::Product(pose[NativePedalBones[i]],delta);}
 float level=PedalValue(clutch)*8;unsigned loClutch=static_cast<unsigned>(level),hiClutch=std::min(loClutch+1,8u);
 pose[51]=DriverIK::Product(pose[51],HornSlerp(NativeClutchPedals[loClutch][3],NativeClutchPedals[hiClutch][3],level-loClutch));
}
