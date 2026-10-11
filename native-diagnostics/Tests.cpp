#include "DriverSamples.hpp"
#include "SteeringPose.hpp"
#include "PedalMotion.hpp"
#include "HornMotion.hpp"
#include "HornPose.hpp"
#include "GameHorn.hpp"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <limits>
extern "C" {
void* gDriverOriginal=nullptr;
void* gAdvanceOriginal=nullptr;
void* gBoneOriginal=nullptr;
uintptr_t BoneObserveStub(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uint32_t);
uintptr_t DriverObserveStub(uintptr_t);
uintptr_t AdvanceObserveStub(uintptr_t,uint32_t,float,uint64_t,uint64_t);
}
static int observedDriver=0,observedAdvance=0;
static uintptr_t recordedReturn=0;
static bool replacePose=false;
static void Check(bool result,const char* message) {
 if(!result) { std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1); }
}
extern "C" void CaptureDriver(uintptr_t controller) noexcept {
 Check(controller==0x12345678,"driver argument into observer");++observedDriver;
}
extern "C" void CaptureAdvance(uintptr_t model,uint32_t index,float requested,uintptr_t ret,float* replacement) noexcept {
 Check(model==0x87654321 && index==17 && requested==2.5f,"observer integer/float arguments");
 Check(ret!=0,"observer original return address");recordedReturn=ret;++observedAdvance;
 Check(replacement && *replacement==requested,"saved float argument address");
 if(replacePose)*replacement=1.25f;
}
static uintptr_t DriverOriginal(uintptr_t controller) {
 Check(controller==0x12345678,"original driver argument");return 0xabcdef;
}
static uintptr_t AdvanceOriginal(uintptr_t model,uint32_t index,float requested,uint64_t fourth,uint64_t fifth) {
 Check(model==0x87654321 && index==17 && requested==(replacePose?1.25f:2.5f),"original advance integer/float arguments");
 Check(fourth==0x11223344 && fifth==0x55667788,"original register and stack arguments");
 return 0x987654;
}
extern "C" void CaptureBone(uintptr_t output,uintptr_t bounds,uintptr_t arrays,uintptr_t bones,uint32_t count) noexcept {
 Check(output==11 && bounds==22 && arrays==33 && bones==44 && count==61,"bone observer register and stack forwarding");
}
static uintptr_t BoneOriginal(uintptr_t output,uintptr_t bounds,uintptr_t arrays,uintptr_t bones,uint32_t count) {
 Check(output==11 && bounds==22 && arrays==33 && bones==44 && count==61,"bone original arguments unchanged");return 123456;
}
int main() {
 constexpr uintptr_t base=0x140000000,game=0x200000000,owner=0x300000000,vt=0x142252050,vehicle=0x400000000,hornState=0x500000000;
 unsigned char heldByte=0;bool wrongOwner=false,missingHorn=false;
 auto reader=[&](uintptr_t address,void* output,size_t size) noexcept {
  uintptr_t value=0;
  if(address==hornState+0xf0 && size==1){std::memcpy(output,&heldByte,1);return true;}
  if(size!=8)return false;
  if(address==base+0x36b8db0)value=game;
  else if(address==game+0xaa0)value=owner;
  else if(address==owner)value=vt;
  else if(address==vt+0x1b8)value=base+(wrongOwner?0x1234:0x4d9c90);
  else if(address==owner+0x31b0)value=vehicle;
  else if(address==vehicle+0x1d38)value=missingHorn?0:hornState;
  else return false;
  std::memcpy(output,&value,8);return true;
 };
 bool held=true;Check(ReadPlayerHorn(base,reader,held) && !held,"game horn released state");
 heldByte=1;Check(ReadPlayerHorn(base,reader,held) && held,"game horn held state independent of physical bindings");
 wrongOwner=true;Check(!ReadPlayerHorn(base,reader,held) && !held,"reject wrong query owner and release pose");wrongOwner=false;
 missingHorn=true;Check(!ReadPlayerHorn(base,reader,held) && !held,"missing vehicle horn releases pose");missingHorn=false;
 heldByte=2;Check(!ReadPlayerHorn(base,reader,held) && !held,"reject non-boolean horn data");

 std::array<std::array<float,4>,61> original{};
 for(auto& q:original)q={1,0,0,0};
 auto pose=original;BlendHornPose(pose,{});Check(pose==original,"released horn leaves every bone byte exact");
 HornMotion full;full.reach=1;full.press=1;BlendHornPose(pose,full);
 for(unsigned bone=0;bone<61;++bone){
  bool selected=false;for(const auto& target:kHornTargets)if(target.bone==bone)selected=true;
  if(!selected)Check(pose[bone]==original[bone],"horn leaves right hand pedals and torso byte exact");
  float norm=0;for(float v:pose[bone])norm+=v*v;Check(std::abs(norm-1)<.00001f,"horn rotations remain normalized");
 }
 HornMotion halfway;halfway.reach=.5f;
 auto current=original;current[5]={0,1,0,0};auto latest=current;
 BlendHornPose(original,halfway);BlendHornPose(latest,halfway);
 Check(original[5]!=latest[5],"horn return blends against current steering grip");
 Check(HornSlerp({1,0,0,0},{-1,0,0,0},.5f)==std::array<float,4>{1,0,0,0},"horn shortest arc handles antipodal quaternions");
 HornMotion horn;
 for(int i=0;i<24;++i) {horn.Step(true,.01f);Check(horn.press==0,"horn reaches before pressing");}
 for(int i=0;i<30;++i)horn.Step(true,.01f);
 Check(horn.reach==1 && horn.press==1,"held horn finishes reach and press");
 horn.Step(false,.01f);Check(horn.reach==1 && horn.press<1,"horn lifts before withdrawal");
 for(int i=0;i<40;++i)horn.Step(false,.01f);
 Check(horn.reach==0 && horn.press==0,"horn release fully returns to live grip");
 horn.Step(true,.01f);horn.Step(false,.01f);Check(horn.reach==0 && horn.press==0,"short horn tap reverses reach");
 horn.Step(true,std::numeric_limits<float>::quiet_NaN());Check(horn.reach==0,"invalid time does not advance horn");
 gBoneOriginal=reinterpret_cast<void*>(BoneOriginal);
 Check(BoneObserveStub(11,22,33,44,61)==123456,"bone observer preserves return");
 gDriverOriginal=reinterpret_cast<void*>(DriverOriginal);
 gAdvanceOriginal=reinterpret_cast<void*>(AdvanceOriginal);
 Check(DriverObserveStub(0x12345678)==0xabcdef,"driver return value");
 Check(AdvanceObserveStub(0x87654321,17,2.5f,0x11223344,0x55667788)==0x987654,"advance return value");
 Check(observedDriver==1 && observedAdvance==1 && recordedReturn,"observer calls");
 replacePose=true;
 Check(AdvanceObserveStub(0x87654321,17,2.5f,0x11223344,0x55667788)==0x987654,"replacement preserves return and remaining arguments");
 Check(SteeringFraction(-1)==1 && SteeringFraction(0)==0 && SteeringFraction(1)==0,"steering endpoints and center seam");
 Check(SteeringFraction(-.5f)==.5f && SteeringFraction(.5f)==.5f,"steering midpoint mapping");
 Check(SteeringFraction(-2)==1 && SteeringFraction(2)==0 && SteeringFraction(std::numeric_limits<float>::quiet_NaN())==0,"clamp and invalid steering");
 Check(IsSteeringClip(1.9166667f,47) && !IsSteeringClip(2.f,2) && !IsSteeringClip(1.9166667f,2) && !IsSteeringClip(std::numeric_limits<float>::quiet_NaN(),47),"reject seated hold, wrong frame count and invalid duration");
 Check(IsThrottleClip(64.5833333f,1551) && !IsThrottleClip(1.9166667f,47) && !IsThrottleClip(64.5833333f,47),"throttle clip guard");
 Check(ThrottleStartFrame(0)==0 && ThrottleStartFrame(.5f)==752 && ThrottleStartFrame(1)==1504,"throttle release half and full positions");
 Check(ThrottleStartFrame(-1)==0 && ThrottleStartFrame(2)==1504 && ThrottleStartFrame(std::numeric_limits<float>::quiet_NaN())==0,"invalid throttle and bounds");
 for(int step=0;step<=1000;++step) {const auto frame=ThrottleStartFrame(step/1000.f);Check(frame%47==0 && frame+46<1551,"steering cannot leave selected throttle block");}
 Check(IsBrakeClip(162.5f,3901) && !IsBrakeClip(64.5833333f,1551),"brake clip guard");
 Check(IsClutchClip(1462.8333333f,35109) && !IsClutchClip(162.5f,3901),"clutch clip guard");
 Check(ClutchStartFrame(1504,0)==1504 && ClutchStartFrame(3854,1)==35062,"independent clutch and right pedal endpoints");
 for(int c=0;c<=1000;++c)for(uint32_t right=0;right<3901;right+=47) {
  const auto combined=ClutchStartFrame(right,c/1000.f);
  Check(combined%3901==right && combined+46<35109,"clutch preserves independent right pose and steering bounds");
 }
 Check(ClutchStartFrame(0,std::numeric_limits<float>::quiet_NaN())==0,"invalid clutch releases ankle");
 PedalMotion pedal;pedal.Step(1,0,.01f);Check(pedal.StartFrame()==1504,"verified gas pose retained");
 pedal.Step(1,1,.01f);Check(pedal.transfer==0 && pedal.press<1,"lift gas before moving");
 bool hovered=false,pressed=false;
 for(int tick=0;tick<100;++tick) {
  pedal.Step(1,1,.01f);
  Check(pedal.StartFrame()%47==0 && pedal.StartFrame()+46<3901,"brake pose and steering bounds");
  if(pedal.transfer>0 && pedal.transfer<1)Check(pedal.press==0,"no ankle press during lateral movement");
  if(pedal.phase==PedalMotion::BrakeHover)hovered=true;
  if(pedal.phase==PedalMotion::BrakePress) {Check(hovered,"hover before brake press");pressed=true;}
 }
 Check(pressed && pedal.transfer==1 && pedal.press==1 && pedal.StartFrame()==3854,"full brake overrides held gas");
 pedal.Step(1,0,.01f);Check(pedal.transfer==1 && pedal.press<1,"lift brake before return");
 for(int tick=0;tick<100;++tick) {
  pedal.Step(1,0,.01f);if(pedal.transfer>0 && pedal.transfer<1)Check(pedal.press==0,"return with lifted ankle");
 }
 Check(pedal.transfer==0 && pedal.press==1 && pedal.StartFrame()==1504,"return to accelerator after brake release");
 pedal={};pedal.Step(0,1,.01f);pedal.Step(0,0,.01f);Check(pedal.press==0,"interrupted reach cannot press brake");
 pedal={};pedal.Step(std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::quiet_NaN(),.01f);Check(pedal.StartFrame()==0,"invalid pedal input returns neutral");
 SampleLedger ledger;DriverSample sample{1,2,0,10.f,5.f,0,100};
 Check(ledger.Record(sample),"valid sample");sample.requested=6.f;Check(ledger.Record(sample),"update sample");
 Check(ledger.slots[0].calls==2 && ledger.slots[0].requested==6.f,"ledger accumulation");
 sample.requested=std::numeric_limits<float>::quiet_NaN();Check(!ledger.Record(sample),"reject NaN");
 sample.requested=11;Check(!ledger.Record(sample),"reject past-duration request");
 sample.requested=0;sample.duration=0;Check(!ledger.Record(sample),"reject zero duration");
 ledger={};sample.duration=10;
 for(uintptr_t i=1;i<=32;++i) {sample.controller=i;Check(ledger.Record(sample),"bounded slot allocation");}
 sample.controller=33;Check(!ledger.Record(sample) && ledger.dropped==1,"bounded overflow");
 ledger={};Check(!ledger.slots[0].controller && !ledger.dropped,"world reset clears ledger");
 std::puts("PASS: observer forwarding, integer/float/stack arguments, return values, bounded samples, invalid-data rejection and reset");
}
