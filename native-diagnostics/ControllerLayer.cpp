#include <Windows.h>
#define XR_NO_PROTOTYPES
#include "vendor/openxr_loader_negotiation.h"
#include <mutex>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <algorithm>
namespace {
std::recursive_mutex mutex;PFN_xrGetInstanceProcAddr nextGet=nullptr;bool enabled=false;
struct Context {
 XrInstance instance{};XrActionSet set{};XrAction actions[2]{};XrPath touch{};int status=1;
 PFN_xrGetInstanceProcAddr get{};PFN_xrCreateSession createSession{};PFN_xrDestroySession destroySession{};PFN_xrDestroyInstance destroyInstance{};PFN_xrAttachSessionActionSets attach{};PFN_xrSyncActions sync{};PFN_xrGetActionStateBoolean boolean{};
 PFN_xrCreateActionSet createSet{};PFN_xrCreateAction createAction{};PFN_xrStringToPath path{};PFN_xrSuggestInteractionProfileBindings suggest{};
 std::vector<XrActionSuggestedBinding> gameTouch;std::vector<XrActiveActionSet> gameActive;
 template<class T> bool Load(const char* name,T& function){PFN_xrVoidFunction f{};auto result=get(instance,name,&f);if(XR_FAILED(result)){status=result;return false;}function=reinterpret_cast<T>(f);return f!=nullptr;}
 bool Init(){return Load("xrCreateSession",createSession)&&Load("xrDestroySession",destroySession)&&Load("xrDestroyInstance",destroyInstance)&&Load("xrAttachSessionActionSets",attach)&&Load("xrSyncActions",sync)&&Load("xrGetActionStateBoolean",boolean)&&Load("xrCreateActionSet",createSet)&&Load("xrCreateAction",createAction)&&Load("xrStringToPath",path)&&Load("xrSuggestInteractionProfileBindings",suggest);}
 bool Prepare(){
  if(set)return status>=0;
  XrActionSetCreateInfo info{XR_TYPE_ACTION_SET_CREATE_INFO};strcpy_s(info.actionSetName,"spf_driver_controller_hands");strcpy_s(info.localizedActionSetName,"Driver controller hands");auto result=createSet(instance,&info,&set);if(XR_FAILED(result)){status=result;return false;}
  std::vector<XrActionSuggestedBinding> bindings=gameTouch;
  for(unsigned i=0;i<2;++i){XrActionCreateInfo ai{XR_TYPE_ACTION_CREATE_INFO};strcpy_s(ai.actionName,i?"right_hand_toggle":"left_hand_toggle");strcpy_s(ai.localizedActionName,i?"Right hand calibration":"Left hand calibration");ai.actionType=XR_ACTION_TYPE_BOOLEAN_INPUT;result=createAction(set,&ai,&actions[i]);if(XR_FAILED(result)){status=result;return false;}XrPath click{};result=path(instance,i?"/user/hand/right/input/thumbstick/click":"/user/hand/left/input/thumbstick/click",&click);if(XR_FAILED(result)){status=result;return false;}bindings.push_back({actions[i],click});}
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
XrResult XRAPI_CALL Sync(XrSession session,const XrActionsSyncInfo* info){std::lock_guard lock(mutex);auto c=sessions.at(session);if(!c->set || c->status<0)return c->sync(session,info);
 c->gameActive.clear();if(info->countActiveActionSets)c->gameActive.assign(info->activeActionSets,info->activeActionSets+info->countActiveActionSets);auto sets=c->gameActive;if(std::none_of(sets.begin(),sets.end(),[&](const auto& a){return a.actionSet==c->set;}))sets.push_back({c->set,XR_NULL_PATH});auto combined=*info;combined.countActiveActionSets=static_cast<uint32_t>(sets.size());combined.activeActionSets=sets.data();return c->sync(session,&combined);
}
XrResult XRAPI_CALL GetProc(XrInstance instance,const char* name,PFN_xrVoidFunction* function){std::lock_guard lock(mutex);if(enabled && instance && instances.count(instance)){
 struct Entry {const char* name;PFN_xrVoidFunction function;};const Entry entries[]={{"xrCreateSession",reinterpret_cast<PFN_xrVoidFunction>(CreateSession)},{"xrDestroySession",reinterpret_cast<PFN_xrVoidFunction>(DestroySession)},{"xrDestroyInstance",reinterpret_cast<PFN_xrVoidFunction>(DestroyInstance)},{"xrAttachSessionActionSets",reinterpret_cast<PFN_xrVoidFunction>(Attach)},{"xrSyncActions",reinterpret_cast<PFN_xrVoidFunction>(Sync)},{"xrSuggestInteractionProfileBindings",reinterpret_cast<PFN_xrVoidFunction>(Suggest)}};
 for(const auto& entry:entries)if(!std::strcmp(name,entry.name)){*function=entry.function;return XR_SUCCESS;}
 return instances.at(instance)->get(instance,name,function);
 }return nextGet?nextGet(instance,name,function):XR_ERROR_FUNCTION_UNSUPPORTED;
}
XrResult XRAPI_CALL CreateInstance(const XrInstanceCreateInfo* info,const XrApiLayerCreateInfo* layer,XrInstance* instance){
 if(!layer || !layer->nextInfo)return XR_ERROR_INITIALIZATION_FAILED;
 auto next=*layer;next.nextInfo=layer->nextInfo->next;nextGet=layer->nextInfo->nextGetInstanceProcAddr;auto result=layer->nextInfo->nextCreateApiLayerInstance(info,&next,instance);
 if(XR_SUCCEEDED(result)&&enabled){std::lock_guard lock(mutex);auto c=std::make_shared<Context>();c->instance=*instance;c->get=nextGet;if(c->Init())instances[*instance]=c;}
 return result;
}
}
extern "C" __declspec(dllexport) XrResult XRAPI_CALL xrNegotiateLoaderApiLayerInterface(const XrNegotiateLoaderInfo* loader,const char*,XrNegotiateApiLayerRequest* request){
 if(!loader || !request || loader->minInterfaceVersion>1 || loader->maxInterfaceVersion<1 || request->structType!=XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST || request->structSize<sizeof(*request))return XR_ERROR_INITIALIZATION_FAILED;
 wchar_t path[MAX_PATH]{};GetModuleFileNameW(nullptr,path,MAX_PATH);auto file=wcsrchr(path,L'\\');enabled=file && !_wcsicmp(file+1,L"amtrucks.exe");request->layerInterfaceVersion=1;request->layerApiVersion=XR_MAKE_VERSION(1,0,0);request->getInstanceProcAddr=GetProc;request->createApiLayerInstance=CreateInstance;return XR_SUCCESS;
}
extern "C" __declspec(dllexport) int __cdecl SPF_ReadControllerButtons(unsigned char* left,unsigned char* right){
 std::lock_guard lock(mutex);*left=*right=0;if(!activeSession || !sessions.count(activeSession))return 5;auto c=sessions.at(activeSession);if(c->status<0)return c->status;
 auto sets=c->gameActive;if(std::none_of(sets.begin(),sets.end(),[&](const auto& a){return a.actionSet==c->set;}))sets.push_back({c->set,XR_NULL_PATH});XrActionsSyncInfo info{XR_TYPE_ACTIONS_SYNC_INFO};info.countActiveActionSets=static_cast<uint32_t>(sets.size());info.activeActionSets=sets.data();auto result=c->sync(activeSession,&info);if(result!=XR_SUCCESS)return result;
 bool valid=true;for(unsigned i=0;i<2;++i){XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=c->actions[i];XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};result=c->boolean(activeSession,&gi,&state);if(XR_FAILED(result))return result;valid=valid&&state.isActive;*(i?right:left)=state.isActive&&state.currentState;}
 return valid?0:4;
}
#ifdef SPF_CONTROLLER_LAYER_TEST
extern "C" void SPF_TestControllerLayer(PFN_xrGetInstanceProcAddr get,XrInstance instance,XrSession session){std::lock_guard lock(mutex);instances.clear();sessions.clear();auto c=std::make_shared<Context>();c->get=get;c->instance=instance;c->Init();instances[instance]=c;sessions[session]=c;enabled=true;}
extern "C" XrResult SPF_TestControllerAttach(XrSession s,const XrSessionActionSetsAttachInfo* i){return Attach(s,i);}
extern "C" XrResult SPF_TestControllerSync(XrSession s,const XrActionsSyncInfo* i){return Sync(s,i);}
#endif
