#include <Windows.h>
#include "NativeControllerInput.hpp"
#include <atomic>
namespace {std::atomic<int> status{5};}
extern "C" void ObserveMarkerXRFrame(uintptr_t) noexcept;
extern "C" void CaptureXRContext(uintptr_t owner) noexcept {ObserveMarkerXRFrame(owner);}
extern "C" uintptr_t PrepareControllerActionAttach(uintptr_t,uintptr_t info) noexcept {return info;}
bool PollNativeControllerButtons(bool (&pressed)[2],float (&grip)[2],float (&trigger)[2],bool (&mode)[2],bool (&thumb)[2],float (&inward)[2]){
 inward[0]=inward[1]=0;pressed[0]=pressed[1]=false;grip[0]=grip[1]=trigger[0]=trigger[1]=0;mode[0]=mode[1]=thumb[0]=thumb[1]=false;auto module=GetModuleHandleW(L"spf_controller_xr_layer.dll");if(!module){status.store(5);return false;}
 using Read=int (__cdecl*)(unsigned char*,unsigned char*,float*,unsigned char*,unsigned char*,unsigned char*,unsigned char*,float*);auto read=reinterpret_cast<Read>(GetProcAddress(module,"SPF_ReadControllerInputWithSelection"));if(!read){status.store(3);return false;}
 unsigned char left=0,right=0,leftMode=0,rightMode=0,leftThumb=0,rightThumb=0;float curls[4]{};int result=read(&left,&right,curls,&leftMode,&rightMode,&leftThumb,&rightThumb,inward);status.store(result);if(result!=0)return false;pressed[0]=left!=0;pressed[1]=right!=0;grip[0]=curls[0];trigger[0]=curls[1];grip[1]=curls[2];trigger[1]=curls[3];mode[0]=leftMode!=0;mode[1]=rightMode!=0;thumb[0]=leftThumb!=0;thumb[1]=rightThumb!=0;return true;
}
int NativeControllerInputStatus(){return status.load();}

int ControllerToggleFeedback(unsigned side){auto module=GetModuleHandleW(L"spf_controller_xr_layer.dll");using Pulse=int (__cdecl*)(unsigned);auto pulse=module?reinterpret_cast<Pulse>(GetProcAddress(module,"SPF_ControllerToggleFeedback")):nullptr;return pulse?pulse(side):5;}
