#include <Windows.h>
#define XR_NO_PROTOTYPES
#include "vendor/openxr_loader_negotiation.h"
#include <mutex>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace {
using EarlyMenuTick=void(__cdecl*)(int);EarlyMenuTick earlyMenuTick=nullptr;
std::recursive_mutex mutex;PFN_xrGetInstanceProcAddr nextGet=nullptr;bool enabled=false;
struct Context {
 XrInstance instance{};XrActionSet set{};XrAction actions[2]{},grip[2]{},trigger[2]{},mode[2]{},thumb[2]{},stick[2]{},haptic[2]{};XrPath touch{};int status=1;XrResult lastSync=XR_SESSION_NOT_FOCUSED;
 PFN_xrGetInstanceProcAddr get{};PFN_xrCreateSession createSession{};PFN_xrDestroySession destroySession{};PFN_xrDestroyInstance destroyInstance{};PFN_xrAttachSessionActionSets attach{};PFN_xrSyncActions sync{};PFN_xrGetActionStateBoolean boolean{};PFN_xrGetActionStateFloat analog{};PFN_xrGetActionStateVector2f vector{};PFN_xrApplyHapticFeedback vibrate{};
 PFN_xrCreateActionSet createSet{};PFN_xrCreateAction createAction{};PFN_xrStringToPath path{};PFN_xrSuggestInteractionProfileBindings suggest{};
 std::vector<XrActionSuggestedBinding> gameTouch;std::vector<XrActiveActionSet> gameActive;
 template<class T> bool Load(const char* name,T& function){PFN_xrVoidFunction f{};auto result=get(instance,name,&f);if(XR_FAILED(result)){status=result;return false;}function=reinterpret_cast<T>(f);return f!=nullptr;}
 bool Init(){return Load("xrCreateSession",createSession)&&Load("xrDestroySession",destroySession)&&Load("xrDestroyInstance",destroyInstance)&&Load("xrAttachSessionActionSets",attach)&&Load("xrSyncActions",sync)&&Load("xrGetActionStateBoolean",boolean)&&Load("xrGetActionStateFloat",analog)&&Load("xrGetActionStateVector2f",vector)&&Load("xrApplyHapticFeedback",vibrate)&&Load("xrCreateActionSet",createSet)&&Load("xrCreateAction",createAction)&&Load("xrStringToPath",path)&&Load("xrSuggestInteractionProfileBindings",suggest);}
 bool Prepare(){
  if(set)return status>=0;
  XrActionSetCreateInfo info{XR_TYPE_ACTION_SET_CREATE_INFO};strcpy_s(info.actionSetName,"spf_driver_controller_hands");strcpy_s(info.localizedActionSetName,"Driver controller hands");auto result=createSet(instance,&info,&set);if(XR_FAILED(result)){status=result;return false;}
  std::vector<XrActionSuggestedBinding> bindings=gameTouch;
  for(unsigned i=0;i<2;++i){XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};strcpy_s(ai.actionName,i?"right_hand_toggle":"left_hand_toggle");strcpy_s(ai.localizedActionName,i?"Right hand calibration":"Left hand calibration");ai.actionType=XR_ACTION_TYPE_BOOLEAN_INPUT;result=createAction(set,&ai,&actions[i]);if(XR_FAILED(result)){status=result;return false;}XrPath click{};result=path(instance,i?"/user/hand/right/input/thumbstick/click":"/user/hand/left/input/thumbstick/click",&click);if(XR_FAILED(result)){status=result;return false;}bindings.push_back({actions[i],click});}

