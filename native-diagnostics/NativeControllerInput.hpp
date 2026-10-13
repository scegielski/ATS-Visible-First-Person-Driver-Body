#pragma once
#include <cstdint>
extern "C" void CaptureXRContext(uintptr_t manager) noexcept;
extern "C" uintptr_t PrepareControllerActionAttach(uintptr_t manager,uintptr_t info) noexcept;
bool PollNativeControllerButtons(bool (&pressed)[2],float (&grip)[2],float (&trigger)[2],bool (&mode)[2],bool (&thumb)[2],float (&inward)[2]);
int NativeControllerInputStatus();

int ControllerToggleFeedback(unsigned side);
