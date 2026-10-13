#include <Windows.h>
#include "NativeControllerInput.hpp"
#include <atomic>
namespace {std::atomic<int> status{5};}
extern "C" void CaptureXRContext(uintptr_t) noexcept {}
extern "C" uintptr_t PrepareControllerActionAttach(uintptr_t,uintptr_t info) noexcept {return info;}
bool PollNativeControllerButtons(bool (&pressed)[2]){
 pressed[0]=pressed[1]=false;auto module=GetModuleHandleW(L"spf_controller_xr_layer.dll");if(!module){status.store(5);return false;}
 using Read=int (__cdecl*)(unsigned char*,unsigned char*);auto read=reinterpret_cast<Read>(GetProcAddress(module,"SPF_ReadControllerButtons"));if(!read){status.store(3);return false;}
 unsigned char left=0,right=0;int result=read(&left,&right);status.store(result);if(result!=0)return false;pressed[0]=left!=0;pressed[1]=right!=0;return true;
}
int NativeControllerInputStatus(){return status.load();}