  for(unsigned side=0;side<2;++side)for(unsigned axis=0;axis<2;++axis){
   XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};const char* names[2][2]={{"left_hand_grip","left_hand_trigger"},{"right_hand_grip","right_hand_trigger"}};strcpy_s(ai.actionName,names[side][axis]);strcpy_s(ai.localizedActionName,names[side][axis]);ai.actionType=XR_ACTION_TYPE_FLOAT_INPUT;auto action=axis?&trigger[side]:&grip[side];result=createAction(set,&ai,action);if(XR_FAILED(result)){status=result;return false;}
   const char* paths[2][2]={{"/user/hand/left/input/squeeze/value","/user/hand/left/input/trigger/value"},{"/user/hand/right/input/squeeze/value","/user/hand/right/input/trigger/value"}};XrPath input{};result=path(instance,paths[side][axis],&input);if(XR_FAILED(result)){status=result;return false;}bindings.push_back({*action,input});
  }

  for(unsigned side=0;side<2;++side){XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};strcpy_s(ai.actionName,side?"right_trigger_finger_mode":"left_trigger_finger_mode");strcpy_s(ai.localizedActionName,side?"Right trigger finger mode":"Left trigger finger mode");ai.actionType=XR_ACTION_TYPE_BOOLEAN_INPUT;result=createAction(set,&ai,&mode[side]);if(XR_FAILED(result)){status=result;return false;}XrPath click{};result=path(instance,side?"/user/hand/right/input/a/click":"/user/hand/left/input/x/click",&click);if(XR_FAILED(result)){status=result;return false;}bindings.push_back({mode[side],click});}
  for(unsigned side=0;side<2;++side){XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};strcpy_s(ai.actionName,side?"right_thumb_curl":"left_thumb_curl");strcpy_s(ai.localizedActionName,side?"Right thumb curl":"Left thumb curl");ai.actionType=XR_ACTION_TYPE_BOOLEAN_INPUT;result=createAction(set,&ai,&thumb[side]);if(XR_FAILED(result)){status=result;return false;}XrPath click{};result=path(instance,side?"/user/hand/right/input/b/click":"/user/hand/left/input/y/click",&click);if(XR_FAILED(result)){status=result;return false;}bindings.push_back({thumb[side],click});}
  for(unsigned side=0;side<2;++side){XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};strcpy_s(ai.actionName,side?"right_finger_membership":"left_finger_membership");strcpy_s(ai.localizedActionName,ai.actionName);ai.actionType=XR_ACTION_TYPE_VECTOR2F_INPUT;result=createAction(set,&ai,&stick[side]);if(XR_FAILED(result)){status=result;return false;}XrPath input{};result=path(instance,side?"/user/hand/right/input/thumbstick":"/user/hand/left/input/thumbstick",&input);if(XR_FAILED(result)){status=result;return false;}bindings.push_back({stick[side],input});}
  for(unsigned side=0;side<2;++side){XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};strcpy_s(ai.actionName,side?"right_toggle_feedback":"left_toggle_feedback");strcpy_s(ai.localizedActionName,ai.actionName);ai.actionType=XR_ACTION_TYPE_VIBRATION_OUTPUT;result=createAction(set,&ai,&haptic[side]);if(XR_FAILED(result)){status=result;return false;}XrPath output{};result=path(instance,side?"/user/hand/right/output/haptic":"/user/hand/left/output/haptic",&output);if(XR_FAILED(result)){status=result;return false;}bindings.push_back({haptic[side],output});}
  result=path(instance,"/interaction_profiles/oculus/touch_controller",&touch);if(XR_FAILED(result)){status=result;return false;}XrInteractionProfileSuggestedBinding binding{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};binding.interactionProfile=touch;binding.countSuggestedBindings=static_cast<uint32_t>(bindings.size());binding.suggestedBindings=bindings.data();result=suggest(instance,&binding);status=result;return XR_SUCCEEDED(result);
 }
};
std::unordered_map<XrInstance,std::shared_ptr<Context>> instances;std::unordered_map<XrSession,std::shared_ptr<Context>> sessions;XrSession activeSession{};
XrResult XRAPI_CALL CreateSession(XrInstance instance,const XrSessionCreateInfo* info,XrSession* session){std::lock_guard lock(mutex);auto c=instances.at(instance);auto result=c->createSession(instance,info,session);if(XR_SUCCEEDED(result))sessions[*session]=c;return result;}
XrResult XRAPI_CALL DestroySession(XrSession session){std::lock_guard lock(mutex);auto c=sessions.at(session);auto result=c->destroySession(session);if(XR_SUCCEEDED(result)){sessions.erase(session);if(activeSession==session)activeSession=XR_NULL_HANDLE;}return result;}
XrResult XRAPI_CALL DestroyInstance(XrInstance instance){std::lock_guard lock(mutex);auto c=instances.at(instance);auto result=c->destroyInstance(instance);if(XR_SUCCEEDED(result)){for(auto it=sessions.begin();it!=sessions.end();)if(it->second==c)it=sessions.erase(it);else ++it;instances.erase(instance);activeSession=XR_NULL_HANDLE;}return result;}
XrResult XRAPI_CALL Suggest(XrInstance instance,const XrInteractionProfileSuggestedBinding* info){std::lock_guard lock(mutex);auto c=instances.at(instance);XrPath touch{};if(XR_SUCCEEDED(c->path(instance,"/interaction_profiles/oculus/touch_controller",&touch))&&info->interactionProfile==touch)c->gameTouch.assign(info->suggestedBindings,info->suggestedBindings+info->countSuggestedBindings);return c->suggest(instance,info);}
XrResult XRAPI_CALL Attach(XrSession session,const XrSessionActionSetsAttachInfo* info){std::lock_guard lock(mutex);auto c=sessions.at(session);
 if(!c->Prepare())return c->attach(session,info);
 std::vector<XrActionSet> sets;if(info->countActionSets)sets.assign(info->actionSets,info->actionSets+info->countActionSets);if(std::find(sets.begin(),sets.end(),c->set)==sets.end())sets.push_back(c->set);
 auto combined=*info;combined.countActionSets=static_cast<uint32_t>(sets.size());combined.actionSets=sets.data();auto result=c->attach(session,&combined);c->status=result;if(XR_SUCCEEDED(result))activeSession=session;return result;
}
XrResult XRAPI_CALL Sync(XrSession session,const XrActionsSyncInfo* info){
 XrResult result;{
  std::lock_guard lock(mutex);auto c=sessions.at(session);if(!c->set || c->status<0)return c->sync(session,info);
  c->gameActive.clear();if(info->countActiveActionSets)c->gameActive.assign(info->activeActionSets,info->activeActionSets+info->countActiveActionSets);auto sets=c->gameActive;if(std::none_of(sets.begin(),sets.end(),[&](const auto& a){return a.actionSet==c->set;}))sets.push_back({c->set,XR_NULL_PATH});auto combined=*info;combined.countActiveActionSets=static_cast<uint32_t>(sets.size());combined.activeActionSets=sets.data();result=c->lastSync=c->sync(session,&combined);
 }
 // Notify focus only; early controller work runs independently of the VR frame.
 if(earlyMenuTick)earlyMenuTick(result==XR_SUCCESS?1:0);return result;
}
XrResult XRAPI_CALL GetProc(XrInstance instance,const char* name,PFN_xrVoidFunction* function){std::lock_guard lock(mutex);if(enabled && instance && instances.count(instance)){
 struct Entry {const char* name;PFN_xrVoidFunction function;};const Entry entries[]={{"xrCreateSession",reinterpret_cast<PFN_xrVoidFunction>(CreateSession)},{"xrDestroySession",reinterpret_cast<PFN_xrVoidFunction>(DestroySession)},{"xrDestroyInstance",reinterpret_cast<PFN_xrVoidFunction>(DestroyInstance)},{"xrAttachSessionActionSets",reinterpret_cast<PFN_xrVoidFunction>(Attach)},{"xrSyncActions",reinterpret_cast<PFN_xrVoidFunction>(Sync)},{"xrSuggestInteractionProfileBindings",reinterpret_cast<PFN_xrVoidFunction>(Suggest)}};
 for(const auto& entry:entries)if(!std::strcmp(name,entry.name)){*function=entry.function;return XR_SUCCESS;}
 return instances.at(instance)->get(instance,name,function);
 }return nextGet?nextGet(instance,name,function):XR_ERROR_FUNCTION_UNSUPPORTED;
}
void StartEarlyMenuHost(){
#ifndef SPF_CONTROLLER_LAYER_TEST
 static std::once_flag once;std::call_once(once,[]{
  HMODULE self=nullptr;wchar_t path[MAX_PATH]{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&StartEarlyMenuHost),&self)||!GetModuleFileNameW(self,path,MAX_PATH))return;
  auto slash=wcsrchr(path,L'\\');if(!slash)return;wcscpy_s(slash+1,MAX_PATH-(slash+1-path),L"SPF_VisibleFirstPersonDriverBody.dll");
  // Keep the module reference: the early menu trampolines live there even
  // while SPF probes plugin manifests and temporarily unloads its own reference.
  auto module=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);using Start=bool(__cdecl*)();auto start=module?reinterpret_cast<Start>(GetProcAddress(module,"SPF_StartEarlyMenuSupport")):nullptr;bool ready=start && start();if(ready)earlyMenuTick=reinterpret_cast<EarlyMenuTick>(GetProcAddress(module,"SPF_PollEarlyMenuSupport"));
  slash=wcsrchr(path,L'\\');if(slash){wcscpy_s(slash+1,MAX_PATH-(slash+1-path),L"logs\\controller_menu_early.log");FILE* log=nullptr;if(!_wfopen_s(&log,path,L"a")){SYSTEMTIME t{};GetLocalTime(&t);std::fprintf(log,"%04u-%02u-%02u %02u:%02u:%02u early_menu_ready=%d\n",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,ready?1:0);std::fclose(log);}}
 });
