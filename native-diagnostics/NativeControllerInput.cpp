#include <Windows.h>
#include "NativeControllerInput.hpp"
#include <atomic>
namespace {std::atomic<int> status{5};}
extern "C" void CaptureXRContext(uintptr_t) noexcept {}
extern "C" uintptr_t PrepareControllerActionAttach(uintptr_t,uintptr_t info) noexcept {return info;}
bool PollNativeControllerButtons(bool (&pressed)[2],float (&grip)[2],float (&trigger)[2]){
 pressed[0]=pressed[1]=false;grip[0]=grip[1]=trigger[0]=trigger[1]=0;auto module=GetModuleHandleW(L"spf_controller_xr_layer.dll");if(!module){status.store(5);return false;}
 using Read=int (__cdecl*)(unsigned char*,unsigned char*,float*);auto read=reinterpret_cast<Read>(GetProcAddress(module,"SPF_ReadControllerInput"));if(!read){status.store(3);return false;}
 unsigned char left=0,right=0;float curls[4]{};int result=read(&left,&right,curls);status.store(result);if(result!=0)return false;pressed[0]=left!=0;pressed[1]=right!=0;grip[0]=curls[0];trigger[0]=curls[1];grip[1]=curls[2];trigger[1]=curls[3];return true;
}
int NativeControllerInputStatus(){return status.load();}
