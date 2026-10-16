#pragma once
#include <SPF_Plugin.h>
#include <SPF_Manifest_API.h>
void DescribeIntegratedAdjustments(SPF_Manifest_Builder_Handle*,const SPF_Manifest_Builder_API*);
void LoadIntegratedAdjustments(const SPF_Load_API*);
void ActivateIntegratedAdjustments(const SPF_Core_API*);
void PollIntegratedAdjustments();
void ResetIntegratedAdjustments();
void CloseIntegratedAdjustments();
extern "C" int __cdecl SetIntegratedAdjustmentInput(int,float,float);

void SetIntegratedDisplayObserver(void(*observer)(uintptr_t,int));
