#pragma once
#include "vendor/openxr.h"
#include <array>
inline bool CombineControllerActionSets(const XrSessionActionSetsAttachInfo& original,XrActionSet extra,XrSessionActionSetsAttachInfo& combined,std::array<XrActionSet,17>& storage){
 if(original.type!=XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO || !extra || original.countActionSets>16 || (original.countActionSets && !original.actionSets))return false;
 unsigned count=original.countActionSets;for(unsigned i=0;i<count;++i){if(!original.actionSets[i])return false;storage[i]=original.actionSets[i];if(storage[i]==extra){combined=original;return true;}}
 storage[count++]=extra;combined=original;combined.countActionSets=count;combined.actionSets=storage.data();return true;
}
