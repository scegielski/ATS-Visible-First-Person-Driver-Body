#include "ExclusiveEditingModes.hpp"
#include "ControllerStickNudge.hpp"
#include "VehicleSteeringPose.hpp"
#include "NativeDriverRender.hpp"
#include "NativeRadialActions.hpp"
#include "ControllerRadial.hpp"
#include "HandGrab.hpp"
#include "MenuKeyPulse.hpp"
#include "MenuDoublePress.hpp"
#include "MenuButtonRoute.hpp"
#include "ControllerModelData.hpp"
#include "ControllerModelInput.hpp"
#include "EarlyMenuCursor.hpp"
#include "NativeMenuCursor.hpp"
#include "MenuLaser.hpp"
#include "ControllerMenuMouse.hpp"
#include "DriverSamples.hpp"
#include "SteeringPose.hpp"
#include "PedalMotion.hpp"
#include "HornMotion.hpp"
#include "HornPose.hpp"
#include "GameHorn.hpp"
#include "GameEngine.hpp"
#include "GameWindow.hpp"
#include "WiperCycle.hpp"
#include "RadioInteraction.hpp"
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
#include "ControllerFingers.hpp"
#include "LiveControllerFingers.hpp"
#include "ControllerFingerMode.hpp"
#include "ControllerSceneFrame.hpp"
#include "IgnitionInteraction.hpp"
#include "InteractionBoxEditor.hpp"
#include "MirrorInteraction.hpp"
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
void* gMapWidgetOriginal=nullptr;
void* gMenuModeOriginal=nullptr;void* gMenuSurfaceOriginal=nullptr;
void* gRadioInputOriginal=nullptr;
void* gRadialHudOriginal=nullptr;void* gRadialCameraOriginal=nullptr;void* gRadialActivateOriginal=nullptr;
void* gRadialMapOriginal=nullptr;void* gRadialQuickOriginal=nullptr;void* gRadialServicesOriginal=nullptr;
void* gWipersOriginal=nullptr;
void* gLightsOriginal=nullptr;
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
extern "C" uint64_t RadioInputTest(uintptr_t,uint64_t);
extern "C" void RadioInputContinuation();
extern "C" void CaptureMenuMode(uint32_t) noexcept {}
extern "C" void CaptureMenuSurface(uintptr_t) noexcept {}
extern "C" void CaptureMapWidget(uintptr_t) noexcept {}
static unsigned radialObserved=99;
extern "C" void CaptureNativeRadialAction(uintptr_t owner,unsigned action) noexcept {if(owner!=123)std::abort();radialObserved=action;}
extern "C" uint64_t RadialHudTest(uintptr_t,uint64_t);extern "C" void RadialHudContinuation();
extern "C" uint64_t RadialCameraTest(uintptr_t,uint64_t);extern "C" void RadialCameraContinuation();
extern "C" uint64_t RadialActivateTest(uintptr_t,uint64_t);extern "C" void RadialActivateContinuation();
extern "C" uint64_t RadialMapTest(uintptr_t,uint64_t);extern "C" void RadialMapContinuation();
extern "C" uint64_t RadialQuickTest(uintptr_t,uint64_t);extern "C" void RadialQuickContinuation();
extern "C" uint64_t RadialServicesTest(uintptr_t,uint64_t);extern "C" void RadialServicesContinuation();
extern "C" void CaptureRadioInput(uintptr_t context) noexcept {if(context!=123)std::abort();}
extern "C" uint64_t WipersTest(uintptr_t,uint64_t,uint64_t,uint64_t*);
extern "C" uint64_t LightsTest(uintptr_t,uint64_t,uint64_t,uint64_t*);
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
uint32_t recenterExpected=0x55667701;unsigned char recenterReplacement=1;
extern "C" uint32_t CaptureNativeRecenter(uintptr_t owner,uint32_t request) noexcept {Check(owner==123 && request==recenterExpected,"native recenter observer callback arguments");return recenterReplacement;}
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
extern "C" {extern void* gHornControlOriginal;uint64_t HornControlObserveStub(uintptr_t,uint64_t,uint64_t,uint64_t);}
extern "C" uint64_t HornControlTestContinuation(uintptr_t object,uint64_t held,uint64_t air,uint64_t transform){Check(object==123 && air==456 && transform==789,"horn observer preserves receiver and other arguments");return held;}
int main() {
 Check(ControllerRadialRecenterSelection(0,5)&&!ControllerRadialRecenterSelection(1,5)&&!ControllerRadialRecenterSelection(0,3),"VR recenter only on left lower-left octant");

 Check(ControllerRadialSetupSelection(0,3)&&!ControllerRadialSetupSelection(1,3)&&!ControllerRadialSetupSelection(0,2),"left lower-right radial setup slot only");InteractionBoxEditor setupToggleTest;setupToggleTest.selected=1;Check(setupToggleTest.Toggle()==1&&setupToggleTest.enabled&&setupToggleTest.selected==-1&&setupToggleTest.dirty,"setup toggle releases and marks active edit for saving");Check(setupToggleTest.Toggle()==2&&!setupToggleTest.enabled,"setup radial toggle disables");

 Check(ControllerRadialAction(1,2)==3&&ControllerRadialAction(1,4)==4&&ControllerRadialAction(1,5)==5&&ControllerRadialAction(1,1)==6,"right HUD next interior activate octants");
 gRadialHudOriginal=reinterpret_cast<void*>(RadialHudContinuation);Check(RadialHudTest(123,0x1122334455667788ull)==0x1122334455667788ull&&radialObserved==3,"HUD native stub state");gRadialCameraOriginal=reinterpret_cast<void*>(RadialCameraContinuation);Check(RadialCameraTest(123,0x1122334455667788ull)==0x1122334455667788ull&&radialObserved==5,"camera native stub state");gRadialActivateOriginal=reinterpret_cast<void*>(RadialActivateContinuation);Check(RadialActivateTest(123,0x1122334455667788ull)==0x1122334455667788ull&&radialObserved==6,"activate native stub state");

 gRadialMapOriginal=reinterpret_cast<void*>(RadialMapContinuation);Check(RadialMapTest(123,0x1122334455667788ull)==0x1122334455667788ull&&radialObserved==0,"native map stub preserves registers and owner");gRadialQuickOriginal=reinterpret_cast<void*>(RadialQuickContinuation);Check(RadialQuickTest(123,0x1122334455667788ull)==0x1122334455667788ull&&radialObserved==1,"native quick info stub preserves registers and owner");gRadialServicesOriginal=reinterpret_cast<void*>(RadialServicesContinuation);Check(RadialServicesTest(123,0x1122334455667788ull)==0x1122334455667788ull&&radialObserved==2,"native services stub preserves registers and owner");

 Check(MergeNativeRadialAction(0,100,150)==2&&MergeNativeRadialAction(2,100,150)==2,"native radial binding presses merge without duplicate physical press");Check(MergeNativeRadialAction(1,0,150)==1&&MergeNativeRadialAction(0,100,700)==0&&MergeNativeRadialAction(0,200,100)==0,"native radial rejects absent expired or reversed-time requests");

 Check(ControllerRadialAction(0,0)==0&&ControllerRadialAction(1,6)==1&&ControllerRadialAction(1,0)==2,"radial map F1 F7 routing");Check(ControllerRadialAction(0,6)==-1&&ControllerRadialAction(1,7)==-1,"unassigned radial octants send no keys");

 ControllerRadialNote noteTest;Check(!noteTest.Step(false,0)&&noteTest.Step(true,0),"radial note begins on highlighted segment");Check(!noteTest.Step(true,0)&&noteTest.Step(true,1),"radial notes only on segment changes");Check(!noteTest.Step(true,-1)&&noteTest.Step(true,1),"center reentry repeats note");Check(!noteTest.Step(false,1)&&noteTest.Step(true,1),"reopening repeats note");

 ControllerFingerMode buttonFinger;buttonFinger.mask=0;buttonFinger.finger=0;Check(!buttonFinger.ToggleButton(true,false),"editing inactive button ignored");Check(!buttonFinger.ToggleButton(true,true),"held button on entering editing ignored");buttonFinger.ToggleButton(false,true);Check(buttonFinger.ToggleButton(true,true)&&buttonFinger.Selected(),"Y B enables selected finger");Check(!buttonFinger.ToggleButton(true,true),"Y B hold does not repeat");buttonFinger.ToggleButton(false,true);Check(buttonFinger.ToggleButton(true,true)&&!buttonFinger.Selected(),"Y B disables selected finger");

 ControllerFingerMode directionalFinger;directionalFinger.finger=0;directionalFinger.mask=0;directionalFinger.AssignDirection(0,true);Check(directionalFinger.AssignDirection(.9f,true)&&directionalFinger.Selected(),"right enables finger");Check(!directionalFinger.AssignDirection(.9f,true),"held direction does not repeat");directionalFinger.AssignDirection(0,true);Check(!directionalFinger.AssignDirection(.9f,true)&&directionalFinger.Selected(),"right is idempotent");directionalFinger.AssignDirection(0,true);Check(directionalFinger.AssignDirection(-.9f,true)&&!directionalFinger.Selected(),"left disables finger");directionalFinger.AssignDirection(0,false);Check(!directionalFinger.AssignDirection(.9f,true),"inactive editing requires neutral before assignment");

 ControllerFingerSetupMode setupMode;Check(!setupMode.enabled&&!setupMode.Select(1,2)&&!setupMode.Select(0,1),"finger setup defaults off and ignores other slots");Check(setupMode.Select(0,2)&&setupMode.enabled,"left right radial enables finger setup");Check(setupMode.Select(0,2)&&!setupMode.enabled,"same radial slot disables finger setup");
 ControllerRadialConfirm radialTest;Check(!radialTest.Step(true,false,1,0),"radial initializes closed");Check(!radialTest.Step(true,true,-1,10)&&radialTest.open,"radial press opens centered menu");Check(!radialTest.Step(true,true,2,20)&&radialTest.open,"radial hold highlights without selecting");Check(radialTest.Step(true,false,2,30)&&radialTest.selected==2&&!radialTest.open,"release selects current octant and closes");Check(!radialTest.Step(true,false,3,40),"unpressed tilt does not select");radialTest.Step(true,true,7,50);Check(radialTest.Step(true,false,7,60)&&radialTest.selected==7,"eighth octant release selection");radialTest.Step(true,true,0,70);Check(!radialTest.Step(true,false,-1,80)&&!radialTest.open,"center release cancels");radialTest.Step(false,false,0,90);Check(!radialTest.Step(true,true,0,100)&&!radialTest.open,"reconnect held stick does not open");ControllerRadialConfirm otherRadial;Check(otherRadial.selected==-1,"radial hands independent");
 Check(ControllerRadialDirection(0,0)==-1 && ControllerRadialDirection(.44f,0)==-1,"radial neutral deadzone");
 Check(ControllerRadialDirection(0,1)==0 && ControllerRadialDirection(1,0)==2 && ControllerRadialDirection(0,-1)==4 && ControllerRadialDirection(-1,0)==6,"radial cardinal directions");
 Check(ControllerRadialDirection(1,1)==1 && ControllerRadialDirection(1,-1)==3 && ControllerRadialDirection(-1,-1)==5 && ControllerRadialDirection(-1,1)==7,"radial diagonal octants");
 auto animatedInput=ControllerModelInput(true,true,false,.4f,.7f,-.8f,.6f);Check(animatedInput.rAxis[0].x==-.8f && animatedInput.rAxis[0].y==.6f && animatedInput.rAxis[1].x==.7f && animatedInput.rAxis[2].x==.4f,"model analog channels match Quest component definitions");Check((animatedInput.ulButtonPressed&vr::ButtonMaskFromId(vr::k_EButton_Axis0)) && (animatedInput.ulButtonPressed&vr::ButtonMaskFromId(vr::k_EButton_Grip)) && !(animatedInput.ulButtonPressed&vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu)),"model stick and face buttons independent");auto releasedInput=ControllerModelInput(false,false,false,0,0,0,0);Check(!releasedInput.ulButtonPressed && releasedInput.rAxis[1].x==0,"model input releases to neutral");

 gHornControlOriginal=reinterpret_cast<void*>(HornControlTestContinuation);Check(HornControlObserveStub(123,0x1122334455667701ull,456,789)==0x1122334455667701ull,"horn observer forwards full register with native held state intact");

 std::ifstream fingerFile("../../releases/controller_left_fingers.txt");std::string fingerText((std::istreambuf_iterator<char>(fingerFile)),{}),fingerError;LiveControllerFingerPose liveFingerLeft(false),liveFingerRight(true);
 Check(liveFingerLeft.Replace(fingerText,fingerError) && liveFingerLeft.Revision()==1,"live controller finger file loads all open closed joints");auto retained=liveFingerLeft.Snapshot();
 Check(!liveFingerLeft.Replace("SPF_CONTROLLER_FINGERS 1\nbone 8 1 0 0 0 1 0 0 0",fingerError) && liveFingerLeft.Snapshot()==retained && liveFingerLeft.Revision()==1,"incomplete live finger edit retains last valid pose");
 Check(!liveFingerRight.Replace(fingerText,fingerError),"left pose cannot edit right-hand bones");
 std::string duplicate=fingerText+"bone 8 1 0 0 0 1 0 0 0\n";Check(!liveFingerLeft.Replace(duplicate,fingerError) && liveFingerLeft.Snapshot()==retained,"duplicate finger joints rejected transactionally");
 std::string changed=fingerText;auto row=changed.find("bone 8 ");auto end=changed.find('\n',row);changed.replace(row,end-row,"bone 8 1 0 0 0 .923879533 0 0 .382683432");Check(liveFingerLeft.Replace(changed,fingerError) && liveFingerLeft.Revision()==2,"changed open closed targets publish a live revision");
 DriverIK::Pose fingerProbe{};for(auto& q:fingerProbe)q={1,0,0,0};auto edited=liveFingerLeft.Snapshot();Check(BlendControllerFingers(fingerProbe,false,0,1,1,edited.get()) && std::abs(fingerProbe[8][3]-.382683432f)<.0001f,"live edited index closed pose is used by default trigger");
 std::string zero=changed;row=zero.find("bone 8 ");end=zero.find('\n',row);zero.replace(row,end-row,"bone 8 0 0 0 0 1 0 0 0");auto currentFingerPose=liveFingerLeft.Snapshot();Check(!liveFingerLeft.Replace(zero,fingerError) && liveFingerLeft.Snapshot()==currentFingerPose,"zero quaternion retains latest valid finger pose");

 gXRActionAttachOriginal=reinterpret_cast<void*>(XRActionAttachContinuation);Check(XRActionAttachTest(123,456)==789,"XR attachment observer replaces only input pointer and preserves continuation");
 std::array<XrActionSet,17> attached{};XrActionSet originalAction=reinterpret_cast<XrActionSet>(1), extraAction=reinterpret_cast<XrActionSet>(2);XrSessionActionSetsAttachInfo originalInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};originalInfo.countActionSets=1;originalInfo.actionSets=&originalAction;XrSessionActionSetsAttachInfo combinedInfo{};
 Check(CombineControllerActionSets(originalInfo,extraAction,combinedInfo,attached) && combinedInfo.countActionSets==2 && attached[0]==originalAction && attached[1]==extraAction,"native controller attachment retains ATS action sets");
 originalInfo.countActionSets=17;Check(!CombineControllerActionSets(originalInfo,extraAction,combinedInfo,attached),"unexpected attachment layout rejected");

 ControllerElbowGuides guides;Check(ParseControllerElbowGuides("left -.12 -.35 .12\nright .12 -.35 .12",guides),"live controller elbow guides parse");Check(!ParseControllerElbowGuides("left 0 0 0 right 1 0 0",guides),"zero elbow guide rejected without replacing valid settings");
 Check(IgnitionBoxesOverlap({0,0,0},{.04f,.04f,.04f},{.05f,0,0},{1,0,0,0},{.04f,.035f,.055f}),"ignition oriented boxes overlap");Check(!IgnitionBoxesOverlap({0,0,0},{.04f,.04f,.04f},{.2f,0,0},{1,0,0,0},{.04f,.035f,.055f}),"separated ignition boxes reject contact");
 IgnitionTriggerGate triggerBox;Check(!triggerBox.Step(1,true,true),"held trigger on activation cannot start");triggerBox.Step(0,true,false);Check(!triggerBox.Step(1,true,false),"outside press does not start");Check(!triggerBox.Step(1,true,true),"moving inside with held trigger does not start");triggerBox.Step(0,true,true);Check(triggerBox.Step(1,true,true),"fresh overlap press starts once");Check(!triggerBox.Step(1,true,true),"held trigger cannot repeat start");
 InteractionBoxEditor editor;editor.boxes[0].center={0,0,0};editor.boxes[1].center={5,0,0};Check(editor.Key(true)==1,"setup hotkey enters once");Check(editor.Key(true)==0,"held setup hotkey does not repeat");editor.Key(false);
 BoxGrabInput grab[2]{};grab[0].valid=grab[1].valid=true;editor.Step(grab);grab[0].grip=1;editor.Step(grab);Check(editor.selected==0,"grip acquires nearby box");grab[0].position={.1f,0,0};editor.Step(grab);Check(std::abs(editor.boxes[0].center[0]-.1f)<.0001f,"one hand moves box");
 grab[0].rotation={.70710678f,0,.70710678f,0};editor.Step(grab);Check(std::abs(editor.boxes[0].rotation[2]-.70710678f)<.0001f,"one hand rotates box");grab[1].position={.3f,0,0};grab[1].grip=1;editor.Step(grab);grab[1].position={.5f,0,0};editor.Step(grab);Check(std::abs(editor.boxes[0].stretch[2]-2)<.001f && editor.boxes[0].stretch[0]==1 && editor.boxes[0].stretch[1]==1,"two hand separation scales only selected local axis");
 auto beforeHandoff=editor.boxes[0];grab[1].grip=0;editor.Step(grab);Check(DriverIK::Length(DriverIK::Sub(editor.boxes[0].center,beforeHandoff.center))<.0001f,"two to one hand transfer does not jump");grab[0].grip=0;editor.Step(grab);Check(editor.dirty && editor.selected<0,"release requests persistent save");
 Check(NextControllerRadioStation(0,34,1)==1 && NextControllerRadioStation(33,34,1)==0 && NextControllerRadioStation(0,34,-1)==33 && NextControllerRadioStation(5,34,-1)==4,"radio full-list next previous and wrap");Check(NextControllerRadioStation(~uint64_t{},34,1)==0 && NextControllerRadioStation(~uint64_t{},34,-1)==33 && NextControllerRadioStation(0,0,1)==-1 && NextControllerRadioStation(0,10001,1)==-1,"radio missing selection and invalid counts");
 IgnitionTriggerGate bilateralIgnition[2];for(auto& gate:bilateralIgnition)gate.Step(0,true,true);Check(bilateralIgnition[0].Step(1,true,true) && !bilateralIgnition[0].Step(1,true,true),"left ignition trigger accepts one press and suppresses holding");Check(bilateralIgnition[1].Step(1,true,true),"right ignition remains independently armed");bilateralIgnition[0].Step(0,true,false);Check(!bilateralIgnition[0].Step(1,true,false),"left ignition rejects off-target press");bilateralIgnition[0].Step(0,false,true);Check(!bilateralIgnition[0].Step(1,true,true),"left ignition rejects held trigger after tracking loss");
 Check(ControllerTextureBytes(4,4,0,1)==64 && ControllerTextureBytes(4,4,1,1)==16 && ControllerTextureBytes(4,4,2,1)==8 && ControllerTextureBytes(4,4,3,3)==48,"SteamVR texture formats and mip byte counts");Check(!ControllerTextureBytes(0,4,0,1) && !ControllerTextureBytes(4,4,99,1),"invalid model textures rejected");ControllerMeshVertex meshVertices[3]{};uint16_t meshIndices[3]{0,1,2};Check(ValidControllerMesh(meshVertices,3,meshIndices,3),"valid controller triangle accepted");meshIndices[2]=3;Check(!ValidControllerMesh(meshVertices,3,meshIndices,3),"out of range controller triangle rejected");auto modelProjection=ControllerModelProjection(-1,1,-1,1);Check(modelProjection[0]==1 && modelProjection[5]==1 && modelProjection[11]==-1,"controller projection uses forward negative Z");
 EarlyMenuCursor pendingCursor;float pendingU=0,pendingV=0;Check(pendingCursor.Publish(.7f,.3f,100) && pendingCursor.Take(pendingU,pendingV,120) && pendingU==.7f && pendingV==.3f,"early worker cursor transfers to game thread");Check(!pendingCursor.Take(pendingU,pendingV,121),"early cursor consumes each command once");pendingCursor.Publish(.2f,.8f,200);Check(!pendingCursor.Take(pendingU,pendingV,301),"stale early cursor command rejected");Check(!pendingCursor.Publish(-1,.5f,400),"invalid early cursor coordinates rejected");
 float nativeRect[4]{0,2880,1800,0};NativeMenuCursorEvent absoluteEvent;Check(BuildNativeMenuCursorEvent(.75f,.25f,nativeRect,absoluteEvent) && absoluteEvent.type==3 && absoluteEvent.x==2160 && absoluteEvent.y==450,"native cursor maps laser UV to native viewport independent of desktop mouse gain");Check(BuildNativeMenuCursorEvent(0,1,nativeRect,absoluteEvent) && absoluteEvent.x==0 && absoluteEvent.y==1800,"native cursor reaches both viewport edges");nativeRect[1]=0;Check(!BuildNativeMenuCursorEvent(.5f,.5f,nativeRect,absoluteEvent),"invalid native viewport rejects cursor event");
 ControllerMenuMouse leftDrag,rightDrag;leftDrag.Step(true,true,0,0,0,0,.01f);rightDrag.Step(true,true,0,0,0,0,.01f);
 Check(rightDrag.Step(true,true,0,0,1,0,.01f).button==1 && !leftDrag.down,"grip starts independent right mouse drag");
 Check(rightDrag.Step(true,true,0,0,1,0,.01f).button==0 && rightDrag.down,"held grip keeps right drag without repeat");
 Check(leftDrag.Step(true,true,0,0,1,0,.01f).button==1 && rightDrag.down,"left and right buttons can be held independently");
 Check(rightDrag.Step(true,true,0,0,0,0,.01f).button==-1 && leftDrag.down,"grip release preserves held left button");
 Check(leftDrag.Step(false,false,0,0,1,0,.01f).button==-1,"tracking loss releases captured left button");
 rightDrag.Step(true,true,0,0,0,0,.01f);rightDrag.Step(true,true,0,0,1,0,.01f);Check(rightDrag.Step(false,false,0,0,1,0,.01f).button==-1,"tracking loss releases captured right button");
 HandGrabGesture grabGesture;Check(!grabGesture.Step(false,0,true,false,false,1),"grab waits for target and fresh press");Check(!grabGesture.Step(false,1,true,false,false,2),"off-target trigger cannot grab");Check(!grabGesture.Step(false,1,true,false,true,3),"entering hand target with held trigger cannot grab");grabGesture.Step(false,0,true,false,true,4);Check(grabGesture.Step(false,1,true,false,true,5)==1,"fresh targeted trigger engages hand");Check(!grabGesture.Step(true,1,true,false,true,6),"activation press does not release hand");grabGesture.Step(true,0,true,false,false,7);Check(!grabGesture.Step(true,1,true,false,false,10),"first release click starts double press");grabGesture.Step(true,0,true,false,false,20);Check(grabGesture.Step(true,1,true,false,false,100)==-1,"double trigger releases without targeting");grabGesture.Step(false,0,true,true,true,110);Check(!grabGesture.Step(false,1,true,true,true,120),"setup mode suppresses hand engagement");
 InteractionBox localGrab{{.1f,.02f,-.03f},{.04f,.035f,.055f},{1,0,0,0},1.4f,{1,2,3}};auto wristTurn=DriverIK::Q{.70710678f,0,.70710678f,0};auto attachedGrab=AttachHandBox(localGrab,{1,2,3},wristTurn);auto detachedGrab=DetachHandBox(attachedGrab,{1,2,3},wristTurn);Check(DriverIK::Length(DriverIK::Sub(detachedGrab.center,localGrab.center))<1e-5f&&detachedGrab.stretch==localGrab.stretch&&detachedGrab.scale==localGrab.scale,"wrist-local grab box transforms round trip including nonuniform size");
 InteractionBoxEditor handBoxEditor;handBoxEditor.boxes[12]=localGrab;HandBoxAttachments attachments;attachments.Load(handBoxEditor);attachments.valid=true;attachments.wrist[0]={1,2,3};attachments.rotation[0]=wristTurn;handBoxEditor.boxes[12]=attachedGrab;handBoxEditor.selected=12;attachments.CaptureEdit(handBoxEditor);auto savedHands=SerializeInteractionBoxes(attachments.Saved(handBoxEditor));InteractionBoxEditor loadedHands;Check(ParseInteractionBoxes(savedHands,loadedHands)&&DriverIK::Length(DriverIK::Sub(loadedHands.boxes[12].center,localGrab.center))<1e-5f,"fourteen-box format saves local hand offsets");
 auto stableLocal=attachments.local[0];handBoxEditor.selected=-1;handBoxEditor.boxes[12].center={99,99,99};for(unsigned i=0;i<1000;++i)attachments.CaptureEdit(handBoxEditor);Check(attachments.local[0].center==stableLocal.center&&attachments.local[0].rotation==stableLocal.rotation,"unedited hand boxes do not accumulate setup transform drift");
 auto scaledWrist=wristTurn;for(float& v:scaledWrist)v*=.97f;auto normalizedGrab=AttachHandBox(localGrab,{1,2,3},scaledWrist);auto axisX=DriverIK::Rotate(normalizedGrab.rotation,{1,0,0}),axisY=DriverIK::Rotate(normalizedGrab.rotation,{0,1,0});Check(std::abs(DriverIK::Length(axisX)-1)<1e-5f&&std::abs(DriverIK::Dot(axisX,axisY))<1e-5f,"non-unit wrist rotations cannot shear hand boxes");
 ControllerUpNudge zoomNudge;Check(!zoomNudge.Step(1,true)&&!zoomNudge.Step(0,true)&&zoomNudge.Step(.8f,true)&&!zoomNudge.Step(1,true)&&!zoomNudge.Step(0,false)&&!zoomNudge.Step(1,true)&&!zoomNudge.Step(0,true)&&zoomNudge.Step(.9f,true),"navigation zoom requires a fresh upward nudge after centering and cancels on lost targeting");
 InteractionBoxEditor remoteEditor;remoteEditor.enabled=true;BoxGrabInput remoteInputs[2]{};remoteInputs[0].valid=true;remoteInputs[0].position={5,5,5};remoteInputs[0].rayBox=14;remoteInputs[0].rayDistance=1;auto remoteStart=remoteEditor.boxes[14].center;remoteEditor.Step(remoteInputs);remoteInputs[0].grip=1;remoteEditor.Step(remoteInputs);Check(remoteEditor.selected==14&&DriverIK::Length(DriverIK::Sub(remoteEditor.boxes[14].center,remoteStart))<1e-5f,"laser grip selects a remote setup box without snapping to the controller");remoteInputs[0].position[0]+=.2f;remoteInputs[0].rayBox=-1;remoteEditor.Step(remoteInputs);Check(std::abs(remoteEditor.boxes[14].center[0]-remoteStart[0]-.2f)<1e-5f,"remote setup grab stays attached after laser leaves the box");remoteInputs[0].valid=false;remoteEditor.Step(remoteInputs);Check(remoteEditor.selected<0&&remoteEditor.dirty,"tracking loss releases and saves remote box edit");
 InteractionBoxEditor exclusiveBoxes;ControllerFingerSetupMode exclusiveFingers;exclusiveFingers.enabled=true;ToggleExclusiveBoxSetup(exclusiveBoxes,exclusiveFingers);Check(exclusiveBoxes.enabled&&!exclusiveFingers.enabled,"setup activation disables finger editing");exclusiveBoxes.selected=14;exclusiveBoxes.owner=0;Check(SelectExclusiveFingerSetup(exclusiveBoxes,exclusiveFingers,0,2)&&exclusiveFingers.enabled&&!exclusiveBoxes.enabled&&exclusiveBoxes.selected==-1&&exclusiveBoxes.dirty,"finger editing activation releases and saves setup grab");KeyExclusiveBoxSetup(exclusiveBoxes,exclusiveFingers,true);Check(exclusiveBoxes.enabled&&!exclusiveFingers.enabled,"F9 setup activation also excludes finger editing");KeyExclusiveBoxSetup(exclusiveBoxes,exclusiveFingers,true);Check(exclusiveBoxes.enabled&&!exclusiveFingers.enabled,"holding setup key does not repeat mode transition");ToggleExclusiveBoxSetup(exclusiveBoxes,exclusiveFingers);Check(!exclusiveBoxes.enabled&&!exclusiveFingers.enabled,"exiting setup leaves both modes off");
 auto expandedBoxes=SerializeInteractionBoxes(InteractionBoxEditor{});auto fourteenBoxes=expandedBoxes;fourteenBoxes.replace(fourteenBoxes.find("BOXES 11"),8,"BOXES 10");fourteenBoxes.erase(fourteenBoxes.find("dashboard "));InteractionBoxEditor upgradedBoxes;Check(ParseInteractionBoxes(fourteenBoxes,upgradedBoxes)&&upgradedBoxes.boxes[14].center[1]==1.6f&&upgradedBoxes.boxes[15].half[0]==.05f&&!upgradedBoxes.authored[14],"fourteen-box profiles gain display boxes without replacing hand placements");Check(ParseInteractionBoxes(expandedBoxes,upgradedBoxes)&&upgradedBoxes.authored[14]&&upgradedBoxes.authored[15],"sixteen-box profiles preserve dashboard and infotainment placements");
 std::string oldBoxes=SerializeInteractionBoxes(InteractionBoxEditor{});oldBoxes.replace(oldBoxes.find("BOXES 11"),8,"BOXES 9");auto oldEnd=oldBoxes.find("hand_left ");oldBoxes.erase(oldEnd);loadedHands.boxes[12].center={99,99,99};Check(ParseInteractionBoxes(oldBoxes,loadedHands)&&loadedHands.boxes[12].center[0]==.07f,"legacy twelve-box files initialize hand-local defaults instead of retaining world coordinates");
 MenuKeyPulse keyPulse;Check(keyPulse.Step(true,true,100)==1,"shortcut key emits key down");Check(keyPulse.Step(true,false,101)==0&&keyPulse.down,"key remains held across frames");Check(keyPulse.Step(true,false,179)==0&&keyPulse.down,"key does not release before 80ms");Check(keyPulse.Step(true,false,180)==-1&&!keyPulse.down,"key emits delayed key up");keyPulse.Step(true,true,200);Check(keyPulse.Step(false,false,201)==-1,"focus loss immediately releases shortcut key");
 MenuDoublePress doubleX,doubleY;doubleX.Step(true,false,90);doubleY.Step(true,false,90);
 Check(!doubleX.Step(true,true,100)&&!doubleX.Step(true,true,110),"single press and holding do not invoke menu key");
 doubleX.Step(true,false,120);Check(doubleX.Step(true,true,250),"two menu presses within 500ms invoke key once");Check(!doubleX.Step(true,true,260),"held second press does not repeat key");
 doubleY.Step(true,true,100);doubleY.Step(true,false,120);Check(!doubleY.Step(true,true,650),"slow second press starts a new pair");
 doubleY.Step(false,false,660);Check(!doubleY.Step(true,true,670),"focus loss resets pair and held button cannot fire on regain");
 doubleX.Step(true,false,270);Check(!doubleX.Step(true,true,280),"third press is not a second overlapping double press");
 MenuButtonRoute route;auto routeOut=route.Step(1,0,true);Check(!routeOut.pressed&&!route.heldMask,"map trigger waits to distinguish click from drag");
 routeOut=route.Step(0,0,true,2,1);Check(!routeOut.pressed,"small map pointer jitter stays a click");
 routeOut=route.Step(-1,0,true);Check(routeOut.tapped==1&&!routeOut.pressed&&!routeOut.released,"stationary map trigger release emits only left tap");
 route.Step(1,0,true);routeOut=route.Step(0,0,true,10,0,.05f);Check(!routeOut.pressed,"map movement waits for drag delay");route.Step(0,0,true,0,0,.05f);routeOut=route.Step(0,0,true,0,0,.05f);Check(routeOut.pressed==2&&route.heldMask==2,"map motion converts trigger to right drag");
 routeOut=route.Step(0,0,false);Check(!routeOut.pressed&&!routeOut.released&&route.heldMask==2,"map exit cannot switch a held drag");
 routeOut=route.Step(-1,0,false);Check(routeOut.released==2&&!routeOut.tapped,"right drag release never adds a left click");
 routeOut=route.Step(1,0,false);Check(routeOut.pressed==1,"accessory trigger remains immediate left button");
 routeOut=route.Step(0,0,true);Check(!routeOut.pressed&&route.heldMask==1,"opening map cannot change existing left drag");route.Step(-1,0,true);
 route.Step(1,1,true);route.Step(0,0,true,10,0,.05f);route.Step(0,0,true,0,0,.05f);route.Step(0,0,true,0,0,.05f);routeOut=route.Step(-1,0,true);Check(!routeOut.released&&route.heldMask==2,"grip keeps right button held after trigger release");
 routeOut=route.Step(0,-1,true);Check(routeOut.released==2,"last right-button source releases right drag");
 route.Step(1,0,true);routeOut=route.Cancel();Check(!routeOut.tapped&&!routeOut.pressed&&!route.heldMask,"tracking loss cancels pending click without acting");
 route.Step(1,1,false);routeOut=route.Cancel();Check(routeOut.released==3&&!route.heldMask,"focus loss releases both captured buttons");
 MenuCursorMotion laserCursor;auto cursorOut=laserCursor.Step(true,1,.5f,.5f,1920,1080,.01f);Check(cursorOut.anchor && cursorOut.dx==0,"menu cursor anchors only on acquisition");for(int i=0;i<20;++i){cursorOut=laserCursor.Step(true,1,.5f,.5f,1920,1080,.01f);Check(!cursorOut.anchor && cursorOut.dx==0 && cursorOut.dy==0,"stationary menu hit emits no cursor warps or relative motion");}cursorOut=laserCursor.Step(true,1,.6f,.5f,1920,1080,.01f);Check(!cursorOut.anchor && cursorOut.dx>0 && cursorOut.dx<192 && cursorOut.dy==0,"menu hit change restores filtered relative movement");
 MenuCursorMotion scaled;scaled.Step(true,0,.1f,.2f,1001,1001,.01f,1.5f,2.f);int totalX=0,totalY=0;for(int i=0;i<100;++i){auto o=scaled.Step(true,0,.9f,.8f,1001,1001,.01f,1.5f,2.f);totalX+=o.dx;totalY+=o.dy;}Check(std::abs(totalX-1200)<=1 && std::abs(totalY-1200)<=1,"independent menu gains preserve full accumulated travel");for(int i=0;i<100;++i){auto o=scaled.Step(true,0,.1f,.2f,1001,1001,.01f,1.5f,2.f);totalX+=o.dx;totalY+=o.dy;}Check(std::abs(totalX)<=1 && std::abs(totalY)<=1,"reversing laser motion returns without accumulating drift");float gx=1,gy=1;Check(ParseMenuCursorGain("1.5 2",gx,gy) && gx==1.5f && gy==2,"live cursor scale accepts independent axes");Check(!ParseMenuCursorGain("nan 2",gx,gy) && !ParseMenuCursorGain("-1 2",gx,gy) && !ParseMenuCursorGain("1 2 extra",gx,gy) && gx==1.5f && gy==2,"invalid live cursor scale preserves previous values");bool noMenuHits[2]{false,false},noMenuPress[2]{false,false};Check(SelectMenuLaserOwner(1,false,noMenuHits,noMenuPress)==1,"brief misses do not transfer menu pointer to resting controller");
 float leftFinalMenu[8]{.032f,-.5f,-.8f,0,1,0,0,0},rightFinalMenu[8]{-.032f,-.5f,-.8f,0,1,0,0,0};float leftEye[3]{-.032f,0,0},rightEye[3]{.032f,0,0};RemoveMenuEyeOffset(leftFinalMenu,leftEye);RemoveMenuEyeOffset(rightFinalMenu,rightEye);Check(std::abs(leftFinalMenu[0]-rightFinalMenu[0])<1e-6f && leftFinalMenu[4]==1 && leftFinalMenu[2]==-.8f,"final menu pose removes per-eye translation without altering final orientation");
 auto headLocalRay=MenuRayInHeadSpace({0,1.5f,-.2f},{1,0,0,0},{0,1.5f,0},{1,0,0,0});Check(std::abs(headLocalRay.origin[1])<1e-6f && headLocalRay.direction[2]<-.99f,"menu ray uses head-relative origin rather than seated cabin origin");auto roomTurn=DriverIK::Q{std::cos(.6f),0,std::sin(.6f),0};auto transformedRay=MenuRayInHeadSpace(DriverIK::Add({2,3,4},DriverIK::Rotate(roomTurn,{0,1.5f,-.2f})),roomTurn,DriverIK::Add({2,3,4},DriverIK::Rotate(roomTurn,{0,1.5f,0})),roomTurn);Check(DriverIK::Length(DriverIK::Sub(transformedRay.origin,headLocalRay.origin))<1e-5f && DriverIK::Length(DriverIK::Sub(transformedRay.direction,headLocalRay.direction))<1e-5f,"menu targeting is invariant to room origin and seated recenter yaw");
 MenuPoseMatrix aimDevice{{{{1,0,0,1}},{{0,1,0,2}},{{0,0,1,3}}}},questTip{{{{1,0,0,.016694f}},{{0,.7944147f,.6073758f,-.02522f}},{{0,-.6073758f,.7944147f,.024687f}}}};auto tipWorld=ComposeMenuAim(aimDevice,questTip);Check(std::abs(tipWorld[1][2]-.6073758f)<1e-6f && std::abs(tipWorld[1][3]-1.97478f)<1e-5f,"menu ray uses Quest tip pitch and offset rather than grip frame");Check(-tipWorld[1][2]<-.6f && -tipWorld[2][2]<-.79f,"Quest pointing tip aims downward from the grip axis while preserving forward direction");
 bool bothMenuHits[2]{true,true},leftMenuPress[2]{true,false},rightMenuPress[2]{false,true};Check(SelectMenuLaserOwner(1,false,bothMenuHits,leftMenuPress)==0,"left menu trigger acquires pointer even while right laser also hits");Check(SelectMenuLaserOwner(0,false,bothMenuHits,rightMenuPress)==1,"right menu trigger acquires pointer independently");Check(SelectMenuLaserOwner(1,true,bothMenuHits,leftMenuPress)==1,"menu drag retains ownership until release");
 // Native mesh uses angle 2*(u-.5)/r; renderer scales X by width/2,
 // Y by height/2, and Z by width, making the curved cross-section elliptical.
 for(float radius:{.63661977f,3.f,8.f})for(float targetU:{.1f,.3f,.5f,.7f,.9f})for(float targetV:{.2f,.5f,.8f}){
  MenuLaserSurface nativePanel{{0,0,-3},{1,0,0,0},1.343f,.839f,radius};float angle=2*(targetU-.5f)/radius;DriverIK::V point{.5f*radius*nativePanel.width*std::sin(angle),(.5f-targetV)*nativePanel.height,-3+radius*nativePanel.width*(1-std::cos(angle))};auto result=IntersectMenuLaser(nativePanel,{0,0,0},point);Check(result.hit && std::abs(result.u-targetU)<.0001f && std::abs(result.v-targetV)<.0001f,"native rendered curved-panel vertices map back to exact UV across width and height");
 }
 MenuLaserSurface flatMenu{{0,0,-1},{1,0,0,0},2,1,0};auto laserHit=IntersectMenuLaser(flatMenu,{0,0,0},{0,0,-1});Check(laserHit.hit && std::abs(laserHit.u-.5f)<1e-5f && std::abs(laserHit.v-.5f)<1e-5f,"menu laser center projects to cursor center");Check(!IntersectMenuLaser(flatMenu,{0,0,0},{0,1,0}).hit && !IntersectMenuLaser(flatMenu,{0,0,0},{0,0,1}).hit,"menu laser rejects parallel and behind rays");flatMenu.radius=3;auto curved=IntersectMenuLaser(flatMenu,{0,0,0},{.25f,0,-1});Check(curved.hit && curved.u>.5f && curved.u<1,"curved menu intersection produces horizontal UV");flatMenu.rotation=DriverIK::Q{std::cos(.2f),0,std::sin(.2f),0};auto rotated=IntersectMenuLaser(flatMenu,{0,0,0},{0,0,-1});Check(rotated.hit,"menu laser handles rotated surface");
 ControllerMenuMouse menuTest;auto mo=menuTest.Step(true,true,0,0,1,0,.01f);Check(mo.dx==0 && mo.button==0,"menu activation anchors cursor and rejects already held trigger");menuTest.Step(true,true,0,0,0,0,.01f);mo=menuTest.Step(true,true,.01f,.02f,1,0,.01f);Check(mo.dx>0 && mo.dy<0 && mo.button==1,"menu rotation moves cursor and fresh trigger clicks");Check(menuTest.Step(true,true,.01f,.02f,1,0,.01f).button==0,"held menu trigger does not repeat");Check(menuTest.Step(false,false,0,0,0,0,.01f).button==-1,"menu exit or tracking loss releases mouse button");menuTest.Step(true,true,3.13f,0,0,0,.01f);Check(std::abs(menuTest.Step(true,true,-3.13f,0,0,0,.01f).dx)<40,"menu yaw wrap does not jump across screen");
 uint64_t lazyCount=0;int loads=0,reads=0;auto load=[&](){++loads;return true;};auto refresh=[&](){++reads;lazyCount=34;return true;};Check(EnsureControllerRadioList(lazyCount,load,refresh) && loads==1 && reads==1 && NextControllerRadioStation(~uint64_t{},lazyCount,1)==0,"first controller station request loads list before selecting once");Check(EnsureControllerRadioList(lazyCount,load,refresh) && loads==1 && reads==1,"loaded radio list skips initialization");lazyCount=0;Check(!EnsureControllerRadioList(lazyCount,[](){return false;},refresh) && reads==1,"failed radio initialization does not select");Check(!EnsureControllerRadioList(lazyCount,load,[](){return false;}),"failed station refresh rejects request");
 Check(ControllerRadioPlaybackAction(true,5,34)==-2 && ControllerRadioPlaybackAction(false,5,34)==5 && ControllerRadioPlaybackAction(false,~uint64_t{},34)==0 && ControllerRadioPlaybackAction(false,0,0)==-1,"radio trigger toggles existing stream or starts same selected station without advancing");IgnitionTriggerGate radioTriggerTest;radioTriggerTest.Step(0,true,true);Check(radioTriggerTest.Step(1,true,true) && !radioTriggerTest.Step(1,true,true),"radio trigger toggles once while held");radioTriggerTest.Step(0,true,false);Check(!radioTriggerTest.Step(1,true,false),"radio trigger outside target has no effect");
 ControllerRadioStick radioStickTest;auto radioTest=radioStickTest.Step(.9f,.8f,true);Check(radioTest.station==1 && radioTest.volume==.8f,"radio stick changes next station and volume independently");Check(radioStickTest.Step(.9f,0,true).station==0,"held radio stick does not repeatedly change stations");radioStickTest.Step(0,0,true);Check(radioStickTest.Step(-.9f,-.7f,true).station==-1 && radioStickTest.Step(0,-.7f,true).volume==-.7f,"radio previous station and held volume down");Check(radioStickTest.Step(.9f,.8f,false).station==0 && radioStickTest.Step(0,0,false).volume==0,"radio stops on target or tracking loss");
 InteractionBoxEditor radioEditor;radioEditor.boxes[11].center={11,12,13};Check(ParseInteractionBoxes(SerializeInteractionBoxes(radioEditor),radioEditor) && radioEditor.authored[11] && MirrorContact(radioEditor,{11,12,13},{1,0,0,0})==9,"radio target and twelve-box persistence");
 unsigned wiperSets=0;uint32_t wiperNext=99;auto setWiper=[&](uintptr_t actor,uint32_t next){Check(actor==123,"wiper native setter owner preserved");++wiperSets;wiperNext=next;};
 Check(ApplyControllerWiperCycle(123,0,4,setWiper) && wiperNext==1 && wiperSets==1,"wipers advance once");Check(ApplyControllerWiperCycle(123,3,4,setWiper) && wiperNext==0 && wiperSets==2,"highest wiper speed wraps off through native setter");Check(ApplyControllerWiperCycle(123,1,2,setWiper) && wiperNext==0,"two-state vehicle wipers wrap off");Check(!ApplyControllerWiperCycle(123,4,4,setWiper) && !ApplyControllerWiperCycle(123,0,0,setWiper) && !ApplyControllerWiperCycle(123,0,33,setWiper) && wiperSets==3,"invalid wiper states never call setter");
 InteractionBoxEditor wiperEditor;wiperEditor.boxes[10].center={10,11,12};Check(ParseInteractionBoxes(SerializeInteractionBoxes(wiperEditor),wiperEditor) && wiperEditor.authored[10] && MirrorContact(wiperEditor,{10,11,12},{1,0,0,0})==8,"wipers target and eleven-box persistence");
 InteractionBoxEditor lightsEditor;lightsEditor.boxes[9].center={9,10,11};Check(ParseInteractionBoxes(SerializeInteractionBoxes(lightsEditor),lightsEditor) && lightsEditor.authored[9] && MirrorContact(lightsEditor,{9,10,11},{1,0,0,0})==7,"lights target and ten-box serialization");
 IgnitionTriggerGate lightsGate;lightsGate.Step(0,true,false);Check(lightsGate.Step(1,true,true) && !lightsGate.Step(1,true,true),"lights cycle once per fresh trigger press");lightsGate.Step(0,true,true);Check(lightsGate.Step(1,true,true),"released lights trigger can cycle again");
 Check(MirrorPhysicalSlot(6)==9 && MirrorPhysicalSlot(4)==-1 && MirrorPhysicalSlot(5)==-1,"wheel combined adjustment routing leaves windows separate");InteractionBoxEditor wheelEditor;wheelEditor.boxes[8].center={8,9,10};Check(MirrorContact(wheelEditor,{8,9,10},{1,0,0,0})==6 && ParseInteractionBoxes(SerializeInteractionBoxes(wheelEditor),wheelEditor) && wheelEditor.authored[8],"wheel box contact and nine-box persistence");
 unsigned windowCalls=0;unsigned char passedOpen=0,passedClose=0;auto nativeWindow=[&](uintptr_t actor,uintptr_t state,float dt,unsigned char open,unsigned char close,unsigned char electric){++windowCalls;Check(actor==0x10000 && state==0x10434 && dt==.016f && electric==7,"window forwards owner state time and electrical flag");passedOpen=open;passedClose=close;return true;};
 Check(ForwardControllerWindow(nativeWindow,0x10000,0x10434,.016f,0,0,7,0x10000,true,-1) && windowCalls==1 && passedOpen==1 && passedClose==0,"window down opens using one native call and preserves return");
 ForwardControllerWindow(nativeWindow,0x10000,0x10434,.016f,0,0,7,0x10000,true,1);Check(passedOpen==0 && passedClose==1,"window up closes");
 ForwardControllerWindow(nativeWindow,0x10000,0x10434,.016f,1,0,7,0x10000,true,1);Check(passedOpen==1 && passedClose==0,"manual window input takes priority");
 ForwardControllerWindow(nativeWindow,0x10000,0x10434,.016f,0,0,7,0x20000,true,1);Check(!passedOpen && !passedClose,"other vehicle window passes untouched");
 ForwardControllerWindow(nativeWindow,0x10000,0x10434,.016f,0,0,7,0x10000,false,1);Check(!passedOpen && !passedClose,"paused unfocused or disabled window input passes untouched");
 Check(NativeWindowSide(0x10000,0x1044c,0x10000)==1 && NativeWindowSide(0x10000,0x10450,0x10000)==-1,"right window state identity and foreign state rejection");
 InteractionBoxEditor windowEditor;windowEditor.boxes[6].center={3,4,5};windowEditor.boxes[7].center={6,4,5};Check(MirrorContact(windowEditor,{3,4,5},{1,0,0,0})==4 && MirrorContact(windowEditor,{6,4,5},{1,0,0,0})==5,"left and right window targets route independently");Check(ParseInteractionBoxes(SerializeInteractionBoxes(windowEditor),windowEditor) && windowEditor.authored[7],"eight boxes round trip with windows");
 Check(ControllerHornPressed(true,true,1) && !ControllerHornPressed(true,true,0) && !ControllerHornPressed(false,true,1) && !ControllerHornPressed(true,false,1) && !ControllerHornPressed(true,true,std::numeric_limits<float>::quiet_NaN()),"horn requires target plus valid held trigger and stops on release or target loss");
 InteractionBoxEditor rayEditor;for(unsigned j=0;j<InteractionBoxCount;++j)rayEditor.boxes[j].center={20.f+static_cast<float>(j),0,0};rayEditor.boxes[2].center={0,0,-1};rayEditor.boxes[3].center={0,0,-1.5f};
 auto ray=RayInteractionBoxes(rayEditor,{0,0,0},{0,0,-1});Check(ray.box==2 && std::abs(ray.distance-.95f)<.001f,"laser chooses nearest mirror box along controller forward");
 Check(RayInteractionBoxes(rayEditor,{0,0,0},{0,0,1}).box==-1 && RayInteractionBoxes(rayEditor,{1,0,0},{0,0,-1}).box==-1,"laser rejects behind origin and parallel misses");
 Check(RayInteractionBoxes(rayEditor,{0,0,-1},{0,0,-1}).distance==0,"laser origin inside box hits immediately");
 rayEditor.boxes[2].rotation=DriverIK::Q{.70710678118f,0,.70710678118f,0};rayEditor.boxes[2].stretch={3,1,1};ray=RayInteractionBoxes(rayEditor,{0,0,0},{0,0,-1});Check(ray.box==2 && std::abs(ray.distance-.79f)<.001f,"laser respects box local rotation and nonuniform scale");
 Check(MirrorPhysicalSlot(3)==6,"seat box routes native seat mode");
 InteractionBoxEditor seatEditor;seatEditor.boxes[5].center={2,3,4};Check(MirrorContact(seatEditor,{2,3,4},{1,0,0,0})==3,"seat contact recognized separately from mirrors");Check(ParseInteractionBoxes(SerializeInteractionBoxes(seatEditor),seatEditor) && seatEditor.authored[5] && seatEditor.boxes[5].center==DriverIK::V{2,3,4},"six-box placements retain seat transform");
 auto rawVolume=ControllerInteractionVolume(false,true,true,false,{9,9,9},{1,0,0,0},{1,2,3},{1,0,0,0});Check(rawVolume.valid && rawVolume.center==DriverIK::V{1,2,3},"disabled hand uses controller-axis collision origin without calibration");
 auto palmVolume=ControllerInteractionVolume(true,true,true,true,{9,9,9},{1,0,0,0},{1,2,3},{1,0,0,0});Check(palmVolume.valid && palmVolume.center==DriverIK::V{9,9,9},"enabled hand retains calibrated palm contact");
 Check(!ControllerInteractionVolume(false,false,true,false,{},{1,0,0,0},{},{1,0,0,0}).valid && !ControllerInteractionVolume(false,true,false,false,{},{1,0,0,0},{},{1,0,0,0}).valid && !ControllerInteractionVolume(true,true,true,false,{},{1,0,0,0},{},{1,0,0,0}).valid,"tracking loss stale scene and failed enabled IK reject interaction");
 InteractionBoxEditor mirrors;mirrors.boxes[2].center={1,2,3};mirrors.boxes[3].center={4,2,3};mirrors.boxes[4].center={7,2,3};
 Check(MirrorContact(mirrors,{1,2,3},{1,0,0,0})==0 && MirrorContact(mirrors,{4,2,3},{1,0,0,0})==1 && MirrorContact(mirrors,{7,2,3},{1,0,0,0})==2,"mirror contact identifies left center right");
 Check(MirrorContact(mirrors,{10,2,3},{1,0,0,0})==-1 && MirrorPhysicalSlot(0)==0 && MirrorPhysicalSlot(1)==4 && MirrorPhysicalSlot(2)==2,"mirror slot routing and no-contact release");
 auto legacy=std::string("SPF_INTERACTION_BOXES 2\nignition 0 0 0 1 0 0 0 1 1 1 1\nhorn 1 0 0 1 0 0 0 1 1 1 1\n");Check(ParseInteractionBoxes(legacy,mirrors) && mirrors.boxes[2].center[0]==1 && !mirrors.authored[2],"legacy placements preserve mirror defaults");
 Check(ParseInteractionBoxes(SerializeInteractionBoxes(mirrors),mirrors) && mirrors.authored[4],"all five box placements round trip");
 auto savedBoxes=SerializeInteractionBoxes(editor);InteractionBoxEditor loadedEditor;Check(ParseInteractionBoxes(savedBoxes,loadedEditor) && std::abs(loadedEditor.boxes[0].stretch[2]-2)<.001f,"box placements round trip including rotation and scale");Check(!ParseInteractionBoxes("SPF_INTERACTION_BOXES 1 ignition 0 0",loadedEditor),"partial edit does not replace saved boxes");Check(editor.Key(true)==2,"setup hotkey exits");
 ControllerSceneFrame sceneFrame;sceneFrame.camera={0,1,0};sceneFrame.neutral={2,0,3};sceneFrame.xrHead={2,0,3};auto physical=DriverIK::V{.3f,1.1f,-.4f};auto initialMarker=sceneFrame.Position(physical,{0,1,0},{1,0,0,0});
 sceneFrame.xrHead={2.2f,.1f,3};auto movedMarker=sceneFrame.Position(physical,{.2f,1.1f,0},{1,0,0,0});Check(DriverIK::Length(DriverIK::Sub(initialMarker,movedMarker))<.00001f,"stationary controller marker stays fixed during head translation across tracking spaces");
 sceneFrame.xrHeadQ={.70710678f,0,.70710678f,0};auto turnedMarker=sceneFrame.Position(physical,{.2f,1.1f,0},{.70710678f,0,.70710678f,0});Check(DriverIK::Length(DriverIK::Sub(initialMarker,turnedMarker))<.00001f,"stationary marker stays fixed during matching head rotation");
 {
  ControllerSceneFrame vehicleFrame;vehicleFrame.SetSeatOffset({.02f,.04f,-.1f});
  Check(vehicleFrame.SetVehicleCamera({0,.3f,0},{-.4f,1.25f,.3f}),"vehicle camera origin accepts SDK cabin and head positions");
  Check(DriverIK::Length(DriverIK::Sub(vehicleFrame.camera,{-.38f,1.59f,.2f}))<1e-5f,"vehicle camera includes cabin pivot, head position and live seat offset once");
  auto validCamera=vehicleFrame.camera;Check(!vehicleFrame.SetVehicleCamera({0,0,0},{0,0,0})&&vehicleFrame.camera==validCamera,"missing SDK head position cannot overwrite a valid camera");
  auto origin=vehicleFrame.Position({0,0,-.3f},{0,0,0},{1,0,0,0});
  for(float z:{-.15f,-.05f,.05f,.15f}){auto point=vehicleFrame.Position({0,0,-.3f+z},{0,0,0},{1,0,0,0});Check(DriverIK::Length(DriverIK::Sub(DriverIK::Sub(point,origin),{0,0,z}))<1e-5f,"non-Mustang controller depth movement remains one-to-one");}
  vehicleFrame.ResetVehicleCamera();Check(DriverIK::Length(DriverIK::Sub(vehicleFrame.camera,{-.3847368658f,1.066045642f,.1469421685f}))<1e-5f,"vehicle switch clears the previous vehicle camera origin");
 }
 ControllerFingerPose routedPose;std::string routeError;std::string routeIndex=changed;auto routePos=routeIndex.find("trigger_finger index");routeIndex.replace(routePos,std::string("trigger_finger index").size(),"trigger_finger index");Check(ParseControllerFingerPose(routeIndex,false,routedPose,routeError) && routedPose[0].triggerControlled && !routedPose[3].triggerControlled,"live routing can select index instead of middle");
 Check(!ParseControllerFingerPose(changed+"trigger_finger thumb\n",false,routedPose,routeError),"duplicate routing rejected without partial pose updates");
 DriverIK::V priorRay{0,0,-1},priorBend{0,-1,0};
 for(unsigned step=0;step<200;++step){float t=step/199.f;auto ray=DriverIK::Unit(DriverIK::V{.3f*std::sin(t*6.28f),-.4f+.8f*t,-1});auto bend=StableControllerBend(ray,{0,-1,0},priorRay,priorBend,1.f/90.f);auto transported=DriverIK::Unit(DriverIK::Rotate(DriverIK::Between(priorRay,ray),priorBend));Check(DriverIK::Dot(bend,transported)>.998f,"controller elbow pole cannot flip between continuous frames");Check(std::abs(DriverIK::Dot(bend,ray))<.0001f,"stable elbow guide remains perpendicular to reach");priorRay=ray;priorBend=bend;}
 for(bool right:{false,true}){
  DriverIK::Pose opened{},closed{};const auto& targets=right?kControllerRightFingers:kControllerLeftFingers;
  for(auto& q:opened)q={1,0,0,0};closed=opened;
  Check(BlendControllerFingers(opened,right,1,0,1,nullptr,nullptr,0),"independent thumb opens while grip held");
  Check(BlendControllerFingers(closed,right,1,0,1,nullptr,nullptr,1),"independent thumb closes on face button");
  for(unsigned i=0;i<15;++i){const auto& expected=i>=12?targets[i].closed:opened[targets[i].bone];float dot=0;for(unsigned j=0;j<4;++j)dot+=closed[targets[i].bone][j]*expected[j];Check(std::abs(dot)>.9999f,"thumb button changes only thumb joints");}
 }
 for(bool right:{false,true}){
  const auto& source=right?kControllerRightFingers:kControllerLeftFingers;ControllerFingerMode mode;
  mode.Step(false,true,1,source);Check(!mode.Toggle(.9f,true),"held stick at startup cannot toggle");mode.Toggle(0,true);
  mode.Step(true,true,1,source);Check(std::string(mode.SpokenFinger())=="ring","spoken ring follows corrected skeleton order");Check(mode.finger==3 && mode.mask==2,"cycle selects without changing existing members");
  Check(mode.Toggle(.8f,true) && mode.mask==10,"inward push adds selected ring");Check(!mode.Toggle(.9f,true),"held inward push only toggles once");
  mode.Advance(source,.18f,.18f);Check(mode.weights[3]==1 && mode.weights[9]==1,"multiple assigned fingers follow trigger");
  mode.Toggle(0,true);Check(mode.Toggle(.8f,true) && mode.mask==2,"next push removes selected finger");
  mode.Step(false,true,1,source);mode.Step(true,true,1,source);Check(mode.finger==2,"cycle selects pinky after ring");
  mode.Toggle(0,true);mode.Toggle(.8f,true);Check(mode.mask==6,"pinky joins existing middle member");mode.Toggle(0,false);Check(!mode.Toggle(.8f,true),"tracking recovery held stick cannot toggle");
  mode.mask=10;mode.finger=3;mode.Step(false,true,1,source);mode.Step(true,true,1,source);Check(mode.mask==10,"selection retains all assigned fingers");
  mode.Step(false,true,2,source);Check(!mode.active && mode.mask==2,"live file restores configured membership");
 }


 ControllerHand button;button.Button(true,100,true);Check(button.enabled && button.calibrate,"controller first thumbstick press enables calibration");button.Button(true,200,true);Check(button.enabled,"held click does not toggle repeatedly");button.Button(false,300,true);button.Button(true,400,true);Check(!button.enabled,"second click disables independently");button.Button(true,1200,true);Check(button.enabled && button.calibrate,"long hold recalibrates and enables");

 gRecenterOriginal=reinterpret_cast<void*>(RecenterTestContinuation);Check(RecenterTestEntry(123,0x1122334455667701ull)==0x1122334455667701ull,"recenter observer leaves native edge register unchanged");
 recenterExpected=0x55667700;Check(RecenterTestEntry(123,0x1122334455667700ull)==0x1122334455667701ull,"controller reset changes only native boolean byte and preserves upper register");
 Check(MergeControllerHeadReset(0,100,200)==1&&MergeControllerHeadReset(1,0,200)==1,"controller reset merges with native reset without dropping user input");Check(MergeControllerHeadReset(0,100,601)==0&&MergeControllerHeadReset(0,300,200)==0,"stale or future controller reset is rejected");

 Check(GestureLookPitch({.9659258f,.258819f,0,0})>.5f && GestureLookPitch({.9659258f,-.258819f,0,0})<-.5f,"upward and downward head pitch signs follow tracked look");
 NativeRecenterGate titleFeedback,cabinCalibration;titleFeedback.Observe(1,100);cabinCalibration.Observe(1,100);
 Check(titleFeedback.Consume(120,110,true)&&!cabinCalibration.Consume(120,110,false),"title reset speaks from fresh head pose without a body or driving telemetry");
 Check(!titleFeedback.Consume(130,125,true),"title confirmation occurs only once");Check(cabinCalibration.Consume(140,135,true)&&!titleFeedback.Consume(140,135,true),"later body calibration does not repeat recenter speech");
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
 gRadioInputOriginal=reinterpret_cast<void*>(RadioInputContinuation);Check(RadioInputTest(123,0x1122334455667788ull)==0x1122334455667788ull,"radio observer preserves original register state and forwards");
 gWipersOriginal=reinterpret_cast<void*>(SignalTestContinuation);signalTestSide=4;signalReplacement=1;Check(WipersTest(123,0x1122334455667701ull,0xabcdef,signalSaved)==0x1122334455667701ull && signalSaved[0]==123 && signalSaved[2]==0xabcdef,"wipers stub preserves native input registers and forwards once");
 gLightsOriginal=reinterpret_cast<void*>(SignalTestContinuation);signalTestSide=3;signalReplacement=1;Check(LightsTest(123,0x1122334455667701ull,0xabcdef,signalSaved)==0x1122334455667701ull && signalSaved[0]==123 && signalSaved[1]==0x1122334455667701ull && signalSaved[2]==0xabcdef,"lights cycle stub preserves register arguments and forwards once");
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
 Check(IsManualEngineStartCaller(0x100000,0x100000+0x9f81ac) && !IsManualEngineStartCaller(0x100000,0x100000+0x5db25f),"manual caller guard excludes automatic engine paths");
 }

 gEngineOriginal=reinterpret_cast<void*>(EngineOriginal);Check(EngineRequestObserveStub(0x12345678,0xabcdef01,0xfedcba00,0x11223344,0x55667788)==0x987654,"engine observer preserves original return");
 suppressEngine=true;Check(EngineRequestObserveStub(0x12345678,0xabcdef01,0xfedcba00,0x11223344,0x55667788)==0 && engineOriginalCalls==1,"deferred engine request skips original exactly once");suppressEngine=false;
 {
 NativeBodyViewLedger bodyViews;
 Check(bodyViews.Take(1,1,0x100)&&!bodyViews.Take(1,1,0x100)&&bodyViews.Take(1,1,0x200),"interior submeshes cannot duplicate a body within one view/frame");
 Check(bodyViews.Take(2,1,0x100)&&bodyViews.Take(2,2,0x100)&&!bodyViews.Take(2,2,0),"new frames and worlds render; missing views do not");
 Check(IsCabinBodyAnchor("/vehicle/car/ford_bronco_24/interior/standard.pmd")&&!IsCabinBodyAnchor("/vehicle/car/ford_bronco_24/interior/accessory/radio.pmd")&&!IsCabinBodyAnchor("/asset/character/human/driver/man/driver.pmd"),"native body anchors to cabin geometry, excluding accessories and recursion");
 constexpr uintptr_t base=0x100000,game=0x200000,owner=0x300000,vt=0x400000,vehicle=0x500000,model=0x600000;bool wrong=false;
 auto reader=[&](uintptr_t a,void* out,size_t n){uintptr_t value=0;if(a==base+0x36badd0)value=game;else if(a==game+0xaa0)value=owner;else if(a==owner)value=vt;else if(a==vt+0x1b8)value=wrong?0:base+0x4d9cf0;else if(a==owner+0x31b0)value=vehicle;else if(a==vehicle+0x1df8)value=model;else return false;if(n!=8)return false;std::memcpy(out,&value,8);return true;};
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


  for(bool right:{false,true})for(float grip:{0.f,1.f})for(float trigger:{0.f,1.f}){
   auto candidate=pose;auto wrist=right?28u:7u;auto before=DriverIK::FK(pose,positions);Check(BlendControllerFingers(candidate,right,grip,trigger,1),"controller open grip and trigger finger poses blend");
   for(const auto& target:right?kControllerRightFingers:kControllerLeftFingers){auto expected=target.triggerControlled?(trigger?target.closed:target.open):(grip?target.closed:target.open);float dot=0;for(unsigned j=0;j<4;++j)dot+=candidate[target.bone][j]*expected[j];Check(std::abs(dot)>.99999f,"trigger controls selected finger independently from grip and thumb");}
   unsigned start=right?29:8;for(unsigned bone=0;bone<61;++bone)if(bone<start || bone>=start+15)Check(candidate[bone]==pose[bone],"controller fingers leave arms opposite hand and body unchanged");
   auto after=DriverIK::FK(candidate,positions);Check(DriverIK::Length(DriverIK::Sub(after.p[wrist],before.p[wrist]))<.00001f,"finger closure preserves IK wrist target");
   candidate=pose;Check(BlendControllerFingers(candidate,right,grip,trigger,0) && candidate==pose,"disabled controller fingers retain stock pose exactly");
  }
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
  {
   auto adjusted=pose;auto before=DriverIK::FK(pose,positions);VehicleSteeringPose grips;grips.offset[0]={0,.005f,.005f};grips.offset[1]={0,-.005f,.005f};
   Check(ApplyVehicleSteeringGrips(adjusted,positions,grips)==3,"both live vehicle grip offsets solve independently");auto after=DriverIK::FK(adjusted,positions);
   Check(DriverIK::Length(DriverIK::Sub(after.p[7],DriverIK::Add(before.p[7],grips.offset[0])))<.001f&&DriverIK::Length(DriverIK::Sub(after.p[28],DriverIK::Add(before.p[28],grips.offset[1])))<.001f,"resting hand offsets preserve requested vehicle-space displacement");
   for(unsigned bone=0;bone<61;++bone)if(bone<5||bone>7&&bone<26||bone>28)Check(adjusted[bone]==pose[bone],"resting hand adjustment preserves torso, fingers, head and legs");
  }
  for(bool right:{false,true}){
   auto candidate=pose;auto before=DriverIK::FK(pose,positions);unsigned wrist=right?28:7;ControllerHand hand;hand.enabled=hand.calibrate=true;
   ControllerHand grabbedAtClick;grabbedAtClick.enabled=true;CalibrateControllerHand(grabbedAtClick,pose,positions,right,{1,2,3},{1,0,0,0},{1,0,0,0});Check(grabbedAtClick.origin==DriverIK::V{1,2,3}&&DriverIK::Length(DriverIK::Sub(grabbedAtClick.anchor,before.p[wrist]))<1e-6f&&!grabbedAtClick.calibrate,"grab captures exact controller position and current wrist before later updates");

   {
    auto rigidPose=pose;auto beforeRigid=DriverIK::FK(rigidPose,positions);unsigned wrist=right?28:7;ControllerHand rigidHand;rigidHand.enabled=true;rigidHand.calibrate=true;auto controller=DriverIK::Add(beforeRigid.p[wrist],DriverIK::V{0,.025f,0});
    Check(ApplyControllerHand(rigidPose,positions,right,rigidHand,controller,{1,0,0,0},{1,0,0,0},1,1.f/90.f,{0,0,0},.5f,true),"rigid controller calibration retains wheel grip");
    DriverIK::Q turn{.9961947f,0,0,.0871557f};auto expected=DriverIK::Add(controller,DriverIK::Rotate(turn,DriverIK::Sub(beforeRigid.p[wrist],controller)));
    Check(ApplyControllerHand(rigidPose,positions,right,rigidHand,controller,turn,{1,0,0,0},1,1.f/90.f,{0,0,0},.5f,true),"controller roll rotates calibrated wrist offset");
    Check(DriverIK::Length(DriverIK::Sub(DriverIK::FK(rigidPose,positions).p[wrist],expected))<.001f,"wrist maintains rigid controller offset through rotation");
   }
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
  if(a==b+0x36badd0)value=g;
  else if(a==g+0xaa0)value=o;
  else if(a==o)value=t;
  else if(a==t+0x1b8)value=badOwner?0:b+0x4d9cf0;
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
  if(address==base+0x36badd0)value=game;
  else if(address==game+0xaa0)value=owner;
  else if(address==owner)value=vt;
  else if(address==vt+0x1b8)value=base+(wrongOwner?0x1234:0x4d9cf0);
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

extern "C" {void* gHornControlOriginal=nullptr;unsigned char CaptureHornBox(uintptr_t,uint32_t held) noexcept{return static_cast<unsigned char>(held);}}
