#include "DriverSamples.hpp"
#include "SteeringPose.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
extern "C" {
void* gDriverOriginal=nullptr;
void* gAdvanceOriginal=nullptr;
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
int main() {
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
