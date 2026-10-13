#pragma once
#include "RuntimeArmIK.hpp"
#include "HeadTorso.hpp"
struct ControllerHand {
 bool enabled=false,calibrate=false,down=false,held=false;
 uint64_t downTick=0;
 DriverIK::V origin{},anchor{};
 DriverIK::Q originRotation{1,0,0,0},anchorRotation{1,0,0,0},basis{1,0,0,0};
 void Button(bool pressed,uint64_t now,bool valid){
  if(!valid){down=false;held=false;return;}
  if(pressed && !down){enabled=!enabled;calibrate=enabled;downTick=now;held=false;}
  if(pressed && !held && now-downTick>=700){enabled=true;calibrate=true;held=true;}
  down=pressed;
 }
};
inline bool ApplyControllerHand(DriverIK::Pose& pose,const DriverIK::Positions& positions,bool right,ControllerHand& hand,DriverIK::V point,DriverIK::Q rotation,DriverIK::Q headRotation,float blend){
 using namespace DriverIK;
 if(!hand.enabled || !std::isfinite(blend))return false;
 for(float x:point)if(!std::isfinite(x))return false;
 for(auto q:{rotation,headRotation}){float n=0;for(float x:q){if(!std::isfinite(x))return false;n+=x*x;}if(n<.98f || n>1.02f)return false;}
 unsigned upper=right?26:5,lower=upper+1,wrist=upper+2;auto world=FK(pose,positions);
 if(hand.calibrate){
  float yaw=std::atan2(2*(headRotation[0]*headRotation[2]+headRotation[1]*headRotation[3]),1-2*(headRotation[2]*headRotation[2]+headRotation[1]*headRotation[1]));
  hand.basis={std::cos(yaw/2),0,-std::sin(yaw/2),0};hand.origin=point;hand.originRotation=rotation;hand.anchor=world.p[wrist];hand.anchorRotation=world.q[wrist];hand.calibrate=false;
 }
 V target=Add(hand.anchor,Rotate(hand.basis,Sub(point,hand.origin)));
 Q change=Product(Product(hand.basis,Product(rotation,Inverse(hand.originRotation))),Inverse(hand.basis));
 Q orientation=Product(change,hand.anchorRotation);
 // Clamp only to the physical arm envelope, never stretch the skeleton.
 V ray=Sub(target,world.p[upper]);float distance=Length(ray),maximum=Length(positions[lower])+Length(positions[wrist])-.002f,minimum=std::abs(Length(positions[lower])-Length(positions[wrist]))+.002f;
 if(distance<1e-6f)return false;target=Add(world.p[upper],Mul(ray,std::clamp(distance,minimum,maximum)/distance));
 blend=std::clamp(blend,0.f,1.f);target=Add(world.p[wrist],Mul(Sub(target,world.p[wrist]),blend));
 Q relative=Product(orientation,Inverse(world.q[wrist]));orientation=Product(Fraction(relative,blend),world.q[wrist]);
 auto candidate=pose;V pole=Add(world.p[lower],V{right?-.04f:.06f,-.03f,0});
 if(!Solve(candidate,positions,upper,lower,wrist,target,orientation,pole))return false;
 auto result=FK(candidate,positions);if(Length(Sub(result.p[wrist],target))>.001f)return false;
 pose=candidate;return true;
}
