#include <Windows.h>
#include "vendor/minhook/include/MinHook.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
void Check(bool okay){if(!okay){std::puts("early menu hook regression failed");std::exit(1);}}
void Destination(){}
int main(){
 unsigned char mode[]{0x0f,0x28,0xd8,0x48,0x8d,0x4d,0x8c,0x0f,0x57,0xc9,0xf3,0x0f,0x11,0x9d,0,9,0,0,0xc3};
 unsigned char surface[]{0x45,0x8b,0xc6,0x48,0x8d,0x55,0xd4,0x48,0x8b,0xce,0xe8,0x12,0x34,0,0,0x48,0x8d,0x8d,0xa0,3,0,0,0xc3};
 auto page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));Check(page!=nullptr);std::memcpy(page,mode,sizeof(mode));std::memcpy(page+256,surface,sizeof(surface));Check(MH_Initialize()==MH_OK);void* first=nullptr;void* second=nullptr;
 Check(MH_CreateHook(page,reinterpret_cast<void*>(Destination),&first)==MH_OK);Check(MH_CreateHook(page+256,reinterpret_cast<void*>(Destination),&second)==MH_OK);Check(first && second);Check(MH_CreateHook(page,reinterpret_cast<void*>(Destination),&first)==MH_ERROR_ALREADY_CREATED);
 Check(MH_QueueEnableHook(page)==MH_OK && MH_QueueEnableHook(page+256)==MH_OK && MH_ApplyQueued()==MH_OK);Check(page[0]==0xe9 && page[256]==0xe9);Check(MH_DisableHook(page)==MH_OK && MH_DisableHook(page+256)==MH_OK);Check(std::memcmp(page,mode,sizeof(mode))==0 && std::memcmp(page+256,surface,sizeof(surface))==0);Check(MH_RemoveHook(page)==MH_OK && MH_RemoveHook(page+256)==MH_OK);Check(MH_Uninitialize()==MH_OK);VirtualFree(page,0,MEM_RELEASE);std::puts("early menu patch, trampoline, duplicate ownership and restoration passed");
}
