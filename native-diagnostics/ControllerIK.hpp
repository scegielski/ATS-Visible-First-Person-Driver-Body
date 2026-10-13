#pragma once
#include "RuntimeArmIK.hpp"
#include "HeadTorso.hpp"
#include <sstream>
struct ControllerElbowGuides {std::array<DriverIK::V,2> values{{{-.12f,-.35f,.12f},{.12f,-.35f,.12f}}};};
inline bool ParseControllerElbowGuides(const std::string& text,ControllerElbowGuides& output){
 if(text.size()>512)return false;std::istringstream input(text);ControllerElbowGuides candidate;std::string label;
 for(unsigned side=0;side<2;++side){if(!(input>>label) || label!=(side?"right":"left"))return false;for(float& x:candidate.values[side])if(!(input>>x) || !std::isfinite(x))return false;if(DriverIK::Length(candidate.values[side])<1e-5f)return false;}
 if(input>>label)return false;output=candidate;return true;
}
inline DriverIK::V StableControllerBend(DriverIK::V direction,DriverIK::V preference,DriverIK::V priorDirection,DriverIK::V priorBend,float seconds){
 using namespace DriverIK;
 V transported=Rotate(Between(priorDirection,direction),priorBend);transported=Sub(transported,Mul(direction,Dot(transported,direction)));
 if(Length(transported)<1e-5f)transported=Cross(direction,std::abs(direction[1])<.9f?V{0,1,0}:V{1,0,0});transported=Unit(transported);
 V desired=Sub(preference,Mul(direction,Dot(preference,direction)));
 // The preferred pole has no reliable side near parallel; preserve continuity there.
 if(Length(desired)<.15f*Length(preference))return transported;desired=Unit(desired);
 float angle=std::atan2(Dot(direction,Cross(transported,desired)),std::clamp(Dot(transported,desired),-1.f,1.f));
 float maximum=3.14159265f*std::clamp(seconds,0.f,.05f);angle=std::clamp(angle,-maximum,maximum);
 Q turn{std::cos(angle/2),direction[0]*std::sin(angle/2),direction[1]*std::sin(angle/2),direction[2]*std::sin(angle/2)};return Unit(Rotate(turn,transported));
}
struct ControllerHand {
 bool enabled=false,calibrate=false,down=false,held=false;
 uint64_t downTick=0;
 DriverIK::V origin{},anchor{};
 DriverIK::Q originRotation{1,0,0,0},anchorRotation{1,0,0,0},basis{1,0,0,0};
 DriverIK::Q bodyOrientation{1,0,0,0};DriverIK::V priorDirection{0,0,-1},priorBend{0,-1,0};bool haveBend=false;
 DriverIK::Q neutralWrist{1,0,0,0};float previousTwist=0;bool haveTwist=false;
 void Button(bool pressed,uint64_t now,bool valid){
  if(!valid){down=false;held=false;return;}
  if(pressed && !down){enabled=!enabled;calibrate=enabled;downTick=now;held=false;}
  if(pressed && !held && now-downTick>=700){enabled=true;calibrate=true;held=true;}
  down=pressed;
 }
};
inline bool ShareControllerWristTwist(DriverIK::Pose& pose,const DriverIK::Positions& positions,unsigned lower,unsigned wrist,ControllerHand& hand,float share){
 using namespace DriverIK;if(!std::isfinite(share) || share<0 || share>1 || lower>=61 || wrist>=61)return false;
 V axis=positions[wrist];for(float x:axis)if(!std::isfinite(x))return false;if(Length(axis)<.001f)return false;axis=Unit(axis);
 Q relative=Product(pose[wrist],Inverse(hand.neutralWrist));for(float x:relative)if(!std::isfinite(x))return false;float projected=relative[1]*axis[0]+relative[2]*axis[1]+relative[3]*axis[2];float norm=std::sqrt(relative[0]*relative[0]+projected*projected);
 // Pure 180-degree swing has no defined axial twist; leave it unchanged.
 if(norm<1e-5f)return true;float w=relative[0]/norm,s=projected/norm;if(w<0){w=-w;s=-s;}float angle=2*std::atan2(s,w);
 if(hand.haveTwist){while(angle-hand.previousTwist>3.14159265f)angle-=6.2831853f;while(angle-hand.previousTwist< -3.14159265f)angle+=6.2831853f;}
 float amount=angle*share;Q turn{std::cos(amount/2),axis[0]*std::sin(amount/2),axis[1]*std::sin(amount/2),axis[2]*std::sin(amount/2)};
 pose[lower]=Product(pose[lower],turn);pose[wrist]=Product(Inverse(turn),pose[wrist]);hand.previousTwist=angle;hand.haveTwist=true;return true;
}
inline bool ApplyControllerHand(DriverIK::Pose& pose,const DriverIK::Positions& positions,bool right,ControllerHand& hand,DriverIK::V point,DriverIK::Q rotation,DriverIK::Q headRotation,float blend,float seconds=1.f/90.f,DriverIK::V bodyGuide={0,0,0},float twistShare=.5f,bool rigidControllerOffset=false){
 using namespace DriverIK;
 if(!hand.enabled || !std::isfinite(blend) || !std::isfinite(seconds) || seconds<0 || !std::isfinite(twistShare) || twistShare<0 || twistShare>1)return false;
 if(Length(bodyGuide)<1e-5f)bodyGuide=ControllerElbowGuides{}.values[right?1:0];for(float x:bodyGuide)if(!std::isfinite(x))return false;
 for(float x:point)if(!std::isfinite(x))return false;
 for(auto q:{rotation,headRotation}){float n=0;for(float x:q){if(!std::isfinite(x))return false;n+=x*x;}if(n<.98f || n>1.02f)return false;}
 unsigned upper=right?26:5,lower=upper+1,wrist=upper+2;auto world=FK(pose,positions);
 if(hand.calibrate){
  float yaw=std::atan2(2*(headRotation[0]*headRotation[2]+headRotation[1]*headRotation[3]),1-2*(headRotation[2]*headRotation[2]+headRotation[1]*headRotation[1]));
  hand.basis={std::cos(yaw/2),0,-std::sin(yaw/2),0};hand.origin=point;hand.originRotation=rotation;hand.anchor=world.p[wrist];hand.anchorRotation=world.q[wrist];hand.bodyOrientation=world.q[3];hand.haveBend=false;hand.neutralWrist=pose[wrist];hand.haveTwist=false;hand.calibrate=false;
 }
 V target=Add(hand.anchor,Rotate(hand.basis,Sub(point,hand.origin)));
 Q change=Product(Product(hand.basis,Product(rotation,Inverse(hand.originRotation))),Inverse(hand.basis));
 Q orientation=Product(change,hand.anchorRotation);
 if(rigidControllerOffset)target=Add(point,Rotate(Product(rotation,Inverse(hand.originRotation)),Sub(hand.anchor,hand.origin)));
 // Clamp only to the physical arm envelope, never stretch the skeleton.
 V ray=Sub(target,world.p[upper]);float distance=Length(ray),maximum=Length(positions[lower])+Length(positions[wrist])-.002f,minimum=std::abs(Length(positions[lower])-Length(positions[wrist]))+.002f;
 if(distance<1e-6f)return false;target=Add(world.p[upper],Mul(ray,std::clamp(distance,minimum,maximum)/distance));
 blend=std::clamp(blend,0.f,1.f);target=Add(world.p[wrist],Mul(Sub(target,world.p[wrist]),blend));
 Q relative=Product(orientation,Inverse(world.q[wrist]));orientation=Product(Fraction(relative,blend),world.q[wrist]);
 V direction=Unit(Sub(target,world.p[upper]));
 if(!hand.haveBend){hand.priorDirection=direction;V reference=Sub(world.p[lower],world.p[upper]);hand.priorBend=Unit(Sub(reference,Mul(direction,Dot(reference,direction))));}
 V preferred=Rotate(Product(world.q[3],Inverse(hand.bodyOrientation)),bodyGuide);
 V animated=Sub(world.p[lower],world.p[upper]);preferred=Add(Mul(preferred,blend),Mul(animated,1-blend));
 V bend=StableControllerBend(direction,preferred,hand.priorDirection,hand.priorBend,seconds);
 auto candidate=pose;V pole=Add(world.p[upper],Mul(bend,.5f));
 if(!Solve(candidate,positions,upper,lower,wrist,target,orientation,pole))return false;
 if(!ShareControllerWristTwist(candidate,positions,lower,wrist,hand,twistShare*blend))return false;
 auto result=FK(candidate,positions);if(Length(Sub(result.p[wrist],target))>.001f)return false;
 hand.priorDirection=direction;hand.priorBend=bend;hand.haveBend=true;pose=candidate;return true;
}
