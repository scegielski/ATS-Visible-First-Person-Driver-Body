extern "C" int SPF_ReadControllerStickAxes(float*);
#define XR_NO_PROTOTYPES
#include "vendor/openxr.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
extern "C" void SPF_TestControllerLayer(PFN_xrGetInstanceProcAddr,XrInstance,XrSession);
extern "C" XrResult SPF_TestControllerAttach(XrSession,const XrSessionActionSetsAttachInfo*);
extern "C" XrResult SPF_TestControllerSync(XrSession,const XrActionsSyncInfo*);
extern "C" int SPF_ReadControllerButtons(unsigned char*,unsigned char*);
extern "C" int SPF_ReadControllerInput(unsigned char*,unsigned char*,float*);
extern "C" int SPF_ReadControllerInputWithModes(unsigned char*,unsigned char*,float*,unsigned char*,unsigned char*);
extern "C" int SPF_ReadControllerInputWithThumbs(unsigned char*,unsigned char*,float*,unsigned char*,unsigned char*,unsigned char*,unsigned char*);
extern "C" int SPF_ControllerToggleFeedback(unsigned);
namespace {
void Check(bool ok){if(!ok){puts("Controller layer regression failed");std::exit(1);}}
unsigned hapticCount=0;unsigned attachCount=0,syncCount=0;XrActionSet extra=reinterpret_cast<XrActionSet>(9),stock=reinterpret_cast<XrActionSet>(8);XrResult syncResult=XR_SUCCESS;unsigned created=0;
XrResult XRAPI_CALL CreateSet(XrInstance,const XrActionSetCreateInfo*,XrActionSet* out){*out=extra;return XR_SUCCESS;}
XrResult XRAPI_CALL CreateAction(XrActionSet,const XrActionCreateInfo* info,XrAction* out){Check(info->actionType==(created>=12?XR_ACTION_TYPE_VIBRATION_OUTPUT:created>=10?XR_ACTION_TYPE_VECTOR2F_INPUT:(created<2 || created>=6)?XR_ACTION_TYPE_BOOLEAN_INPUT:XR_ACTION_TYPE_FLOAT_INPUT));*out=reinterpret_cast<XrAction>(static_cast<uintptr_t>(++created));return XR_SUCCESS;}
XrResult XRAPI_CALL Path(XrInstance,const char* text,XrPath* out){*out=std::strstr(text,"left")?1:std::strstr(text,"right")?2:3;return XR_SUCCESS;}
XrResult XRAPI_CALL Suggest(XrInstance,const XrInteractionProfileSuggestedBinding* info){Check(info->countSuggestedBindings==14 && info->suggestedBindings[0].binding==1 && info->suggestedBindings[1].binding==2);return XR_SUCCESS;}
XrResult XRAPI_CALL Attach(XrSession,const XrSessionActionSetsAttachInfo* info){++attachCount;Check(info->countActionSets==2 && info->actionSets[0]==stock && info->actionSets[1]==extra);return XR_SUCCESS;}
XrResult XRAPI_CALL Sync(XrSession,const XrActionsSyncInfo* info){++syncCount;Check(info->countActiveActionSets==2 && info->activeActionSets[0].actionSet==stock && info->activeActionSets[1].actionSet==extra);return syncResult;}
XrResult XRAPI_CALL Boolean(XrSession,const XrActionStateGetInfo* info,XrActionStateBoolean* state){state->isActive=XR_TRUE;state->currentState=info->action==reinterpret_cast<XrAction>(1) || info->action==reinterpret_cast<XrAction>(7) || info->action==reinterpret_cast<XrAction>(10);return XR_SUCCESS;}
XrResult XRAPI_CALL Analog(XrSession,const XrActionStateGetInfo* info,XrActionStateFloat* state){state->isActive=XR_TRUE;state->currentState=static_cast<float>(reinterpret_cast<uintptr_t>(info->action)-2)*.25f;return XR_SUCCESS;}
XrResult XRAPI_CALL Vector(XrSession,const XrActionStateGetInfo* info,XrActionStateVector2f* state){state->isActive=XR_TRUE;state->currentState.x=info->action==reinterpret_cast<XrAction>(11)?.8f:-.9f;state->currentState.y=.6f;return XR_SUCCESS;}
XrResult XRAPI_CALL Haptic(XrSession,const XrHapticActionInfo* info,const XrHapticBaseHeader* base){Check(info->action==reinterpret_cast<XrAction>(13+hapticCount));auto pulse=reinterpret_cast<const XrHapticVibration*>(base);Check(pulse->duration==60000000 && pulse->amplitude==.6f);++hapticCount;return XR_SUCCESS;}
XrResult XRAPI_CALL Unused(){return XR_SUCCESS;}
XrResult XRAPI_CALL Get(XrInstance,const char* name,PFN_xrVoidFunction* out){
 struct Entry{const char* name;PFN_xrVoidFunction value;};Entry entries[]={{"xrApplyHapticFeedback",reinterpret_cast<PFN_xrVoidFunction>(Haptic)},{"xrGetActionStateVector2f",reinterpret_cast<PFN_xrVoidFunction>(Vector)},{"xrCreateActionSet",reinterpret_cast<PFN_xrVoidFunction>(CreateSet)},{"xrCreateAction",reinterpret_cast<PFN_xrVoidFunction>(CreateAction)},{"xrStringToPath",reinterpret_cast<PFN_xrVoidFunction>(Path)},{"xrSuggestInteractionProfileBindings",reinterpret_cast<PFN_xrVoidFunction>(Suggest)},{"xrAttachSessionActionSets",reinterpret_cast<PFN_xrVoidFunction>(Attach)},{"xrSyncActions",reinterpret_cast<PFN_xrVoidFunction>(Sync)},{"xrGetActionStateBoolean",reinterpret_cast<PFN_xrVoidFunction>(Boolean)},{"xrGetActionStateFloat",reinterpret_cast<PFN_xrVoidFunction>(Analog)}};for(const auto& e:entries)if(!std::strcmp(name,e.name)){*out=e.value;return XR_SUCCESS;}*out=reinterpret_cast<PFN_xrVoidFunction>(Unused);return XR_SUCCESS;
}
}
int main(){
 auto instance=reinterpret_cast<XrInstance>(1);auto session=reinterpret_cast<XrSession>(2);SPF_TestControllerLayer(Get,instance,session);
 XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};attach.countActionSets=1;attach.actionSets=&stock;
 Check(SPF_TestControllerAttach(session,&attach)==XR_SUCCESS && attachCount==1 && attach.countActionSets==1 && attach.actionSets==&stock);
 XrActiveActionSet stockActive{stock,XR_NULL_PATH};XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO};sync.countActiveActionSets=1;sync.activeActionSets=&stockActive;
 Check(SPF_TestControllerSync(session,&sync)==XR_SUCCESS && sync.countActiveActionSets==1);
 unsigned char left=0,right=0;Check(SPF_ReadControllerButtons(&left,&right)==0 && left==1 && right==0 && syncCount==2);
 float curls[4]{};Check(SPF_ReadControllerInput(&left,&right,curls)==0 && curls[0]==.25f && curls[1]==.5f && curls[2]==.75f && curls[3]==1.f);
 unsigned char leftMode=0,rightMode=0;Check(SPF_ReadControllerInputWithModes(&left,&right,curls,&leftMode,&rightMode)==0 && leftMode==1 && rightMode==0);
 unsigned char lt=0,rt=0;Check(SPF_ReadControllerInputWithThumbs(&left,&right,curls,&leftMode,&rightMode,&lt,&rt)==0 && lt==0 && rt==1);
 float axes[4]{};Check(SPF_ReadControllerStickAxes(axes)==0 && axes[0]==.8f && axes[1]==.6f && axes[2]==-.9f && axes[3]==.6f);
 Check(SPF_ControllerToggleFeedback(0)==0 && SPF_ControllerToggleFeedback(1)==0 && hapticCount==2);Check(SPF_ControllerToggleFeedback(2)==5);
 syncResult=XR_SESSION_NOT_FOCUSED;Check(SPF_ReadControllerButtons(&left,&right)==XR_SESSION_NOT_FOCUSED && left==0 && right==0);
 puts("Controller layer attachment, game-input preservation, per-hand states and focus checks passed.");
}

