#pragma once
#include <cmath>
#include <sstream>
#include <string>
inline bool ParseSignalDelay(const std::string& text,float& accepted) {
 if(text.size()>128)return false;
 std::istringstream input(text);float value;std::string extra;
 if(!(input>>value) || (input>>extra) || !std::isfinite(value) || value<0 || value>.45f)return false;
 accepted=value;return true;
}

inline bool ParseSignalActuation(const std::string& text,float& accepted) {
 if(text.size()>128)return false;
 std::istringstream input(text);float value;std::string extra;
 if(!(input>>value) || (input>>extra) || !std::isfinite(value) || value<.05f || value>.6f)return false;
 accepted=value;return true;
}
