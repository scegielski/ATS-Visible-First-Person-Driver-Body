#include "DriverSamples.hpp"
#include "SteeringPose.hpp"
#include "PedalMotion.hpp"
#include "HornMotion.hpp"
#include "HornPose.hpp"
#include "GameHorn.hpp"
#include "GameEngine.hpp"
#include "EngineStartGate.hpp"
#include "IgnitionMotion.hpp"
#include "IgnitionTargets.hpp"
#include "RuntimeArmIK.hpp"
#include "SignalMotion.hpp"
#include "SignalTargets.hpp"
#include "BodyVisibility.hpp"
#include "PlayerExteriorDriver.hpp"
#include <fstream>
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
void* gEngineOriginal=nullptr;
uintptr_t EngineRequestObserveStub(uintptr_t,uint32_t,uint32_t,uint64_t,uint64_t);
void* gAdvanceOriginal=nullptr;
void* gBoneOriginal=nullptr;
uintptr_t BoneObserveStub(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uint32_t);
uintptr_t DriverObserveStub(uintptr_t);
uintptr_t AdvanceObserveStub(uintptr_t,uint32_t,float,uint64_t,uint64_t);
}
static int observedDriver=0,observedAdvance=0,boneOriginalCalls=0,bonePostCalls=0;
static uintptr_t recordedReturn=0;
static bool replacePose=false,suppressEngine=false;static int engineOriginalCalls=0;
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
extern "C" bool CaptureEngineRequest(uintptr_t vehicle,uint32_t request,uint32_t immediate,uintptr_t caller) noexcept {
 Check(vehicle==0x12345678 && request==0xabcdef01 && immediate==0xfedcba00,"engine observer preserves full register arguments");Check(caller!=0,"engine gate receives original return address");return suppressEngine;
}
static uintptr_t EngineOriginal(uintptr_t vehicle,uint32_t request,uint32_t immediate,uint64_t fourth,uint64_t fifth){
 Check(vehicle==0x12345678 && request==0xabcdef01 && immediate==0xfedcba00 && fourth==0x11223344 && fifth==0x55667788,"engine original receives unchanged arguments including stack");++engineOriginalCalls;return 0x987654;
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
 Check(output==11 && bounds==22 && arrays==33 && bones==44 && count==61,"bone original arguments unchanged");++boneOriginalCalls;return 123456;
}
static double BoneFloatOriginal(uintptr_t output,uintptr_t bounds,uintptr_t arrays,uintptr_t bones,uint32_t count){BoneOriginal(output,bounds,arrays,bones,count);return 42.25;}
extern "C" void PostBone(uintptr_t output,uintptr_t bounds,uint32_t count) noexcept {
 Check(output==11 && bounds==22 && count==61,"post bone receives original output/model/count");
 Check(boneOriginalCalls==bonePostCalls+1,"visibility runs after original hierarchy assembly");++bonePostCalls;volatile double clobber=1.25;clobber=std::sin(clobber);
}
int main() {
 {
 EngineStartGate gate;DeferredEngineStart replay;Check(gate.Queue(100,1,1000,9),"engine delay queues once");
 Check(!gate.Queue(100,1,1010,10),"repeated request cannot restart delay");
 Check(gate.Poll(100,1,1100,true,0,0,replay)==EngineStartGate::None,"engine held off before hand arrives");
 Check(gate.Poll(100,1,1450,true,0,9,replay)==EngineStartGate::KeyReached && replay.vehicle==100,"key arrival releases original request once");
 Check(gate.Poll(100,1,1460,true,0,9,replay)==EngineStartGate::None,"no double release");
 gate.Queue(100,1,2000,10);Check(gate.Cancel() && gate.Poll(100,1,2500,true,0,10,replay)==EngineStartGate::None,"stop cancels queued start");
 gate.Queue(100,1,3000,11);Check(gate.Poll(100,2,3100,true,0,11,replay)==EngineStartGate::Cancelled,"world reload prevents replay");
 gate.Queue(100,1,3000,11);Check(gate.Poll(200,1,3100,true,0,11,replay)==EngineStartGate::Cancelled,"vehicle change prevents replay");
 gate.Queue(100,1,3000,11);Check(gate.Poll(100,1,5000,true,0,0,replay)==EngineStartGate::Timeout,"missing animation has bounded delay fallback");
 gate.Queue(100,1,6000,12);Check(gate.Poll(100,1,6100,false,0,0,replay)==EngineStartGate::Cancelled,"stale world does not start vehicle");
 Check(IsManualEngineStartCaller(0x100000,0x100000+0x9f79ec) && !IsManualEngineStartCaller(0x100000,0x100000+0x5db25f),"manual caller guard excludes automatic engine paths");
 }

 gEngineOriginal=reinterpret_cast<void*>(EngineOriginal);Check(EngineRequestObserveStub(0x12345678,0xabcdef01,0xfedcba00,0x11223344,0x55667788)==0x987654,"engine observer preserves original return");
 suppressEngine=true;Check(EngineRequestObserveStub(0x12345678,0xabcdef01,0xfedcba00,0x11223344,0x55667788)==0 && engineOriginalCalls==1,"deferred engine request skips original exactly once");suppressEngine=false;
 {
 constexpr uintptr_t base=0x100000,game=0x200000,owner=0x300000,vt=0x400000,vehicle=0x500000,model=0x600000;bool wrong=false;
 auto reader=[&](uintptr_t a,void* out,size_t n){uintptr_t value=0;if(a==base+0x36b8db0)value=game;else if(a==game+0xaa0)value=owner;else if(a==owner)value=vt;else if(a==vt+0x1b8)value=wrong?0:base+0x4d9c90;else if(a==owner+0x31b0)value=vehicle;else if(a==vehicle+0x1df8)value=model;else return false;if(n!=8)return false;std::memcpy(out,&value,8);return true;};
 uintptr_t controller=0,candidate=0;Check(ReadPlayerExteriorDriver(base,reader,controller,candidate) && controller==vehicle+0x1df0 && candidate==model,"player exterior identity follows guarded vehicle owner");
 Check(IsConfirmedPlayerDriver(controller,candidate,controller,candidate),"exact observed player driver matches");
 Check(!IsConfirmedPlayerDriver(vehicle+0x2c8,candidate,controller,candidate),"traffic driver using same asset rejected");
 Check(!IsConfirmedPlayerDriver(controller,model+8,controller,candidate),"different model instance rejected");
 wrong=true;Check(!ReadPlayerExteriorDriver(base,reader,controller,candidate) && !controller && !candidate,"wrong owner cannot nominate exterior actor");
 }

 {
 Check(!HideInteriorBody(true,2) && !HideInteriorBody(false,1),"interior camera and unknown camera retain body");
 for(int camera:{0,1,3,4,5,6,7,9})Check(HideInteriorBody(true,camera),"exterior camera hides private body");
 DriverPalette palette{};for(auto& matrix:palette){matrix[0]=matrix[5]=matrix[10]=matrix[15]=1;matrix[12]=1;matrix[13]=2;matrix[14]=3;}
 auto headOnly=palette;Check(CollapseDriverHead(headOnly),"interior head collapse remains finite");
 for(unsigned bone=0;bone<61;++bone)if(bone!=46 && bone!=47)Check(headOnly[bone]==palette[bone],"head hiding preserves body arms fingers and legs");
 Check(headOnly[46][0]==0 && headOnly[47][0]==0 && headOnly[46]==headOnly[47],"head and neck collapse at shared finite anchor");
 Check(CollapseDriverPalette(palette),"finite palette collapses safely");
 for(auto& matrix:palette)Check(matrix[0]==0 && matrix[5]==0 && matrix[10]==0 && matrix[12]==1 && matrix[13]==2 && matrix[14]==3 && matrix[15]==1,"every skinned vertex maps to same finite point");
 auto invalid=palette;invalid[4][2]=std::numeric_limits<float>::quiet_NaN();Check(!CollapseDriverPalette(invalid),"invalid palette rejected before writing");
 }

 {
 SignalSelection input;Check(input.Sample(true,false,false)==0,"loaded active signal does not replay");
 Check(input.Sample(false,false,false)==0,"signal off does not act as new turn");
 Check(input.Sample(true,false,false)==1,"left selection triggers once");
 for(int i=0;i<100;++i)Check(input.Sample(true,false,false)==0,"selected signal does not repeat with flashing time");
 Check(input.Sample(false,true,false)==2,"switch to right triggers right gesture");
 Check(input.Sample(true,true,true)==0 && input.Sample(true,false,true)==0,"hazards suppress stalk movement");
 Check(input.Sample(true,false,false)==0,"leaving hazards establishes baseline without new gesture");
 input.Reset();Check(input.Sample(false,false,false)==0 && input.Sample(false,true,false)==2,"world reset safely rearms");
 SignalMotion motion;motion.Start(1);
 for(int i=0;i<20;++i){motion.Step(true,false,.01f);Check(motion.press==0 && motion.Pose().extend==0,"stalk movement begins only after reach");}
 for(int i=0;i<100;++i)motion.Step(true,false,.01f);
 Check(motion.phase==SignalMotion::Idle && motion.reach==0,"signal gesture returns while indicator remains selected");
 motion.Start(2);motion.Step(true,false,.05f);motion.Step(false,false,.05f);for(int i=0;i<50;++i)motion.Step(false,false,.01f);Check(motion.reach==0,"stale signal input releases hand");
 }

 {
 std::ifstream source("../../stock-reference/ford_mustang_67_ani.pma",std::ios::binary);
 Check(static_cast<bool>(source),"runtime IK stock steering fixture opens");
 source.seekg(72);uint32_t offset=0;source.read(reinterpret_cast<char*>(&offset),4);
 for(unsigned frame=0;frame<47;++frame){
  DriverIK::Pose pose{};DriverIK::Positions positions{};
  for(unsigned bone=0;bone<61;++bone){float values[14]{};source.seekg(offset+(frame*61+bone)*56);source.read(reinterpret_cast<char*>(values),56);Check(static_cast<bool>(source),"runtime IK fixture read");for(unsigned i=0;i<4;++i)pose[bone][i]=values[4+i];for(unsigned i=0;i<3;++i){positions[bone][i]=values[8+i];Check(std::abs(values[11+i]-1.f)<.001f,"stock rig uses unit scale");}}
  for(unsigned mode=0;mode<3;++mode){
   auto candidate=pose;if(mode==1)BlendWindowGesture(candidate,{1,1,1},kRightGestureTargets);if(mode==2)BlendWindowGesture(candidate,{1,1,1},kIgnitionTargets);
   auto before=DriverIK::FK(candidate,positions);float error;
   Check(DriverIK::InsetRightElbow(candidate,positions,.04f,error),"right elbow inset solves steering gesture and ignition");
   auto after=DriverIK::FK(candidate,positions);
   Check(after.p[27][0]<=before.p[27][0]+.001f && DriverIK::Length(DriverIK::Sub(after.p[28],before.p[28]))<.001f,"right elbow moves left with hand target retained");
  }
  for(unsigned mode=0;mode<3;++mode){
   auto candidate=pose;
   if(mode==1)BlendHornPose(candidate,{1,1});
   if(mode==2)BlendWindowGesture(candidate,{1,1,1},kWindowGestureTargets);
   auto before=DriverIK::FK(candidate,positions);float error;
   Check(DriverIK::InsetLeftElbow(candidate,positions,.04f,error),"left elbow inset solves steering horn and gesture");
   auto after=DriverIK::FK(candidate,positions);
   Check(after.p[6][0]>=before.p[6][0]-.001f && DriverIK::Length(DriverIK::Sub(after.p[7],before.p[7]))<.001f,"left elbow moves inward with hand target retained");
  }
  for(unsigned side=0;side<2;++side){
   auto candidate=pose;BlendWindowGesture(candidate,{1,1,1},side==0?kSignalLeftTargets:kSignalRightTargets);auto before=DriverIK::FK(candidate,positions);float error;
   Check(DriverIK::InsetLeftElbow(candidate,positions,.06f,error),"signal pose supports inward elbow correction");
   auto after=DriverIK::FK(candidate,positions);Check(DriverIK::Length(DriverIK::Sub(after.p[7],before.p[7]))<.001f,"signal elbow retains stalk target");
   for(unsigned i=25;i<61;++i)Check(candidate[i]==pose[i],"signal leaves right arm and pedals unchanged");
  }
  for(unsigned turnStep=0;turnStep<=10;++turnStep){
   auto unturned=pose,turning=pose;BlendWindowGesture(unturned,{1,1,0},kIgnitionTargets);BlendWindowGesture(turning,{1,1,turnStep/10.f},kIgnitionTargets);
   auto base=DriverIK::FK(unturned,positions),before=DriverIK::FK(turning,positions);auto offset=DriverIK::Sub(base.p[43],before.p[43]);float degrees,error;
   Check(DriverIK::LeanAndCompensate(turning,positions,1,degrees,error,.1524f,offset),"thumb pivot IK solves throughout key turn");
   auto after=DriverIK::FK(turning,positions);
   Check(DriverIK::Length(DriverIK::Sub(after.p[43],DriverIK::Add(base.p[43],{0,0,-.1524f})))<.001f,"thumb pivot stays fixed throughout rotation");
   Check(DriverIK::Length(DriverIK::Sub(after.p[7],base.p[7]))<.001f,"thumb pivot preserves opposite steering grip");
  }
  for(unsigned ignitionStep=0;ignitionStep<=10;++ignitionStep){
   auto keyPose=pose;WindowGestureMotion motion{ignitionStep/10.f,1.f,.5f};BlendWindowGesture(keyPose,motion,kIgnitionTargets);
   auto before=DriverIK::FK(keyPose,positions);float degrees,error;
   Check(DriverIK::LeanAndCompensate(keyPose,positions,motion.reach,degrees,error,.1524f),"runtime IK solves ignition with live steering hand");
   auto after=DriverIK::FK(keyPose,positions);
   for(unsigned hand:{7u,28u}){
    Check(DriverIK::Length(DriverIK::Sub(after.p[hand],DriverIK::Add(before.p[hand],hand==28?DriverIK::V{0,0,-.1524f*motion.reach}:DriverIK::V{0,0,0})))<.001f,"ignition IK retains left grip and extends right key target");
    float dot=0;for(unsigned c=0;c<4;++c)dot+=after.q[hand][c]*before.q[hand][c];Check(std::abs(dot)>.9999f,"runtime IK retains grip orientation");
   }
  }
  for(unsigned step=0;step<=10;++step){
   auto candidate=pose;auto before=DriverIK::FK(candidate,positions);float degrees=0,error=0;
   Check(DriverIK::LeanAndCompensate(candidate,positions,step/10.f,degrees,error),"runtime IK solves full steering range throughout lean");
   auto after=DriverIK::FK(candidate,positions);
   Check(DriverIK::Length(DriverIK::Sub(after.p[7],before.p[7]))<.001f && DriverIK::Length(DriverIK::Sub(after.p[28],before.p[28]))<.001f,"both wrist targets preserved");
   for(unsigned i=0;i<61;++i)if(i!=1 && i!=2 && i!=3 && i!=5 && i!=6 && i!=7 && i!=26 && i!=27 && i!=28)Check(candidate[i]==pose[i],"IK leaves hips fingers legs and other rotations unchanged");
  }
 }
 auto pose=kDriverReferenceRotations;auto badPositions=kDriverLocalPositions;badPositions[7][0]=std::numeric_limits<float>::quiet_NaN();float degrees,error;
 Check(!DriverIK::LeanAndCompensate(pose,badPositions,1,degrees,error) && pose==kDriverReferenceRotations,"invalid runtime IK input leaves pose unchanged");
 auto before=pose;Check(!DriverIK::Solve(pose,kDriverLocalPositions,5,6,7,{100,100,100},{1,0,0,0},{0,1,0}) && pose==before,"unreachable IK target fails without mutation");
 }

 {
 IgnitionMotion key;key.Start();
 for(int i=0;i<40;++i){key.Step(true,true,.01f);Check(key.turn==0,"ignition turns only after reaching key");}
 for(int i=0;i<40;++i)key.Step(true,true,.01f);
 Check(key.reach==1 && key.turn==1,"ignition reaches and turns");
 for(int i=0;i<100;++i)key.Step(false,true,.01f);
 Check(key.reach==0 && key.turn==0 && !key.active,"ignition returns after starting ends");
 key.Start();for(int i=0;i<200;++i)key.Step(false,true,.01f);
 Check(key.reach==0 && !key.active,"short starting pulse completes and returns");
 key.Start();for(int i=0;i<700;++i)key.Step(true,true,.01f);
 Check(key.reach==0 && !key.active,"stuck starting signal has bounded animation");
 key.Start();key.Step(true,true,.01f);for(int i=0;i<100;++i)key.Step(false,false,.01f);
 Check(key.reach==0,"missing engine data returns to wheel");
 }

 {
 constexpr uintptr_t b=0x100000,g=0x200000,o=0x300000,v=0x400000,t=0x500000;
 uint32_t engine=0;bool badOwner=false,missing=false;
 auto readEngine=[&](uintptr_t a,void* out,size_t n){
  uintptr_t value=0;
  if(a==b+0x36b8db0)value=g;
  else if(a==g+0xaa0)value=o;
  else if(a==o)value=t;
  else if(a==t+0x1b8)value=badOwner?0:b+0x4d9c90;
  else if(a==o+0x31b0)value=missing?0:v;
  else if(a==v+0x1b8 && n==4){std::memcpy(out,&engine,4);return true;}
  else return false;
  if(n!=8)return false;std::memcpy(out,&value,8);return true;
 };
 PlayerEngineState state{};
 for(engine=0;engine<=3;++engine)Check(ReadPlayerEngine(b,readEngine,state) && static_cast<uint32_t>(state)==engine,"engine query maps off/start/run/stop");
 Check(!ReadPlayerEngine(b,readEngine,state),"invalid engine enum rejected");engine=1;
 badOwner=true;Check(!ReadPlayerEngine(b,readEngine,state),"engine wrong owner rejected");badOwner=false;
 missing=true;Check(!ReadPlayerEngine(b,readEngine,state),"engine missing vehicle rejected");
 }

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
 gBoneOriginal=reinterpret_cast<void*>(BoneFloatOriginal);
 using FloatBoneCall=double(*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uint32_t);
 Check(reinterpret_cast<FloatBoneCall>(&BoneObserveStub)(11,22,33,44,61)==42.25,"post hook preserves floating return despite callback clobber");
 gBoneOriginal=reinterpret_cast<void*>(BoneOriginal);
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
