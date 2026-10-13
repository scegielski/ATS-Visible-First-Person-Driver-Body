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
#include "SignalRequestGate.hpp"
#include "SignalReplayGuard.hpp"
#include "SignalTiming.hpp"
#include "HeadTracking.hpp"
#include "HeadTorso.hpp"
#include "GestureHeadFollow.hpp"
#include "ControllerIK.hpp"
#include "ControllerActionAttach.hpp"
#include "NativeRecenter.hpp"
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
void* gHeadOriginal=nullptr;
void* gXRActionAttachOriginal=nullptr;
void* gRecenterOriginal=nullptr;
void* gSignalLeftOriginal=nullptr;void* gSignalRightOriginal=nullptr;
uintptr_t EngineRequestObserveStub(uintptr_t,uint32_t,uint32_t,uint64_t,uint64_t);
void* gAdvanceOriginal=nullptr;
void* gBoneOriginal=nullptr;
uintptr_t BoneObserveStub(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uint32_t);
uintptr_t DriverObserveStub(uintptr_t);
uintptr_t AdvanceObserveStub(uintptr_t,uint32_t,float,uint64_t,uint64_t);
}
static int observedDriver=0,observedAdvance=0,boneOriginalCalls=0,bonePostCalls=0;
static uintptr_t recordedReturn=0;
extern "C" uint64_t SignalTestLeft(uintptr_t,uint64_t,uint64_t,uint64_t*);
extern "C" uint64_t SignalTestRight(uintptr_t,uint64_t,uint64_t,uint64_t*);
extern "C" void SignalTestContinuation();
static unsigned char signalReplacement=0;static unsigned signalTestSide=0;
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
extern "C" unsigned char CaptureSignalRequest(uintptr_t vehicle,uint32_t side,uint32_t input) noexcept {Check(vehicle==123 && side==signalTestSide && input==0x55667701,"signal mid-function callback arguments");return signalReplacement;}
extern "C" uint64_t RecenterTestEntry(uintptr_t,uint64_t);
extern "C" void RecenterTestContinuation();
extern "C" void CaptureNativeRecenter(uintptr_t owner,uint32_t request) noexcept {Check(owner==123 && request==0x55667701,"native recenter observer callback arguments");}
extern "C" uintptr_t HeadTestEntry(uintptr_t,uint32_t,uint64_t,int32_t);
extern "C" void HeadTestContinuation();
extern "C" void CaptureHeadViews(uintptr_t views,uint32_t count,uint64_t flags,int32_t result,uintptr_t locate) noexcept {
 const auto* info=reinterpret_cast<const ObservedXRLocate*>(locate);
 Check(views==123 && count==2 && flags==15 && result==-3,"head observer reads native output registers and frame offsets");
 Check(info->type==6 && info->displayTime==42 && info->space==456,"head observer forwards locate info as fifth callback argument");
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
extern "C" void CaptureXRContext(uintptr_t) noexcept {}
extern "C" uintptr_t PrepareControllerActionAttach(uintptr_t owner,uintptr_t info) noexcept {Check(owner==123 && info==456,"XR attach observer receives owner and original info");return 789;}
extern "C" uintptr_t XRActionAttachTest(uintptr_t,uintptr_t);
extern "C" void XRActionAttachContinuation();
int main() {
 gXRActionAttachOriginal=reinterpret_cast<void*>(XRActionAttachContinuation);Check(XRActionAttachTest(123,456)==789,"XR attachment observer replaces only input pointer and preserves continuation");
 std::array<XrActionSet,17> attached{};XrActionSet originalAction=reinterpret_cast<XrActionSet>(1), extraAction=reinterpret_cast<XrActionSet>(2);XrSessionActionSetsAttachInfo originalInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};originalInfo.countActionSets=1;originalInfo.actionSets=&originalAction;XrSessionActionSetsAttachInfo combinedInfo{};
 Check(CombineControllerActionSets(originalInfo,extraAction,combinedInfo,attached) && combinedInfo.countActionSets==2 && attached[0]==originalAction && attached[1]==extraAction,"native controller attachment retains ATS action sets");
 originalInfo.countActionSets=17;Check(!CombineControllerActionSets(originalInfo,extraAction,combinedInfo,attached),"unexpected attachment layout rejected");

 ControllerElbowGuides guides;Check(ParseControllerElbowGuides("left -.12 -.35 .12\nright .12 -.35 .12",guides),"live controller elbow guides parse");Check(!ParseControllerElbowGuides("left 0 0 0 right 1 0 0",guides),"zero elbow guide rejected without replacing valid settings");
 DriverIK::V priorRay{0,0,-1},priorBend{0,-1,0};
 for(unsigned step=0;step<200;++step){float t=step/199.f;auto ray=DriverIK::Unit(DriverIK::V{.3f*std::sin(t*6.28f),-.4f+.8f*t,-1});auto bend=StableControllerBend(ray,{0,-1,0},priorRay,priorBend,1.f/90.f);auto transported=DriverIK::Unit(DriverIK::Rotate(DriverIK::Between(priorRay,ray),priorBend));Check(DriverIK::Dot(bend,transported)>.998f,"controller elbow pole cannot flip between continuous frames");Check(std::abs(DriverIK::Dot(bend,ray))<.0001f,"stable elbow guide remains perpendicular to reach");priorRay=ray;priorBend=bend;}
 ControllerHand button;button.Button(true,100,true);Check(button.enabled && button.calibrate,"controller first thumbstick press enables calibration");button.Button(true,200,true);Check(button.enabled,"held click does not toggle repeatedly");button.Button(false,300,true);button.Button(true,400,true);Check(!button.enabled,"second click disables independently");button.Button(true,1200,true);Check(button.enabled && button.calibrate,"long hold recalibrates and enables");

 gRecenterOriginal=reinterpret_cast<void*>(RecenterTestContinuation);Check(RecenterTestEntry(123,0x1122334455667701ull)==0x1122334455667701ull,"recenter observer leaves native edge register unchanged");
 Check(GestureLookPitch({.9659258f,.258819f,0,0})>.5f && GestureLookPitch({.9659258f,-.258819f,0,0})<-.5f,"upward and downward head pitch signs follow tracked look");
 NativeRecenterGate nativeCalibration;nativeCalibration.Observe(1,100);
 Check(!nativeCalibration.Consume(100,100,true),"recenter waits beyond pre-request pose");
 Check(!nativeCalibration.Consume(120,110,false),"invalid tracking cannot recalibrate");
 Check(nativeCalibration.Consume(130,120,true) && !nativeCalibration.Consume(140,130,true),"fresh post-request sample recalibrates once");
 nativeCalibration.Observe(2,200);Check(!nativeCalibration.Consume(2300,2290,true) && !nativeCalibration.pending,"stale reset request expires");

 gHeadOriginal=reinterpret_cast<void*>(HeadTestContinuation);Check(HeadTestEntry(123,2,15,-3)==static_cast<uint32_t>(-3),"head observer preserves OpenXR result for game continuation");
 std::array<ObservedXRView,2> xrViews{};for(auto& view:xrViews){view.type=7;view.orientation={0,0,0,1};view.position={0,1.6f,-.2f};}
 xrViews[0].position[0]=-.032f;xrViews[1].position[0]=.032f;ObservedXRLocate xrInfo{};xrInfo.type=6;xrInfo.displayTime=1;xrInfo.space=123;ObservedHead xrHead;
 Check(HeadFromStereo(xrViews,15,xrInfo,xrHead) && std::abs(xrHead.position[0])<1e-6 && xrHead.position[1]==1.6f,"stereo eye midpoint yields head position");
 xrViews[1].orientation={0,0,0,-1};Check(HeadFromStereo(xrViews,3,xrInfo,xrHead) && std::abs(xrHead.orientation[3]-1)<1e-6,"opposite quaternion signs average correctly");
 Check(!HeadFromStereo(xrViews,1,xrInfo,xrHead),"untracked position is rejected");
 xrViews[1].position[0]=1;Check(!HeadFromStereo(xrViews,3,xrInfo,xrHead),"implausible stereo separation rejected");


 uint64_t signalSaved[3]{};gSignalLeftOriginal=gSignalRightOriginal=reinterpret_cast<void*>(SignalTestContinuation);
 signalTestSide=1;signalReplacement=0;
 Check(SignalTestLeft(123,0x1122334455667701ull,0xabcdef,signalSaved)==0x1122334455667700ull,"left signal observer alters only CL");
 Check(signalSaved[0]==123 && signalSaved[1]==0x1122334455667701ull && signalSaved[2]==0xabcdef,"signal observer preserves vehicle and other registers");
 signalTestSide=2;signalReplacement=1;
 Check(SignalTestRight(123,0x1122334455667701ull,0xabcdef,signalSaved)==0x1122334455667701ull,"right signal observer preserves native edge");
 Check(KeepSignalReplay(1,100,110,0,false),"old telemetry sample must retain replay suppression");
 Check(KeepSignalReplay(1,100,120,1,false),"matching delayed selection consumes suppression");
 Check(!KeepSignalReplay(1,100,120,2,false),"opposite signal is not suppressed");
 Check(!KeepSignalReplay(1,100,601,0,false),"unacknowledged replay suppression expires");
 Check(!KeepSignalReplay(1,100,120,0,true),"hazards clear replay suppression");
 float liveDelay=.22f;
 Check(ParseSignalDelay("0.10\n",liveDelay) && liveDelay==.10f,"live signal timing accepts seconds");
 Check(!ParseSignalDelay("nan",liveDelay) && liveDelay==.10f,"bad timing retains last valid value");
 Check(!ParseSignalDelay("-0.1",liveDelay) && !ParseSignalDelay("0.46",liveDelay) && !ParseSignalDelay("0.2 garbage",liveDelay),"timing rejects unsafe range and incomplete edits");
 SignalMotion arrivalSignal;arrivalSignal.Start(1);
 float arrivalTime=0;while(arrivalSignal.phase==SignalMotion::Reach){arrivalSignal.Step(true,false,.01f);arrivalTime+=.01f;}
 Check(arrivalSignal.reach==1 && arrivalSignal.press==0 && arrivalTime<.24f,"signal contact occurs before stalk press rather than after it");
 float actuation=.16f;
 Check(ParseSignalActuation("0.16",actuation) && !ParseSignalActuation("0",actuation) && !ParseSignalActuation("1",actuation),"live stalk duration validates range");
 SignalMotion normalStalk,slowStalk;normalStalk.Start(1);slowStalk.Start(1);
 for(int i=0;i<23;++i){normalStalk.Step(true,false,.01f,.12f);slowStalk.Step(true,false,.01f,.16f);}
 Check(normalStalk.reach==slowStalk.reach && normalStalk.phase==SignalMotion::Actuate && slowStalk.phase==SignalMotion::Actuate,"stalk slowdown preserves reach timing");
 for(int i=0;i<8;++i){normalStalk.Step(true,false,.01f,.12f);slowStalk.Step(true,false,.01f,.16f);}
 Check(slowStalk.press<normalStalk.press && slowStalk.reach==1,"only stalk actuation moves more slowly");
 SeatedHeadCalibration calibration;ObservedHead seated;seated.space=10;seated.orientation={0,0,0,1};seated.flags=15;seated.position={0,1.6f,0};
 Check(calibration.Step(seated,1,100,true,false,.01f,.65f),"initial seated head calibration");
 seated.position[0]=.2f;calibration.Step(seated,1,110,true,false,.05f,.65f);Check(calibration.filtered.delta[0]>0 && calibration.filtered.delta[0]<.12f,"head translation gain clamp and smoothing");
 Check(calibration.Step(seated,1,120,true,true,.01f,.65f) && DriverIK::Length(calibration.filtered.delta)==0,"recenter zeroes neutral offset");
 seated.space=11;Check(calibration.Step(seated,1,130,true,false,.01f,.65f),"reference-space change recalibrates");
 SeatedHeadCalibration exteriorCalibration;ObservedHead exteriorHead;exteriorHead.space=22;exteriorHead.orientation={0,0,0,1};exteriorHead.flags=15;
 Check(!exteriorCalibration.Step(exteriorHead,1,100,true,false,.05f,1,.3f,.24f,false) && !exteriorCalibration.baseline.space,"exterior startup waits for seated baseline");
 Check(exteriorCalibration.Step(exteriorHead,1,110,true,false,.05f,1,.3f,.24f,true),"interior establishes seated baseline");
 exteriorHead.position[2]=-.2f;
 Check(!exteriorCalibration.Step(exteriorHead,1,120,true,false,.05f,1,.3f,.24f,false) && exteriorCalibration.filtered.delta[2]<0,"external view retains neutral and tracks lean");
 exteriorHead.space=23;exteriorCalibration.Step(exteriorHead,1,130,true,false,.05f,1,.3f,.24f,false);Check(!exteriorCalibration.baseline.space,"changed tracking space cannot reuse exterior baseline");
 Check(exteriorCalibration.Step(exteriorHead,1,140,true,true,.05f,1,.3f,.24f,false),"explicit recenter available in exterior view");
 HeadTrackingLimits largeForward;
 Check(ParseHeadTrackingLimits("forward_m 0.70 max_lean_degrees 40 lateral_m 0.30",largeForward),"larger forward range and lean cap accepted");
 Check(ParseHeadTrackingLimits("forward_m 1000 max_lean_degrees 1000 lateral_m 1000",largeForward),"live range settings have no arbitrary upper cap");
 HeadTrackingLimits lateralLimits;
 Check(ParseHeadTrackingLimits("forward_m 0.30 max_lean_degrees 28 lateral_m 0.24",lateralLimits) && lateralLimits.lateral==.24f,"live sideways limit accepted");
 Check(!ParseHeadTrackingLimits("forward_m 0.30 max_lean_degrees 28 lateral_m -1",lateralLimits),"negative sideways limit rejected");
 HeadTrackingLimits limits;
 Check(ParseHeadTrackingLimits("forward_m 0.30\nmax_lean_degrees 28\n",limits) && limits.forward==.30f && limits.degrees==28,"live head limits parse");
 Check(!ParseHeadTrackingLimits("forward_m -1 max_lean_degrees 28",limits) && !ParseHeadTrackingLimits("forward_m 0.3 max_lean_degrees nan",limits),"invalid head limits retain accepted setting");
 SignalRequestGate sg;
 Check(sg.Step(123,1,100,true,0,1,true,0,10)==1,"left signal queued");
 Check(sg.Step(123,1,200,true,0,1,false,0,11)==0,"no signal before hand arrival");
 Check(sg.Step(123,1,450,true,0,1,false,10,11)==3 && !sg.vehicle,"native toggle released exactly once at contact");
 Check(sg.Step(123,1,500,true,0,1,false,10,11)==0,"no duplicate replay");
 sg.Step(123,1,600,true,0,1,true,0,11);Check(sg.Step(123,1,620,true,0,1,true,0,12)==2,"second left press cancels pending signal");
 sg.Step(123,1,700,true,0,1,true,0,12);Check(sg.Step(123,1,710,true,0,2,true,0,13)==1 && sg.direction==2,"opposite direction replaces pending signal");
 Check(sg.Step(123,1,2800,true,0,2,false,0,14)==4,"signal timeout falls back to native toggle");
 sg.Step(123,1,3000,true,0,1,true,0,14);Check(sg.Step(123,1,3020,true,16,1,false,0,15)==0 && !sg.vehicle,"hazards cancel pending toggle");
 Check(sg.Step(123,1,3030,true,4,1,true,0,15)==0,"already-selected signal cancels immediately");
 Check(sg.Step(123,1,3040,false,0,1,true,0,15)==0,"stale body leaves native action intact");
 sg.Step(123,1,3100,true,0,1,true,0,15);sg.Step(124,1,3120,true,0,1,false,0,16);Check(!sg.vehicle,"vehicle switch cannot replay old signal");
 sg.Step(123,1,3200,true,0,1,true,0,16);sg.Step(123,2,3220,true,0,1,false,0,17);Check(!sg.vehicle,"world reset cannot replay old signal");

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
 DriverPalette palette{};for(auto& matrix:palette){matrix[0]=matrix[5]=matrix[10]=matrix[15]=1;matrix[3]=1;matrix[7]=2;matrix[11]=3;}
 auto headOnly=palette;Check(CollapseDriverHead(headOnly),"interior head collapse remains finite");
 for(unsigned bone=0;bone<61;++bone)if(bone!=46 && bone!=47)Check(headOnly[bone]==palette[bone],"head hiding preserves body arms fingers and legs");
 Check(headOnly[46][0]==0 && headOnly[47][0]==0 && headOnly[46]==headOnly[47],"head and neck collapse at shared finite anchor");
 auto visibleHead=palette;Check(OffsetDriverHead(visibleHead,0) && visibleHead==palette,"visible interior head retains complete normal palette at default position");
 Check(OffsetDriverHead(visibleHead,.08f),"intact head rearward offset is finite");
 for(unsigned bone=0;bone<61;++bone)for(unsigned element=0;element<16;++element){if((bone==46 || bone==47) && element==11)Check(std::abs(visibleHead[bone][element]-palette[bone][element]-.08f)<1e-6f,"intact head moves rearward");else Check(visibleHead[bone][element]==palette[bone][element],"intact head keeps rotation scale and all other bones");}
 auto forwardHead=palette;Check(OffsetDriverHead(forwardHead,-.20f),"signed head offset supports opposite direction");Check(std::abs(forwardHead[47][11]-palette[47][11]+.20f)<1e-6f,"opposite offset moves head forwards");
 auto rearHead=palette;Check(CollapseDriverHead(rearHead,.08f),"interior head rearward adjustment is finite");
 Check(std::abs(rearHead[47][11]-headOnly[47][11]-.08f)<1e-6f && rearHead[47][3]==headOnly[47][3] && rearHead[47][7]==headOnly[47][7],"rearward shift changes Z without lowering Y");
 for(unsigned bone=0;bone<61;++bone)if(bone!=46 && bone!=47)Check(rearHead[bone]==palette[bone],"rearward hiding does not change torso arms or legs");
 auto rejectRear=palette;Check(!CollapseDriverHead(rejectRear,std::numeric_limits<float>::quiet_NaN()) && rejectRear==palette,"invalid backshift leaves palette unchanged");
 auto rotatedRoot=palette;rotatedRoot[0][0]=0;rotatedRoot[0][2]=-1;rotatedRoot[0][8]=1;rotatedRoot[0][10]=0;auto noShift=rotatedRoot;
 Check(CollapseDriverHead(noShift) && CollapseDriverHead(rotatedRoot,.08f),"model-space rearward shift with rotated root");
 Check(std::abs(rotatedRoot[47][11]-noShift[47][11]-.08f)<1e-6f && rotatedRoot[47][3]==noShift[47][3] && rotatedRoot[47][7]==noShift[47][7],"head shift uses model Z independently of root rotation");
 auto visibleRotated=palette;visibleRotated[0]=rotatedRoot[0];auto originalVisible=visibleRotated;
 Check(OffsetDriverHead(visibleRotated,-.20f),"intact head model-space forward shift with rotated root");
 Check(std::abs(visibleRotated[47][11]-originalVisible[47][11]+.20f)<1e-6f && visibleRotated[47][3]==originalVisible[47][3] && visibleRotated[47][7]==originalVisible[47][7],"rotation cannot convert forward offset into downward or sideways motion");
 Check(CollapseDriverPalette(palette),"finite palette collapses safely");
 for(auto& matrix:palette)Check(matrix[0]==0 && matrix[5]==0 && matrix[10]==0 && matrix[3]==1 && matrix[7]==2 && matrix[11]==3 && matrix[15]==1,"every skinned vertex maps to same finite point");
 auto invalid=palette;invalid[4][2]=std::numeric_limits<float>::quiet_NaN();Check(!CollapseDriverPalette(invalid),"invalid palette rejected before writing");
 auto worldNeck=originalVisible;worldNeck[46]={0,0,1,.30f,0,1,0,1.50f,-1,0,0,-.10f,0,0,0,1};
 Check(CollapseDriverHead(worldNeck),"row-major neck origin collapse");
 Check(worldNeck[47][3]==.30f && worldNeck[47][7]==1.50f && worldNeck[47][11]==-.10f && worldNeck[47][12]==0 && worldNeck[47][13]==0 && worldNeck[47][14]==0 && worldNeck[47][15]==1,"collapse retains actual neck origin and homogeneous row");
 std::array<float,16> inverseBind{0,0,-1,2,0,1,0,-1.5f,1,0,0,3,0,0,0,1},skinned{};
 for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)for(unsigned k=0;k<4;++k)skinned[row*4+col]+=worldNeck[47][row*4+k]*inverseBind[k*4+col];
 for(std::array<float,4> vertex:{std::array<float,4>{0,0,0,1},std::array<float,4>{2,1.6f,-4,1}}){std::array<float,4> result{};for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)result[row]+=skinned[row*4+col]*vertex[col];Check(result[0]==.30f && result[1]==1.50f && result[2]==-.10f && result[3]==1,"inverse bind leaves collapsed vertices at neck rather than zero origin");}

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

  for(bool right:{false,true}){
   unsigned lower=right?27:6,wrist=lower+1;ControllerHand twistState;twistState.neutralWrist=pose[wrist];auto axis=DriverIK::Unit(positions[wrist]);DriverIK::Q previousForearm=pose[lower];
   for(unsigned degrees=0;degrees<=200;degrees+=5){
    float angle=degrees*.0174532925f;DriverIK::Q turn{std::cos(angle/2),axis[0]*std::sin(angle/2),axis[1]*std::sin(angle/2),axis[2]*std::sin(angle/2)};auto candidate=pose;candidate[wrist]=DriverIK::Product(turn,pose[wrist]);auto expected=DriverIK::FK(candidate,positions);
    Check(ShareControllerWristTwist(candidate,positions,lower,wrist,twistState,.5f),"forearm and wrist share axial roll");auto result=DriverIK::FK(candidate,positions);
    Check(DriverIK::Length(DriverIK::Sub(result.p[wrist],expected.p[wrist]))<.00001f,"twist share keeps wrist position fixed");float dot=0;for(unsigned i=0;i<4;++i)dot+=result.q[wrist][i]*expected.q[wrist][i];Check(std::abs(dot)>.99999f,"twist share keeps final hand orientation");
    DriverIK::Q half{std::cos(angle/4),axis[0]*std::sin(angle/4),axis[1]*std::sin(angle/4),axis[2]*std::sin(angle/4)};auto remaining=DriverIK::Product(candidate[wrist],DriverIK::Inverse(pose[wrist]));float residualDot=0;for(unsigned i=0;i<4;++i)residualDot+=remaining[i]*half[i];Check(std::abs(residualDot)>.99999f,"wrist carries exactly half of axial twist");
    Check(std::abs(twistState.previousTwist-angle)<.0001f,"twist angle remains continuous through 180 degrees");float continuity=0;for(unsigned i=0;i<4;++i)continuity+=candidate[lower][i]*previousForearm[i];Check(std::abs(continuity)>.999f,"forearm roll does not jump at quaternion wrap");previousForearm=candidate[lower];
    for(unsigned bone=0;bone<61;++bone)if(bone!=lower && bone!=wrist)Check(candidate[bone]==pose[bone],"axial share changes only forearm and wrist local rotations");
   }
  }
  for(bool right:{false,true}){
   auto candidate=pose;auto before=DriverIK::FK(pose,positions);unsigned wrist=right?28:7;ControllerHand hand;hand.enabled=hand.calibrate=true;
   Check(ApplyControllerHand(candidate,positions,right,hand,{0,0,0},{1,0,0,0},{1,0,0,0},1),"controller calibration solves at current grip");
   auto calibrated=DriverIK::FK(candidate,positions);Check(DriverIK::Length(DriverIK::Sub(calibrated.p[wrist],before.p[wrist]))<.001f,"calibration preserves grip without jump");
   Check(ApplyControllerHand(candidate,positions,right,hand,{0,.01f,0},{.9961947f,0,.0871557f,0},{1,0,0,0},1),"controller live translation and rotation solve");
   auto moved=DriverIK::FK(candidate,positions);Check(DriverIK::Length(DriverIK::Sub(moved.p[wrist],DriverIK::Add(before.p[wrist],{0,.01f,0})))<.001f,"controller wrist follows calibrated displacement");
   for(unsigned bone=0;bone<61;++bone)if(bone!=(right?26:5) && bone!=(right?27:6) && bone!=wrist)Check(candidate[bone]==pose[bone],"controller preserves opposite arm fingers legs torso");
   Check(ApplyControllerHand(candidate,positions,right,hand,{0,0,-10},{1,0,0,0},{1,0,0,0},1),"controller unreachable target clamps to arm envelope");
   auto upper=right?26u:5u;float armLength=DriverIK::Length(positions[upper+1])+DriverIK::Length(positions[wrist]);
   for(unsigned step=0;step<60;++step){
    float t=step/59.f;auto direction=DriverIK::Unit(DriverIK::Add(DriverIK::Sub(before.p[wrist],before.p[upper]),{.16f*std::sin(t*6.28f),.15f*std::sin(t*3.14f),0}));auto target=DriverIK::Add(before.p[upper],DriverIK::Mul(direction,armLength*.92f));auto point=DriverIK::Sub(target,hand.anchor);candidate=pose;
    Check(ApplyControllerHand(candidate,positions,right,hand,point,{1,0,0,0},{1,0,0,0},1),"controller stable elbow solves continuous cross-body and extended sweep");auto solved=DriverIK::FK(candidate,positions);
    Check(DriverIK::Length(DriverIK::Sub(solved.p[wrist],target))<.001f,"stable elbow retains controller wrist target throughout sweep");float orientationDot=0;for(unsigned j=0;j<4;++j)orientationDot+=solved.q[wrist][j]*before.q[wrist][j];Check(std::abs(orientationDot)>.9999f,"elbow stabilization retains hand orientation");
   }

  }

  for(DriverIK::V delta: {DriverIK::V{.08f,0,0},DriverIK::V{-.08f,0,0},DriverIK::V{0,0,-.10f},DriverIK::V{0,0,.04f}}){
   auto candidate=pose;auto before=DriverIK::FK(candidate,positions);float strength,error;
   Check(DriverIK::TrackHeadTorso(candidate,positions,delta,.08f,strength,error),"head torso compensation solves across steering range");
   auto after=DriverIK::FK(candidate,positions);
   Check(DriverIK::Length(DriverIK::Sub(after.p[7],before.p[7]))<.001f && DriverIK::Length(DriverIK::Sub(after.p[28],before.p[28]))<.001f,"both wrists retain live steering positions");
   for(unsigned bone=0;bone<61;++bone)if(bone!=1 && bone!=2 && bone!=3 && bone!=5 && bone!=6 && bone!=7 && bone!=26 && bone!=27 && bone!=28)Check(candidate[bone]==pose[bone],"head tracking preserves hips legs fingers and other joints");
   for(unsigned bone:{7u,28u})Check(std::abs(DriverIK::Dot({after.q[bone][1],after.q[bone][2],after.q[bone][3]},{before.q[bone][1],before.q[bone][2],before.q[bone][3]})+after.q[bone][0]*before.q[bone][0])>.9999f,"head compensation preserves hand orientation");
  }
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
  {
   auto expanded=pose;auto before=DriverIK::FK(expanded,positions);float strength,error;
   Check(DriverIK::TrackHeadTorso(expanded,positions,{0,0,-.30f},0,strength,error,28),"expanded forward torso range solves through steering");
   auto after=DriverIK::FK(expanded,positions);
   for(unsigned hand:{7u,28u})Check(DriverIK::Length(DriverIK::Sub(after.p[hand],before.p[hand]))<.001f,"expanded torso range keeps hand targets fixed");
  }
  for(float side:{-.24f,.24f}){
   auto wide=pose;auto before=DriverIK::FK(wide,positions);float strength,error;
   Check(DriverIK::TrackHeadTorso(wide,positions,{side,0,0},0,strength,error,28),"expanded sideways lean solves across steering range");
   auto after=DriverIK::FK(wide,positions);for(unsigned hand:{7u,28u})Check(DriverIK::Length(DriverIK::Sub(after.p[hand],before.p[hand]))<.001f,"expanded sideways range preserves hand targets");
  }
  for(DriverIK::Q turn:{DriverIK::Q{.9659258f,0,.258819f,0},DriverIK::Q{.9659258f,.258819f,0,0},DriverIK::Q{.9659258f,0,0,.258819f}}){
   auto candidate=pose;auto originalHead=DriverIK::FK(candidate,positions).q[47];float strength,error;
   Check(DriverIK::TrackHeadTorso(candidate,positions,{.06f,0,-.12f},.08f,strength,error,28),"torso fixture for exterior head");auto before=candidate;
   Check(DriverIK::ApplyTrackedHead(candidate,positions,turn,originalHead),"exterior neck and head tracking applies");
   auto after=DriverIK::FK(candidate,positions);auto expected=DriverIK::Product(turn,originalHead);float dot=0;for(unsigned j=0;j<4;++j)dot+=after.q[47][j]*expected[j];Check(std::abs(dot)>.9999f,"head world rotation follows HMD without duplicate torso rotation");
   for(unsigned bone=0;bone<61;++bone)if(bone!=46 && bone!=47)Check(candidate[bone]==before[bone],"head animation preserves arms fingers torso and legs");
  }
  {
   auto large=pose;auto before=DriverIK::FK(large,positions);float strength,error;
   Check(DriverIK::TrackHeadTorso(large,positions,{0,0,-.70f},0,strength,error,40),"large forward range solves across steering");
   auto after=DriverIK::FK(large,positions);for(unsigned hand:{7u,28u})Check(DriverIK::Length(DriverIK::Sub(after.p[hand],before.p[hand]))<.001f,"large forward lean preserves both hand targets");
  }
  for(float side:{-.6f,.6f}){
   auto candidate=pose;auto before=DriverIK::FK(candidate,positions);float strength,error;
   Check(DriverIK::TrackHeadTorso(candidate,positions,{side,0,0},0,strength,error,40),"larger lateral range accepted without arbitrary solver cap");
   auto after=DriverIK::FK(candidate,positions);for(unsigned hand:{7u,28u})Check(DriverIK::Length(DriverIK::Sub(after.p[hand],before.p[hand]))<.001f,"large lateral lean retains both wrist targets");
  }
  for(bool right:{false,true})for(float yaw:{-1.f,0.f,1.f}){
   auto candidate=pose;BlendWindowGesture(candidate,{1,1,1},right?kRightGestureTargets:kWindowGestureTargets);auto original=candidate;auto before=DriverIK::FK(candidate,positions);unsigned upper=right?26:5,lower=right?27:6,hand=right?28:7;
   Check(FollowGestureHead(candidate,positions,right,1,yaw,1),"gesture follows calibrated look yaw at shoulder");
   auto after=DriverIK::FK(candidate,positions);DriverIK::Q turn{std::cos(yaw/2),0,std::sin(yaw/2),0};auto target=DriverIK::Add(before.p[upper],DriverIK::Rotate(turn,DriverIK::Sub(before.p[hand],before.p[upper])));
   Check(DriverIK::Length(DriverIK::Sub(after.p[hand],target))<.001f && DriverIK::Length(DriverIK::Sub(after.p[upper],before.p[upper]))<.001f,"gesture wrist rotates around stationary shoulder");
   Check(std::abs(DriverIK::Length(DriverIK::Sub(after.p[lower],after.p[upper]))-DriverIK::Length(DriverIK::Sub(before.p[lower],before.p[upper])))<1e-5f,"gesture follow preserves arm length");
   for(unsigned bone=0;bone<61;++bone)if(bone!=upper)Check(candidate[bone]==original[bone],"look following preserves finger pose and all other local joints");
   float strength,error;Check(DriverIK::TrackHeadTorso(candidate,positions,{.04f,0,-.05f},.05f,strength,error,40),"torso compensation supports moving gesture target");auto leaned=DriverIK::FK(candidate,positions);Check(DriverIK::Length(DriverIK::Sub(leaned.p[hand],target))<.001f,"torso IK retains head-following gesture target");
   auto neutral=original;Check(FollowGestureHead(neutral,positions,right,0,yaw,1) && neutral==original,"released gesture leaves steering unchanged");
  }
  for(bool right:{false,true})for(float pitch:{-.6f,.6f}){
   auto candidate=pose;BlendWindowGesture(candidate,{1,1,1},right?kRightGestureTargets:kWindowGestureTargets);auto original=candidate;auto before=DriverIK::FK(candidate,positions);unsigned upper=right?26:5,hand=right?28:7;float yaw=.4f;
   Check(FollowGestureHead(candidate,positions,right,1,yaw,1,pitch),"gesture follows head pitch and yaw together");
   DriverIK::Q yawQ{std::cos(yaw/2),0,std::sin(yaw/2),0},pitchQ{std::cos(pitch/2),std::sin(pitch/2),0,0};auto turn=DriverIK::Product(yawQ,pitchQ);auto target=DriverIK::Add(before.p[upper],DriverIK::Rotate(turn,DriverIK::Sub(before.p[hand],before.p[upper])));auto after=DriverIK::FK(candidate,positions);
   Check(DriverIK::Length(DriverIK::Sub(after.p[hand],target))<.001f,"pitched hand swings about shoulder pivot");
   for(unsigned bone=0;bone<61;++bone)if(bone!=upper)Check(candidate[bone]==original[bone],"pitch follow preserves all other local joints and finger shape");
   float strength,error;Check(DriverIK::TrackHeadTorso(candidate,positions,{.04f,0,-.05f},.05f,strength,error,40),"pitched gesture remains compatible with torso IK");auto leaned=DriverIK::FK(candidate,positions);Check(DriverIK::Length(DriverIK::Sub(leaned.p[hand],target))<.001f,"torso IK preserves pitched gesture hand target");
  }
  for(unsigned mode=0;mode<6;++mode){
   auto candidate=pose;
   if(mode==1)BlendHornPose(candidate,{1,1});
   if(mode==2)BlendWindowGesture(candidate,{1,1,1},kWindowGestureTargets);
   if(mode==3)BlendWindowGesture(candidate,{1,1,1},kRightGestureTargets);
   if(mode==4)BlendWindowGesture(candidate,{1,1,1},kSignalLeftTargets);
   if(mode==5){BlendWindowGesture(candidate,{1,1,1},kIgnitionTargets);float d,e;Check(DriverIK::LeanAndCompensate(candidate,positions,1,d,e,.1524f),"ignition fixture before head compensation");}
   auto before=DriverIK::FK(candidate,positions);float strength,error;auto original=candidate;
   const bool solved=DriverIK::TrackHeadTorso(candidate,positions,{.04f,0,-.04f},.05f,strength,error);
   Check(solved,"head lean combines with horn gestures signals and ignition");
   auto after=DriverIK::FK(candidate,positions);
   for(unsigned hand:{7u,28u})Check(DriverIK::Length(DriverIK::Sub(after.p[hand],before.p[hand]))<.001f,"head lean retains existing action targets");
   for(unsigned finger=8;finger<=22;++finger)Check(candidate[finger]==original[finger],"head lean retains active left finger shape");
   for(unsigned finger=29;finger<=43;++finger)Check(candidate[finger]==original[finger],"head lean retains active right finger shape");
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
 auto headInvalid=pose;float headStrength,headError;Check(!DriverIK::TrackHeadTorso(headInvalid,kDriverLocalPositions,{std::numeric_limits<float>::quiet_NaN(),0,0},0,headStrength,headError) && headInvalid==pose,"invalid head target leaves skeleton unchanged");
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
