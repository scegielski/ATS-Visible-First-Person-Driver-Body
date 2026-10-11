#include "DriverSamples.hpp"
#include "SteeringPose.hpp"
#include "PedalMotion.hpp"
#include "HornMotion.hpp"
#include "HornPose.hpp"
#include "GameHorn.hpp"
#include "WindowGesture.hpp"
#include "LiveGesturePose.hpp"
#include "RightGestureTargets.hpp"
#include <iomanip>
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
 auto encode=[](const GesturePoseTargets& pose){std::ostringstream text;text<<"SPF_GESTURE_POSE 1\n"<<std::setprecision(9);for(const auto& bone:pose){text<<"bone "<<bone.bone;for(float v:bone.hover)text<<" "<<v;for(float v:bone.pressed)text<<" "<<v;text<<"\n";}return text.str();};
 auto encodeV2=[](const GesturePoseTargets& pose){std::ostringstream text;text<<"SPF_GESTURE_POSE 2\n"<<std::setprecision(9);for(const auto& bone:pose){text<<"bone "<<bone.bone;for(float v:bone.hover)text<<" "<<v;for(float v:bone.intermediate)text<<" "<<v;for(float v:bone.pressed)text<<" "<<v;text<<"\n";}return text.str();};
 WindowGestureMotion staged;
 bool intermediateReached=false;
 for(int tick=0;tick<80;++tick){const bool arrived=staged.reach>=1;staged.Step(true,.01f);if(!arrived)Check(staged.extend==0,"middle finger cannot rise before hand arrives");if(staged.reach==1 && staged.extend==0)intermediateReached=true;}
 Check(intermediateReached && staged.extend==1,"distinct finger-down arrival before extension");
 staged.Step(false,.01f);Check(staged.reach<1 && staged.press==1 && staged.extend==1,"release starts arm return immediately without open-hand exit keyframe");
 for(int tick=0;tick<40;++tick)staged.Step(false,.01f);
 Check(staged.reach==0 && staged.press==0 && staged.extend==0,"direct release ends at steering grip");
 WindowGestureMotion brief;brief.Step(true,.05f);brief.Step(false,.05f);Check(brief.extend==0,"short tap never raises middle finger before arrival");
 LiveGesturePose rightFile{kRightGestureTargets};std::string rightError;
 Check(rightFile.Replace(encodeV2(kRightGestureTargets),rightError),"right pose file accepts its own bone set");
 const auto rightSnapshot=rightFile.Snapshot();
 Check(!rightFile.Replace(encodeV2(kWindowGestureTargets),rightError) && rightFile.Snapshot()==rightSnapshot,"right pose file rejects left bones and keeps prior pose");
 auto rightRotations=std::array<std::array<float,4>,61>{};for(auto& q:rightRotations)q={1,0,0,0};const auto beforeRight=rightRotations;
 WindowGestureMotion rightFinal;rightFinal.reach=rightFinal.press=rightFinal.extend=1;
 BlendWindowGesture(rightRotations,rightFinal,*rightSnapshot);
 for(unsigned i=0;i<61;++i) {
  if(i<25 || i>45)Check(rightRotations[i]==beforeRight[i],"right gesture preserves left hand torso and pedals");
  float norm=0;for(float v:rightRotations[i])norm+=v*v;Check(std::abs(norm-1)<.00001f,"right gesture keeps normalized joints");
 }
 auto both=rightRotations;BlendWindowGesture(both,rightFinal,kWindowGestureTargets);
 for(unsigned i=25;i<=45;++i)Check(both[i]==rightRotations[i],"left gesture does not overwrite right pose");
 BlendHornPose(both,HornMotion{1,1});for(unsigned i=25;i<=45;++i)Check(both[i]==rightRotations[i],"left horn does not overwrite right gesture");
 WindowGestureMotion independentRight;independentRight.Step(true,.05f);LeftArmGestures independentLeft;independentLeft.Step(true,false,.05f);
 Check(independentRight.reach>0 && independentLeft.window.reach==0,"right gesture operates independently of left horn");
 LiveGesturePose stagedFile;std::string stagedError;auto stagedTargets=kWindowGestureTargets;
 stagedTargets[1].intermediate={0,1,0,0};stagedTargets[1].pressed={1,0,0,0};
 Check(stagedFile.Replace(encodeV2(stagedTargets),stagedError),"three-keyframe live pose accepted");
 auto stagedSnapshot=stagedFile.Snapshot();
 auto stagedRotations=std::array<std::array<float,4>,61>{};for(auto& q:stagedRotations)q={1,0,0,0};
 WindowGestureMotion atArrival;atArrival.reach=atArrival.press=1;
 BlendWindowGesture(stagedRotations,atArrival,*stagedSnapshot);Check(std::abs(stagedRotations[5][1])>.99999f,"intermediate joint pose sampled at arrival");
 atArrival.extend=1;BlendWindowGesture(stagedRotations,atArrival,*stagedSnapshot);Check(stagedRotations[5]==std::array<float,4>{1,0,0,0},"final keyframe sampled after extension");
 Check(!stagedFile.Replace("SPF_GESTURE_POSE 2\nbone 4 1 0 0 0 1 0 0 0\n",stagedError) && stagedFile.Snapshot()==stagedSnapshot,"incomplete third-keyframe write keeps valid pose");
 LiveGesturePose livePose;std::string poseError;
 const auto initial=livePose.Snapshot();auto editable=kWindowGestureTargets;editable[1].pressed={1,0,0,0};
 const auto validText=encode(editable);
 Check(livePose.Replace(validText,poseError) && livePose.Generation()==1,"valid file publishes new pose");
 auto accepted=livePose.Snapshot();Check((*accepted)[1].pressed==std::array<float,4>{1,0,0,0},"file joint rotation reaches snapshot");
 Check(initial->at(1).pressed==kWindowGestureTargets[1].pressed,"existing snapshot remains immutable after reload");
 Check(!livePose.Replace("SPF_GESTURE_POSE 1\nbone 4 1 0 0 0 1 0 0 0\n",poseError) && livePose.Snapshot()==accepted,"partial write keeps last valid pose");
 Check(!livePose.Replace(validText+"bone 4 1 0 0 0 1 0 0 0\n",poseError),"duplicate bone rejected");
 auto bad=editable;bad[1].pressed={0,0,0,0};Check(!livePose.Replace(encode(bad),poseError),"zero quaternion rejected");
 bad=editable;bad[1].bone=25;Check(!livePose.Replace(encode(bad),poseError),"file cannot target right arm");
 bad=editable;bad[1].pressed={1.05f,0,0,0};Check(livePose.Replace(encode(bad),poseError) && livePose.Snapshot()->at(1).pressed[0]==1,"small quaternion drift normalized");
 Check(!livePose.Replace(std::string(65537,'x'),poseError),"oversized pose file rejected");
 GesturePoseTargets parsed{};Check(ParseGesturePose(validText+"# comments allowed\n",parsed,poseError),"pose comments supported");
 auto liveRotations=std::array<std::array<float,4>,61>{};for(auto& q:liveRotations)q={1,0,0,0};WindowGestureMotion fullLive;fullLive.reach=fullLive.press=1;
 BlendWindowGesture(liveRotations,fullLive,*accepted);Check(liveRotations[5]==std::array<float,4>{1,0,0,0},"hot reloaded targets drive live gesture blend");

 LeftArmGestures gestures;
 gestures.Step(false,true,.01f);Check(gestures.window.reach>0 && gestures.window.reach<1 && gestures.window.press>0,"gesture forms while hand is moving");
 for(int i=0;i<100;++i)gestures.Step(false,true,.01f);
 Check(gestures.window.reach==1 && gestures.window.press==1,"held window gesture forms completely");
 auto windowPose=std::array<std::array<float,4>,61>{};for(auto& q:windowPose)q={1,0,0,0};auto untouched=windowPose;
 BlendWindowGesture(windowPose,gestures.window);
 for(unsigned i=0;i<61;++i){
  bool selected=false;for(const auto& target:kWindowGestureTargets)if(target.bone==i)selected=true;
  if(!selected)Check(windowPose[i]==untouched[i],"window gesture preserves torso right arm and pedals");
  float norm=0;for(float value:windowPose[i])norm+=value*value;Check(std::abs(norm-1)<.00001f,"window gesture rotation normalization");
 }
 for(int i=0;i<100;++i){gestures.Step(true,true,.01f);Check(!(gestures.horn.reach>0 && gestures.window.reach>0),"horn and window never compete for left arm");}
 Check(gestures.horn.reach==1 && gestures.horn.press==1 && gestures.window.reach==0,"horn takes priority over held gesture");
 for(int i=0;i<150;++i){gestures.Step(false,true,.01f);Check(!(gestures.horn.reach>0 && gestures.window.reach>0),"return to grip before resuming window gesture");}
 Check(gestures.horn.reach==0 && gestures.window.press==1,"held window gesture resumes after horn");
 for(int i=0;i<100;++i)gestures.Step(false,false,.01f);
 Check(gestures.window.reach==0 && gestures.window.press==0,"window key release returns to live steering grip");
 auto released=untouched;BlendWindowGesture(released,gestures.window);Check(released==untouched,"released window gesture is byte exact");

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
