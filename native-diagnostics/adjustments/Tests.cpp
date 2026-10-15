#include <algorithm>
#include "MirrorControls.cpp"
#include <iostream>
#include <cstdlib>
#define CHECK(x) do { if(!(x)) { std::cerr<<"FAIL: "<<#x<<" line "<<__LINE__<<'\n'; std::exit(1); } } while(false)
extern "C" void RunSlotStubs(uintptr_t,void*);
extern "C" void TestSelectReturn();
extern "C" void RunInputStubs(uintptr_t,void*);
extern "C" void TestSlotReturn();
namespace {
int cueCalls=0; std::wstring lastCue; DWORD cueFlags=0;
int clickCalls=0; int lastClickSlot=-1;
void StubClick(int slot) { ++clickCalls; lastClickSlot=slot; }
BOOL WINAPI StubCue(LPCWSTR path,HMODULE,DWORD flags) { ++cueCalls; lastCue=path?path:L""; cueFlags=flags; return TRUE; }
int calls=0; void* passed=nullptr; float passedDt=0; uintptr_t scoped=0;unsigned scopedSlot=0;unsigned char scopedMask=0;
void __fastcall StubUpdate(void* context,float dt) { ++calls; passed=context; passedDt=dt; scoped=gOverrideContext;scopedSlot=gOverrideSlot;scopedMask=gInputMask; }
template<class T,size_t N> void Put(std::array<unsigned char,N>& bytes,size_t off,T value) { std::memcpy(bytes.data()+off,&value,sizeof(value)); }
}
extern "C" int RunSeatUiStub(uintptr_t,unsigned char*);
extern "C" void TestSeatUiPass();
extern "C" void TestSeatUiBypass();
int main(int argc,char** argv) {
 if(argc>1) {
  std::ifstream clip(argv[1],std::ios::binary); ServoWaveHeader header{};
  CHECK(clip.read(reinterpret_cast<char*>(&header),sizeof(header)) && ValidServoHeader(header));
  std::vector<char> pcm(header.bytes); CHECK(clip.read(pcm.data(),pcm.size()));
  CHECK(std::any_of(pcm.begin(),pcm.end(),[](char value){ return value!=0; }));
 }

 // Resolve native defaults from live-shaped memory without opening F4.
 std::array<unsigned char,0x31b8> world{};
 std::array<unsigned char,0x28> service{};
 std::array<unsigned char,0x200> seatActor{};
 std::array<unsigned char,0x220> settings{};
 std::array<unsigned char,0x12a4> config{};
 std::array<unsigned char,0x48> manager{};
 std::array<unsigned char,0x5d0> camera{};
 std::array<uintptr_t,3> cameras={0,0,reinterpret_cast<uintptr_t>(camera.data())};
 Put(world,0x31b0,reinterpret_cast<uintptr_t>(service.data()));
 Put(service,8,uint32_t{0x80000000}); Put(service,0x18,reinterpret_cast<uintptr_t>(seatActor.data()));
 Put(service,0x20,reinterpret_cast<uintptr_t>(config.data())); Put(seatActor,0x1f8,reinterpret_cast<uintptr_t>(settings.data()));
 Put(manager,0x38,reinterpret_cast<uintptr_t>(cameras.data())); Put(manager,0x40,uint64_t{3});
 Put(camera,8,uint32_t{0x80000000});
 for(size_t i=0;i<3;++i) { Put(camera,0x4a4+i*4,float(i+1)); Put(camera,0x498+i*4,99.f); }
 const size_t bounds[]={0x1284,0x1290,0x1294,0x1288,0x128c,0x1298};
 for(size_t i=0;i<6;++i) Put(config,bounds[i],float(i%3+1)+(i<3?-0.25f:0.25f));
 Put(camera,0x5b0,4.f); Put(camera,0x5c0,5.f); Put(camera,0x408,6.f);
 Put(config,0x129c,7.f); Put(config,0x12a0,8.f); Put(camera,0x4b1,static_cast<unsigned char>(1));
 std::array<unsigned char,0x400> automatic{}; uintptr_t identity=0;
 auto readDefaults=[&] { return ReadSeatDefaults(reinterpret_cast<uintptr_t>(world.data()),reinterpret_cast<uintptr_t>(manager.data()),automatic,identity); };
 CHECK(readDefaults() && identity==reinterpret_cast<uintptr_t>(settings.data()));
 CHECK(SeatFloat(automatic,0x39c)==1 && SeatFloat(automatic,0x3a0)==2 && SeatFloat(automatic,0x3a4)==3);
 CHECK(SeatFloat(automatic,0x388)==1.75f && SeatFloat(automatic,0x398)==3.25f);
 CHECK(SeatFloat(automatic,0x3b4)==4 && SeatFloat(automatic,0x3c4)==5 && SeatFloat(automatic,0x3d4)==6 && SeatFloat(automatic,0x3e4)==7 && SeatFloat(automatic,0x3f4)==8 && automatic[0x3f8]==1);
 CHECK(PrepareSeatValues(automatic,{0.f,0.1f,0.1f,1.f,2.f,3.f,4.f,5.f},0.1f,4));
 CHECK(std::abs(SeatFloat(automatic,0x37c)-2.11f)<0.00001f && SeatFloat(automatic,0x3c8)==7 && SeatFloat(automatic,0x3a8)==8);
 Put(manager,0x40,uint64_t{2}); CHECK(!readDefaults()); Put(manager,0x40,uint64_t{65}); CHECK(!readDefaults()); Put(manager,0x40,uint64_t{3});
 Put(camera,8,uint32_t{0}); CHECK(!readDefaults()); Put(camera,8,uint32_t{0x80000000});
 Put(service,8,uint32_t{0}); CHECK(!readDefaults()); Put(service,8,uint32_t{0x80000000});
 Put(config,0x1290,9.f); CHECK(!readDefaults()); Put(config,0x1290,1.75f);
 cameras[2]=1; CHECK(!readDefaults()); cameras[2]=reinterpret_cast<uintptr_t>(camera.data()); CHECK(readDefaults());
 std::array<unsigned char,0x400> seat{};
 for(size_t axis=0;axis<3;++axis) { SeatPut(seat,0x384+axis*4,-0.1f); SeatPut(seat,0x390+axis*4,0.1f); SeatPut(seat,0x39c+axis*4,0.f); }
 std::array<float,8> seatCurrent={0.02f,0.01f,-0.01f,1.f,2.f,3.f,4.f,5.f};
 CHECK(PrepareSeatValues(seat,seatCurrent,0.1f,4));
 CHECK(std::abs(SeatFloat(seat,0x37c)-0.02f)<0.00001f && SeatFloat(seat,0x378)==0.02f);
 CHECK(SeatFloat(seat,0x3c8)==1.f && SeatFloat(seat,0x3d8)==2.f && SeatFloat(seat,0x3e8)==3.f && SeatFloat(seat,0x3a8)==4.f && SeatFloat(seat,0x3b8)==5.f);
 CHECK(PrepareSeatValues(seat,seatCurrent,0.1f,1)); CHECK(std::abs(SeatFloat(seat,0x380)+0.02f)<0.00001f);
 seatCurrent[1]=0.1f; CHECK(PrepareSeatValues(seat,seatCurrent,0.1f,4)); CHECK(SeatFloat(seat,0x37c)==0.1f);
 CHECK(!PrepareSeatValues(seat,seatCurrent,0.2f,4)); CHECK(!PrepareSeatValues(seat,seatCurrent,0.1f,12));
 CHECK(PrepareSeatValues(seat,seatCurrent,0.1f,16)); CHECK(SeatFloat(seat,0x37c)==0 && SeatFloat(seat,0x380)==0 && SeatFloat(seat,0x378)==0.02f);
 // Steering changes only its own native field; reset and bounds are independent.
 SeatPut(seat,0x3e4,0.5f); SeatPut(seat,0x3f4,0.5f);
 SeatPut(seat,0x3dc,0.f); SeatPut(seat,0x3e0,1.f); SeatPut(seat,0x3ec,0.f); SeatPut(seat,0x3f0,1.f);
 seatCurrent={0.02f,0.01f,-0.01f,0.1f,0.2f,0.3f,0.4f,0.5f};
 CHECK(PrepareSeatValues(seat,seatCurrent,0.1f,2,7));
 CHECK(std::abs(SeatFloat(seat,0x3e8)-0.825f)<0.00001f && SeatFloat(seat,0x3d8)==0.7f);
 CHECK(SeatFloat(seat,0x37c)==0.01f && SeatFloat(seat,0x380)==-0.01f && SeatFloat(seat,0x3c8)==0.1f);
 CHECK(!PrepareSeatValues(seat,seatCurrent,0.1f,4,7));
 CHECK(PrepareSeatValues(seat,seatCurrent,0.1f,4,8)); CHECK(std::abs(SeatFloat(seat,0x3d8)-0.725f)<0.00001f);
 CHECK(PrepareSeatValues(seat,seatCurrent,0.1f,16,7)); CHECK(SeatFloat(seat,0x3e8)==0.5f && SeatFloat(seat,0x3d8)==0.7f);
 seatCurrent[5]=0.5f; CHECK(PrepareSeatValues(seat,seatCurrent,0.1f,2,7)); CHECK(SeatFloat(seat,0x3e8)==1.f);
 CHECK(ResolveHeldSelection(-1,16,0)==7); CHECK(ResolveHeldSelection(7,0,16)==-1);
 CHECK(ResolveHeldSelection(-1,32,0)==8); CHECK(ResolveHeldSelection(8,0,32)==-1);
 gSeatUiOriginal=reinterpret_cast<void*>(TestSeatUiPass); gSeatApplyReturn=reinterpret_cast<void*>(TestSeatUiBypass);
 unsigned char carry=0; gSyntheticSeatContext=123;
 CHECK(RunSeatUiStub(123,&carry)==22 && carry==1);
 CHECK(RunSeatUiStub(124,&carry)==11 && carry==1);
 gSyntheticSeatContext=0; CHECK(RunSeatUiStub(0,&carry)==11 && carry==1);

 voiceCuesEnabled=true; playCueSound=StubCue; cuePaths={L"left.wav",L"center.wav",L"right.wav",L"seat.wav",L"off.wav",L"steering_extension.wav",L"steering_tilt.wav"};
 playSelectionClick=StubClick;
 CHECK(CueIndex(0)==0 && CueIndex(4)==1 && CueIndex(2)==2 && CueIndex(-1)==-1);
 UpdateVoiceCue(0); CHECK(cueCalls==1 && lastCue==L"left.wav" && (cueFlags&SND_ASYNC));
 UpdateVoiceCue(0); CHECK(cueCalls==1);
 UpdateVoiceCue(4); CHECK(cueCalls==2 && lastCue==L"center.wav");
 UpdateVoiceCue(2); CHECK(cueCalls==3 && lastCue==L"right.wav");
 UpdateVoiceCue(-1); CHECK(cueCalls==4 && lastCue==L"off.wav" && lastClickSlot==-1);
 const int afterExitClicks=clickCalls; UpdateVoiceCue(-1); CHECK(cueCalls==4 && clickCalls==afterExitClicks);
 voiceCuesEnabled=false; UpdateVoiceCue(0); CHECK(cueCalls==4);
 UpdateVoiceCue(-1); CHECK(cueCalls==4 && clickCalls==afterExitClicks+2 && lastClickSlot==-1);
 StopVoiceCue(); CHECK(cueCalls==5 && lastCue.empty());
 voiceCuesEnabled=true; UpdateVoiceCue(0); CHECK(cueCalls==6 && lastCue==L"left.wav");
 const int beforeRepeat=clickCalls; UpdateVoiceCue(0); CHECK(clickCalls==beforeRepeat);
 UpdateVoiceCue(4); CHECK(clickCalls==beforeRepeat+1 && lastClickSlot==4);
 UpdateVoiceCue(2); CHECK(clickCalls==beforeRepeat+2 && lastClickSlot==2);
 UpdateVoiceCue(-1); CHECK(clickCalls==beforeRepeat+3 && lastClickSlot==-1 && lastCue==L"off.wav");
 selectionClicksEnabled=false; const int mutedClicks=clickCalls;
 UpdateVoiceCue(0); UpdateVoiceCue(-1); CHECK(clickCalls==mutedClicks && lastCue==L"off.wav");
 selectionClicksEnabled=true;
 UpdateVoiceCue(6); CHECK(lastCue==L"seat.wav" && lastClickSlot==6);
 UpdateVoiceCue(-1); CHECK(lastCue==L"off.wav" && lastClickSlot==-1);
 // Exit feedback is emitted synchronously even without another OnUpdate tick.
 UpdateVoiceCue(7); CHECK(lastCue==L"steering_extension.wav" && lastClickSlot==7);
 UpdateVoiceCue(8); CHECK(lastCue==L"steering_tilt.wav" && lastClickSlot==8);
 UpdateVoiceCue(6); selectedSlot=6; const int beforeExit=cueCalls;
 ExitCenter(); CHECK(selectedSlot==-1 && cueCalls==beforeExit+1 && lastCue==L"off.wav" && lastClickSlot==-1);
 ExitCenter(); UpdateVoiceCue(-1); CHECK(cueCalls==beforeExit+1);
 CHECK(ResolveHeldSelection(-1,8,0)==6); CHECK(ResolveHeldSelection(6,0,8)==-1);
 const auto saved=nlohmann::json::parse(R"json({"keybinds":{"SPF_VisibleFirstPersonDriverBody.Select":{"left":{"bindings":[]},"cycle":{"bindings":[{"key":"KEY_1","type":"keyboard"}]}}}})json");
 CHECK(!DefaultBindingNeeded(saved,"Select","left"));
 CHECK(!DefaultBindingNeeded(saved,"Select","cycle"));
 CHECK(DefaultBindingNeeded(saved,"Select","right"));
 CHECK(DefaultBindingNeeded(nlohmann::json::object(),"Select","left"));
 const auto malformed=nlohmann::json::parse(R"json({"keybinds":{"SPF_VisibleFirstPersonDriverBody.Select":{"left":{"bindings":null}}}})json");
 CHECK(DefaultBindingNeeded(malformed,"Select","left"));
 const auto fixture=std::filesystem::temp_directory_path()/"spf-mirror-persistence-regression.json";
 { std::ofstream file(fixture); file<<saved.dump(); }
 for(int run=0;run<2;++run) { const auto reloaded=ReadSavedSettings(fixture); CHECK(!DefaultBindingNeeded(reloaded,"Select","left")); CHECK(!DefaultBindingNeeded(reloaded,"Select","cycle")); }
 std::filesystem::remove(fixture);
 CHECK(ServoChannelVolume(0)==0x00004000); CHECK(ServoChannelVolume(4)==0x40004000); CHECK(ServoChannelVolume(2)==0x40000000); CHECK(ServoChannelVolume(-1)==0);
 CHECK(NextServoVolume(0x40004000,-1)==0x40004000); CHECK(NextServoVolume(0x40000000,-1)==0x40000000); CHECK(NextServoVolume(0x40004000,0)==0x00004000);
 CHECK(ServoMovement(0,1,true,true)); CHECK(ServoMovement(4,4,true,true));
 CHECK(!ServoMovement(0,0,true,true)); CHECK(!ServoMovement(-1,1,true,true));
 CHECK(!ServoMovement(0,3,true,true)); CHECK(!ServoMovement(0,12,true,true));
 CHECK(ServoMovement(7,2,true,true)); CHECK(!ServoMovement(7,4,true,true));
 CHECK(ServoMovement(8,4,true,true)); CHECK(!ServoMovement(8,2,true,true));
 CHECK(!ServoMovement(0,16,true,true)); CHECK(!ServoMovement(0,1,false,true)); CHECK(!ServoMovement(0,1,true,false));
 ServoWaveHeader wav{}; std::memcpy(wav.riff,"RIFF",4); std::memcpy(wav.wave,"WAVE",4); std::memcpy(wav.fmt,"fmt ",4); std::memcpy(wav.data,"data",4);
 wav.fmtSize=16; wav.format=1; wav.channels=1; wav.bits=16; wav.rate=44100; wav.align=2; wav.byteRate=88200; wav.bytes=44100;
 CHECK(ValidServoHeader(wav)); wav.channels=2; wav.align=4; wav.byteRate=176400; CHECK(ValidServoHeader(wav)); wav.bytes=1048578; CHECK(!ValidServoHeader(wav)); wav.bytes=3; CHECK(!ValidServoHeader(wav));
 int invalidRead=0; CHECK(!SafeRead(1,&invalidRead,sizeof(invalidRead))); CHECK(Slot(0x1040,0x1000,9)==4); CHECK(Slot(0x1041,0x1000,9)==-1);
 gSelectOriginal=reinterpret_cast<void*>(TestSelectReturn); gSlotOriginal=reinterpret_cast<void*>(TestSlotReturn);
 alignas(8) unsigned char out[16]{};
 gOverrideContext=0; RunSlotStubs(0x1234,out); CHECK(out[0]==0 && *reinterpret_cast<int*>(out+4)==2 && out[8]==1 && out[9]==1);
 gOverrideSlot=4; gOverrideContext=0x1234; RunSlotStubs(0x1234,out); CHECK(out[0]==1 && *reinterpret_cast<int*>(out+4)==4 && out[8]==1 && out[9]==1);
 RunSlotStubs(0x5678,out); CHECK(out[0]==0 && *reinterpret_cast<int*>(out+4)==2 && out[8]==1 && out[9]==1);
 gOverrideContext=0; RunSlotStubs(0,out); CHECK(out[0]==0 && *reinterpret_cast<int*>(out+4)==2);
 for(int slot:{0,2,4}) {
  gOverrideContext=0x1234; gOverrideSlot=slot; RunSlotStubs(0x1234,out);
  CHECK(out[0]==1 && *reinterpret_cast<int*>(out+4)==slot && out[8]==1 && out[9]==1);
 }
 gLeftOriginal=gRightOriginal=gUpOriginal=gDownOriginal=gResetOriginal=reinterpret_cast<void*>(TestSelectReturn);
 for(unsigned int mask=0;mask<32;++mask) {
  gOverrideContext=0x1234; gInputMask=static_cast<unsigned char>(mask); RunInputStubs(0x1234,out);
  for(int i=0;i<5;++i) CHECK(out[i]==((mask>>i)&1) && out[8+i]==1);
 }
 gOverrideContext=0; gInputMask=0; RunInputStubs(0x1234,out); for(int i=0;i<5;++i) CHECK(out[i]==1);
 gOverrideContext=0x1234; RunInputStubs(0x5678,out); for(int i=0;i<5;++i) CHECK(out[i]==1);
 for(int current:{-1,0,2,4}) for(int requested:{0,2,4}) CHECK(NextSelection(current,requested)==(current==requested?-1:requested));
 gOverrideContext=0;
 CHECK(ResolveHeldSelection(-1,1,0)==0);
 CHECK(ResolveHeldSelection(0,1,1)==0);
 CHECK(ResolveHeldSelection(0,0,1)==-1);
 CHECK(ResolveHeldSelection(0,3,1)==2);
 CHECK(ResolveHeldSelection(2,1,3)==0);
 CHECK(ResolveHeldSelection(2,7,3)==4);
 CHECK(ResolveHeldSelection(4,3,7)==2);
 CHECK(ResolveHeldSelection(-1,4,0)==4);
 CHECK(ResolveHeldSelection(-1,7,0)==4);
 CHECK(ResolveHeldSelection(4,0,4)==-1);
 CHECK(NextCycleSlot(-1,false)==0); CHECK(NextCycleSlot(0,false)==4); CHECK(NextCycleSlot(4,false)==2); CHECK(NextCycleSlot(2,false)==6); CHECK(NextCycleSlot(6,false)==7); CHECK(NextCycleSlot(7,false)==8); CHECK(NextCycleSlot(8,false)==-1);
 CHECK(NextCycleSlot(-1,true)==0); CHECK(NextCycleSlot(0,true)==4); CHECK(NextCycleSlot(4,true)==2); CHECK(NextCycleSlot(2,true)==6); CHECK(NextCycleSlot(6,true)==7); CHECK(NextCycleSlot(7,true)==8); CHECK(NextCycleSlot(8,true)==0);
 CHECK(ResolveHeldWithCycle(4,0,0,true,4)==4);
 CHECK(ResolveHeldWithCycle(4,0,0,false,4)==-1);
 CHECK(ResolveHeldWithCycle(4,1,0,true,4)==0);
 CHECK(ResolveHeldWithCycle(4,1,1,false,4)==0);
 holdSelectors=true; selectedSlot=-1; ToggleLeft(); ToggleRight(); ToggleCenter(); CHECK(selectedSlot==-1);
 holdSelectors=false;
 std::array<float,36> entries{}; for(int i=0;i<9;++i) entries[i*4+3]=1;
 const auto beforeEntries=entries;
 std::array<unsigned char,0x130> context{}; std::array<unsigned char,0x210> actor{},mirrors{};
 Put(context,0x118,reinterpret_cast<uintptr_t>(actor.data())); Put(actor,0x1f8,reinterpret_cast<uintptr_t>(mirrors.data()));
 Put(mirrors,0x1d0,reinterpret_cast<uintptr_t>(entries.data())); Put(mirrors,0x1d8,uint64_t{9});
 const auto beforeContext=context; const auto beforeActor=actor; const auto beforeMirrors=mirrors;
 observing=true; originalUpdate=StubUpdate;
 selectedSlot=-1; movementAllowed=true; DetourUpdate(context.data(),0.016f);
 CHECK(calls==1 && passed==context.data() && passedDt==0.016f && scoped==0 && gOverrideContext==0);
 selectedSlot=4; DetourUpdate(context.data(),0.016f);
 CHECK(calls==2 && scoped==reinterpret_cast<uintptr_t>(context.data()) && gOverrideContext==0);
 movementAllowed=false; DetourUpdate(context.data(),0.016f); CHECK(calls==3 && scoped==0);
 movementAllowed=true; DetourUpdate(context.data(),0.5f); CHECK(calls==4 && scoped==0);
 Put(mirrors,0x1d8,uint64_t{3}); DetourUpdate(context.data(),0.016f); CHECK(calls==5 && scoped==0);
 Put(mirrors,0x1d8,uint64_t{9});
 CHECK(context==beforeContext && actor==beforeActor && mirrors==beforeMirrors && entries==beforeEntries);
 selectedSlot=-1;controllerMirrorLease.Put(2,.8f,-.9f,GetTickCount64());DetourUpdate(context.data(),.016f);CHECK(calls==6 && scoped==reinterpret_cast<uintptr_t>(context.data()) && scopedSlot==2 && scopedMask==10 && gOverrideContext==0 && gInputMask==0);
 int audioSlot=-1;unsigned char audioMask=0;CHECK(controllerMirrorLease.Get(GetTickCount64(),audioSlot,audioMask) && audioSlot==2 && ServoMovement(audioSlot,audioMask,true,true) && ServoChannelVolume(audioSlot)==0x40000000);
 controllerMirrorLease.Put(0,0,0,GetTickCount64());CHECK(controllerMirrorLease.Get(GetTickCount64(),audioSlot,audioMask) && !ServoMovement(audioSlot,audioMask,true,true));
 controllerMirrorLease.Put(-1,0,0,GetTickCount64());DetourUpdate(context.data(),.016f);CHECK(calls==7 && scoped==0);
 int wheelSlot=-1;unsigned char wheelMask=0;controllerMirrorLease.Put(9,.8f,-.9f,100);CHECK(controllerMirrorLease.Get(101,wheelSlot,wheelMask) && wheelSlot==9 && wheelMask==10 && ServoMovement(wheelSlot,wheelMask,true,true));
 int leaseSlot=-1;unsigned char leaseMask=0;controllerMirrorLease.Put(4,-.8f,.9f,100);CHECK(controllerMirrorLease.Get(150,leaseSlot,leaseMask) && leaseSlot==4 && leaseMask==5);CHECK(!controllerMirrorLease.Get(251,leaseSlot,leaseMask));controllerMirrorLease.Put(-1,0,0,100);CHECK(!controllerMirrorLease.Get(101,leaseSlot,leaseMask));
 ExitCenter(); CHECK(selectedSlot.load()==-1); CHECK(!HooksReady()); ToggleCenter(); CHECK(selectedSlot.load()==-1);
 std::cout<<"PASS: real assembly stubs select all mirror slots only for matching scoped context; flags preserved; side/default pass-through; one native call with unchanged arguments; override cleared; input override masks, toggle/switch and hold/release/overlap and cycle transitions, voice cue mapping and no-repeat/mute/asynchronous playback, persisted assigned/cleared bindings across reloads, servo movement/mute/cancel guards and WAV validation and left/center/right channel mapping, disallowed/large-dt/short-array guards; host metadata and rotations untouched by wrapper.\n";
}
