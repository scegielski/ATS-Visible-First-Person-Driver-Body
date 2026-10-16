#include <set>
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
int displayObserved=0,displayBefore=-1,displayCallCount=0;uintptr_t displayContext=0;
int calls=0; void* passed=nullptr; float passedDt=0; uintptr_t scoped=0;unsigned scopedSlot=0;unsigned char scopedMask=0;
void __fastcall StubUpdate(void* context,float dt) { ++calls; passed=context; passedDt=dt; scoped=gOverrideContext;scopedSlot=gOverrideSlot;scopedMask=gInputMask; }
template<class T,size_t N> void Put(std::array<unsigned char,N>& bytes,size_t off,T value) { std::memcpy(bytes.data()+off,&value,sizeof(value)); }
}
extern "C" int RunSeatUiStub(uintptr_t,unsigned char*);
extern "C" void TestSeatUiPass();
extern "C" void TestSeatUiBypass();
int main(int argc,char** argv) {
 std::set<std::string> bindingOwnership{"Gestures.LeftWindowBird","Move.left","Select.right","Setup.EditInteractionBoxes"};std::vector<std::string> removedBindings;RetireControllerKeybinds([&](const char* name){bindingOwnership.insert(name);},[&](const char* name){if(bindingOwnership.erase(name))removedBindings.push_back(name);});CHECK(removedBindings.size()==std::size(RetiredControllerKeybinds)&&bindingOwnership.size()==1&&!bindingOwnership.count("Move.left")&&bindingOwnership.count("Gestures.LeftWindowBird"));for(auto name:RetiredControllerKeybinds)CHECK(!bindingOwnership.count(name));

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
 gSeatUiOriginal=reinterpret_cast<void*>(TestSeatUiPass); gSeatApplyReturn=reinterpret_cast<void*>(TestSeatUiBypass);
 unsigned char carry=0; gSyntheticSeatContext=123;
 CHECK(RunSeatUiStub(123,&carry)==22 && carry==1);
 CHECK(RunSeatUiStub(124,&carry)==11 && carry==1);
 gSyntheticSeatContext=0; CHECK(RunSeatUiStub(0,&carry)==11 && carry==1);

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
 gOverrideContext=0;
 std::array<float,36> entries{}; for(int i=0;i<9;++i) entries[i*4+3]=1;
 const auto beforeEntries=entries;
 std::array<unsigned char,0x130> context{}; std::array<unsigned char,0x210> actor{},mirrors{};
 Put(context,0x118,reinterpret_cast<uintptr_t>(actor.data())); Put(actor,0x1f8,reinterpret_cast<uintptr_t>(mirrors.data()));
 Put(mirrors,0x1d0,reinterpret_cast<uintptr_t>(entries.data())); Put(mirrors,0x1d8,uint64_t{9});
 const auto beforeContext=context; const auto beforeActor=actor; const auto beforeMirrors=mirrors;
 observing=true; originalUpdate=StubUpdate;
 controllerMirrorLease.Put(-1,0,0,GetTickCount64()); movementAllowed=true; DetourUpdate(context.data(),0.016f);
 CHECK(calls==1 && passed==context.data() && passedDt==0.016f && scoped==0 && gOverrideContext==0);
 controllerMirrorLease.Put(4,0,0,GetTickCount64()); DetourUpdate(context.data(),0.016f);
 CHECK(calls==2 && scoped==reinterpret_cast<uintptr_t>(context.data()) && gOverrideContext==0);
 movementAllowed=false; DetourUpdate(context.data(),0.016f); CHECK(calls==3 && scoped==0);
 movementAllowed=true; DetourUpdate(context.data(),0.5f); CHECK(calls==4 && scoped==0);
 Put(mirrors,0x1d8,uint64_t{3}); DetourUpdate(context.data(),0.016f); CHECK(calls==5 && scoped==0);
 Put(mirrors,0x1d8,uint64_t{9});
 CHECK(context==beforeContext && actor==beforeActor && mirrors==beforeMirrors && entries==beforeEntries);
 controllerMirrorLease.Put(2,.8f,-.9f,GetTickCount64());DetourUpdate(context.data(),.016f);CHECK(calls==6 && scoped==reinterpret_cast<uintptr_t>(context.data()) && scopedSlot==2 && scopedMask==10 && gOverrideContext==0 && gInputMask==0);
 int audioSlot=-1;unsigned char audioMask=0;CHECK(controllerMirrorLease.Get(GetTickCount64(),audioSlot,audioMask) && audioSlot==2 && ServoMovement(audioSlot,audioMask,true,true) && ServoChannelVolume(audioSlot)==0x40000000);
 controllerMirrorLease.Put(0,0,0,GetTickCount64());CHECK(controllerMirrorLease.Get(GetTickCount64(),audioSlot,audioMask) && !ServoMovement(audioSlot,audioMask,true,true));
 controllerMirrorLease.Put(-1,0,0,GetTickCount64());DetourUpdate(context.data(),.016f);CHECK(calls==7 && scoped==0);
 std::array<unsigned char,0x15f0> displayOwner{};Put(displayOwner,0x15d4,int{3});SetIntegratedDisplayObserver([](uintptr_t owner,int before){++displayObserved;displayBefore=before;displayContext=owner;displayCallCount=calls;});observing=false;movementAllowed=false;DetourUpdate(displayOwner.data(),.016f);CHECK(displayObserved==1&&displayBefore==3&&displayContext==reinterpret_cast<uintptr_t>(displayOwner.data())&&displayCallCount==8&&calls==8);SetIntegratedDisplayObserver(nullptr);DetourUpdate(displayOwner.data(),.016f);CHECK(displayObserved==1&&calls==9);
 int wheelSlot=-1;unsigned char wheelMask=0;controllerMirrorLease.Put(9,.8f,-.9f,100);CHECK(controllerMirrorLease.Get(101,wheelSlot,wheelMask) && wheelSlot==9 && wheelMask==10 && ServoMovement(wheelSlot,wheelMask,true,true));
 int leaseSlot=-1;unsigned char leaseMask=0;controllerMirrorLease.Put(4,-.8f,.9f,100);CHECK(controllerMirrorLease.Get(150,leaseSlot,leaseMask) && leaseSlot==4 && leaseMask==5);CHECK(!controllerMirrorLease.Get(251,leaseSlot,leaseMask));controllerMirrorLease.Put(-1,0,0,100);CHECK(!controllerMirrorLease.Get(101,leaseSlot,leaseMask));
 std::cout<<"PASS: native adjustment stubs, register preservation, controller leases, pause guards, seat bounds, servo audio, retired binding migration, and shared display observer.\n";
}
