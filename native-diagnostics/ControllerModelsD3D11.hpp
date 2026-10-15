#pragma once
#include "vendor/openxr.h"
#include "ControllerModelData.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <memory>
class ControllerModelsD3D11 {
 struct Impl;std::unique_ptr<Impl> impl;
public:
 ControllerModelsD3D11(XrInstance instance,XrSession session,const XrSessionCreateInfo* info,PFN_xrGetInstanceProcAddr get);
 ~ControllerModelsD3D11();
 XrResult EndFrame(const XrFrameEndInfo* info,PFN_xrEndFrame next);
};
ControllerModelStore& ControllerModelsStore();
