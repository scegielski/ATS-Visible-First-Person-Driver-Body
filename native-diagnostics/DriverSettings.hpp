#pragma once
#include <Windows.h>
#include "DriverDefaults.hpp"
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>

using DriverSettingSections=std::map<std::string,std::string>;
inline bool DriverSettingName(const std::string& name){if(name.empty()||name.size()>80)return false;for(char c:name)if((c<'a'||c>'z')&&(c<'0'||c>'9')&&c!='_')return false;return true;}
inline bool ParseDriverSettings(const std::string& text,DriverSettingSections& output){
 if(text.size()>262144 || text.find('\0')!=std::string::npos)return false;
 DriverSettingSections candidate;auto offset=text.starts_with("\xef\xbb\xbf")?3u:0u;std::istringstream input(text.substr(offset));std::string line,current;
 while(std::getline(input,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();auto first=line.find_first_not_of(" \t");
  if(first!=std::string::npos && line[first]=='['){auto last=line.find_last_not_of(" \t");if(line[last]!=']')return false;auto name=line.substr(first+1,last-first-1);if(!DriverSettingName(name)||candidate.count(name))return false;current=name;candidate[current]={};}
  else if(current.empty()){if(first!=std::string::npos && line[first]!='#' && line[first]!=';')return false;}
  else candidate[current]+=line+'\n';
 }
 if(candidate.empty())return false;output=std::move(candidate);return true;
}
inline std::string SerializeDriverSettings(const DriverSettingSections& sections){std::string text="# Live driver settings and poses. Preserve section names.\n";for(const auto& [name,value]:sections){text+='\n';text+='['+name+"]\n";auto last=value.find_last_not_of("\r\n");if(last!=std::string::npos)text+=value.substr(0,last+1);text+='\n';}return text;}
inline bool ReadDriverSettingsFile(const std::filesystem::path& path,std::string& text){
 std::error_code ec;auto size=std::filesystem::file_size(path,ec);if(ec||size>262144)return false;std::ifstream file(path,std::ios::binary);if(!file)return false;text.resize(static_cast<size_t>(size));file.read(text.data(),static_cast<std::streamsize>(size));return file.good()||(!size&&file.eof());
}
struct DriverSettingsCache {std::filesystem::file_time_type stamp{};uintmax_t size=0;bool attempted=false;DriverSettingSections sections;};
inline std::mutex& DriverSettingsMutex(){static std::mutex mutex;return mutex;}
inline auto& DriverSettingsCaches(){static std::map<std::filesystem::path,DriverSettingsCache> caches;return caches;}
inline std::filesystem::path DriverSettingBundle(const std::filesystem::path& directory,const std::string& key){
 auto specific=directory/(key=="interaction_boxes"?"mustang_boxes.ini":"mustang_poses.ini");
 bool pose=key=="controller_left_fingers"||key=="controller_right_fingers"||key=="gesture_pose"||key=="right_gesture_pose"||key=="ignition_pose"||key=="signal_left_pose"||key=="signal_right_pose";
 return ((key=="interaction_boxes"||pose)&&std::filesystem::exists(specific))?specific:directory/"driver_body.ini";
}
inline bool ReadDriverSetting(const std::filesystem::path& oldPath,std::string& text){
 static const auto defaults=[]{DriverSettingSections values;ParseDriverSettings(kDriverDefaults,values);return values;}();
 auto general=defaults.find(oldPath.stem().string());if(general!=defaults.end()){text=general->second;return true;}
 auto bundle=DriverSettingBundle(oldPath.parent_path(),oldPath.stem().string());std::lock_guard lock(DriverSettingsMutex());std::error_code ec;
 if(!std::filesystem::exists(bundle,ec))return !ec&&ReadDriverSettingsFile(oldPath,text); // Migration compatibility only.
 auto stamp=std::filesystem::last_write_time(bundle,ec);if(ec)return false;auto size=std::filesystem::file_size(bundle,ec);if(ec)return false;auto& cache=DriverSettingsCaches()[bundle];
 if(!cache.attempted||cache.stamp!=stamp||cache.size!=size){cache.attempted=true;cache.stamp=stamp;cache.size=size;std::string content;DriverSettingSections parsed;if(ReadDriverSettingsFile(bundle,content)&&ParseDriverSettings(content,parsed))cache.sections=std::move(parsed);}
 auto found=cache.sections.find(oldPath.stem().string());if(found==cache.sections.end())return false;text=found->second;return true;
}
inline std::istringstream OpenDriverSetting(const std::filesystem::path& oldPath,std::ios::openmode = std::ios::in){std::string text;bool okay=ReadDriverSetting(oldPath,text);std::istringstream stream(text);if(!okay)stream.setstate(std::ios::failbit);return stream;}
inline uintmax_t DriverSettingSize(const std::filesystem::path& path,std::error_code& ec){std::string text;if(!ReadDriverSetting(path,text)){ec=std::make_error_code(std::errc::invalid_argument);return 0;}ec.clear();return text.size();}
inline bool WriteDriverSetting(const std::filesystem::path& directory,const std::string& key,const std::string& value){
 if(!DriverSettingName(key)||value.size()>65536)return false;std::lock_guard lock(DriverSettingsMutex());auto target=DriverSettingBundle(directory,key);DriverSettingSections sections;std::string before;std::error_code ec;bool exists=std::filesystem::exists(target,ec);if(ec)return false;
 if(exists){if(!ReadDriverSettingsFile(target,before)||!ParseDriverSettings(before,sections))return false;}
 else{for(std::filesystem::directory_iterator i(directory,ec),end;!ec&&i!=end;i.increment(ec)){auto path=i->path();auto name=path.stem().string();std::string text;if(path.extension()==".txt"&&DriverSettingName(name)&&ReadDriverSettingsFile(path,text))sections[name]=text;}if(ec)return false;}
 sections[key]=value;auto text=SerializeDriverSettings(sections);DriverSettingSections checked;if(!ParseDriverSettings(text,checked))return false;auto temporary=target;temporary+=L".tmp";
 {std::ofstream file(temporary,std::ios::binary|std::ios::trunc);if(!file)return false;file<<text;file.flush();if(!file.good())return false;}
 // Do not overwrite an external edit made while constructing the new bundle.
 std::string latest;if(exists&&(!ReadDriverSettingsFile(target,latest)||latest!=before)){std::filesystem::remove(temporary,ec);return false;}
 if(!exists&&std::filesystem::exists(target,ec)){std::filesystem::remove(temporary,ec);return false;}
 if(!MoveFileExW(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return false;
 DriverSettingsCaches().erase(target);return true;
}