#endif
}
XrResult XRAPI_CALL CreateInstance(const XrInstanceCreateInfo* info,const XrApiLayerCreateInfo* layer,XrInstance* instance){
 if(!layer || !layer->nextInfo)return XR_ERROR_INITIALIZATION_FAILED;
 auto next=*layer;next.nextInfo=layer->nextInfo->next;nextGet=layer->nextInfo->nextGetInstanceProcAddr;auto result=layer->nextInfo->nextCreateApiLayerInstance(info,&next,instance);
 if(XR_SUCCEEDED(result)&&enabled){{std::lock_guard lock(mutex);auto c=std::make_shared<Context>();c->instance=*instance;c->get=nextGet;if(c->Init())instances[*instance]=c;}StartEarlyMenuHost();}
 return result;
}
}
extern "C" __declspec(dllexport) XrResult XRAPI_CALL xrNegotiateLoaderApiLayerInterface(const XrNegotiateLoaderInfo* loader,const char*,XrNegotiateApiLayerRequest* request){
 if(!loader || !request || loader->minInterfaceVersion>1 || loader->maxInterfaceVersion<1 || request->structType!=XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST || request->structSize<sizeof(*request))return XR_ERROR_INITIALIZATION_FAILED;
 wchar_t path[MAX_PATH]{};GetModuleFileNameW(nullptr,path,MAX_PATH);auto file=wcsrchr(path,L'\\');enabled=file && !_wcsicmp(file+1,L"amtrucks.exe");request->layerInterfaceVersion=1;request->layerApiVersion=XR_MAKE_VERSION(1,0,0);request->getInstanceProcAddr=GetProc;request->createApiLayerInstance=CreateInstance;return XR_SUCCESS;
}
extern "C" __declspec(dllexport) int __cdecl SPF_ReadControllerInputWithSelection(unsigned char* left,unsigned char* right,float* curls,unsigned char* leftMode,unsigned char* rightMode,unsigned char* leftThumb,unsigned char* rightThumb,float* inward){
 std::lock_guard lock(mutex);inward[0]=inward[1]=0;*left=*right=*leftMode=*rightMode=*leftThumb=*rightThumb=0;for(unsigned i=0;i<4;++i)curls[i]=0;if(!activeSession || !sessions.count(activeSession))return 5;auto c=sessions.at(activeSession);if(c->status<0)return c->status;
 auto result=c->lastSync;if(result!=XR_SUCCESS)return result;
 bool valid=true;for(unsigned i=0;i<2;++i){XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=c->actions[i];XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};result=c->boolean(activeSession,&gi,&state);if(XR_FAILED(result))return result;valid=valid&&state.isActive;*(i?right:left)=state.isActive&&state.currentState;}
 for(unsigned side=0;side<2;++side)for(unsigned axis=0;axis<2;++axis){XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=axis?c->trigger[side]:c->grip[side];XrActionStateFloat state{XR_TYPE_ACTION_STATE_FLOAT};result=c->analog(activeSession,&gi,&state);if(XR_FAILED(result))return result;valid=valid&&state.isActive&&std::isfinite(state.currentState);if(state.isActive&&std::isfinite(state.currentState))curls[side*2+axis]=std::clamp(state.currentState,0.f,1.f);}
 for(unsigned side=0;side<2;++side){XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=c->mode[side];XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};result=c->boolean(activeSession,&gi,&state);if(XR_FAILED(result))return result;valid=valid&&state.isActive;*(side?rightMode:leftMode)=state.isActive&&state.currentState;}
 for(unsigned side=0;side<2;++side){XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=c->thumb[side];XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};result=c->boolean(activeSession,&gi,&state);if(XR_FAILED(result))return result;valid=valid&&state.isActive;*(side?rightThumb:leftThumb)=state.isActive&&state.currentState;}
 for(unsigned side=0;side<2;++side){XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=c->stick[side];XrActionStateVector2f state{XR_TYPE_ACTION_STATE_VECTOR2F};result=c->vector(activeSession,&gi,&state);if(XR_FAILED(result))return result;valid=valid&&state.isActive&&std::isfinite(state.currentState.x);if(state.isActive&&std::isfinite(state.currentState.x))inward[side]=(side?-1.f:1.f)*state.currentState.x;}
 return valid?0:4;
}
// Full axes share the existing synced action set; preserve older exports.
extern "C" __declspec(dllexport) int __cdecl SPF_ReadControllerStickAxes(float* axes){
 if(!axes)return 5;for(unsigned i=0;i<4;++i)axes[i]=0;
 std::lock_guard lock(mutex);if(!activeSession || !sessions.count(activeSession))return 5;auto c=sessions.at(activeSession);if(c->status<0)return c->status;
 for(unsigned side=0;side<2;++side){XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=c->stick[side];XrActionStateVector2f state{XR_TYPE_ACTION_STATE_VECTOR2F};auto result=c->vector(activeSession,&gi,&state);if(XR_FAILED(result))return result;if(!state.isActive || !std::isfinite(state.currentState.x)||!std::isfinite(state.currentState.y))return 4;axes[side*2]=std::clamp(state.currentState.x,-1.f,1.f);axes[side*2+1]=std::clamp(state.currentState.y,-1.f,1.f);}return 0;
}
extern "C" __declspec(dllexport) int __cdecl SPF_ReadControllerInputWithThumbs(unsigned char* l,unsigned char* r,float* c,unsigned char* lm,unsigned char* rm,unsigned char* lt,unsigned char* rt){float inward[2]{};return SPF_ReadControllerInputWithSelection(l,r,c,lm,rm,lt,rt,inward);}
extern "C" __declspec(dllexport) int __cdecl SPF_ReadControllerInputWithModes(unsigned char* left,unsigned char* right,float* curls,unsigned char* leftMode,unsigned char* rightMode){unsigned char lt=0,rt=0;return SPF_ReadControllerInputWithThumbs(left,right,curls,leftMode,rightMode,&lt,&rt);}
extern "C" __declspec(dllexport) int __cdecl SPF_ReadControllerInput(unsigned char* left,unsigned char* right,float* curls){unsigned char leftMode=0,rightMode=0;return SPF_ReadControllerInputWithModes(left,right,curls,&leftMode,&rightMode);}
extern "C" __declspec(dllexport) int __cdecl SPF_ReadControllerButtons(unsigned char* left,unsigned char* right){float curls[4]{};return SPF_ReadControllerInput(left,right,curls);}
extern "C" __declspec(dllexport) int __cdecl SPF_ControllerToggleFeedback(unsigned side){
 std::lock_guard lock(mutex);if(side>1 || !activeSession || !sessions.count(activeSession))return 5;auto c=sessions.at(activeSession);if(c->status<0)return c->status;
 XrHapticActionInfo info{XR_TYPE_HAPTIC_ACTION_INFO};info.action=c->haptic[side];XrHapticVibration pulse{XR_TYPE_HAPTIC_VIBRATION};pulse.duration=60000000;pulse.frequency=XR_FREQUENCY_UNSPECIFIED;pulse.amplitude=.6f;return c->vibrate(activeSession,&info,reinterpret_cast<const XrHapticBaseHeader*>(&pulse));
}
#ifdef SPF_CONTROLLER_LAYER_TEST
extern "C" void SPF_TestSetEarlyMenuTick(EarlyMenuTick tick){earlyMenuTick=tick;}
extern "C" void SPF_TestControllerLayer(PFN_xrGetInstanceProcAddr get,XrInstance instance,XrSession session){std::lock_guard lock(mutex);instances.clear();sessions.clear();auto c=std::make_shared<Context>();c->get=get;c->instance=instance;c->Init();instances[instance]=c;sessions[session]=c;enabled=true;}
extern "C" XrResult SPF_TestControllerAttach(XrSession s,const XrSessionActionSetsAttachInfo* i){return Attach(s,i);}
extern "C" XrResult SPF_TestControllerSync(XrSession s,const XrActionsSyncInfo* i){return Sync(s,i);}
#endif
