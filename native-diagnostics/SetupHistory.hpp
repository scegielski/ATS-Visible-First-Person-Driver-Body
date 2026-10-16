#pragma once
#include "RuntimeArmIK.hpp"
#include <string>
#include <vector>
#include <array>
struct SetupHistoryState {std::string boxes;std::array<DriverIK::V,4> hands{};DriverIK::V camera{};bool cameraConfigured=false,headVisible=true,driverVisible=true;bool operator==(const SetupHistoryState&)const=default;};
struct SetupHistory {
 SetupHistoryState current;std::vector<SetupHistoryState> undo,redo;bool initialized=false;
 void Record(const SetupHistoryState& value){if(!initialized){current=value;initialized=true;return;}if(value==current)return;undo.push_back(current);if(undo.size()>100)undo.erase(undo.begin());current=value;redo.clear();}
 const SetupHistoryState* Target(bool forward)const{auto& stack=forward?redo:undo;return stack.empty()?nullptr:&stack.back();}
 void Commit(bool forward){auto& from=forward?redo:undo;auto& to=forward?undo:redo;if(from.empty())return;to.push_back(current);current=from.back();from.pop_back();}
};
