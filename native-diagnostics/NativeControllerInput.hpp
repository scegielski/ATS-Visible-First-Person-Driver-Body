#pragma once
#include <cstdint>
extern "C" void CaptureXRContext(uintptr_t manager) noexcept;
extern "C" uintptr_t PrepareControllerActionAttach(uintptr_t manager,uintptr_t info) noexcept;
bool PollNativeControllerButtons(bool (&pressed)[2],float (&grip)[2],float (&trigger)[2]);
int NativeControllerInputStatus();
