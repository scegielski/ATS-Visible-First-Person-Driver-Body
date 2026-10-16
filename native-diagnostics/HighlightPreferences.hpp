#pragma once
#include "DriverSettings.hpp"
#include <array>
#include <set>
struct HighlightPreferences {
 bool enabled=true;unsigned color=0,opacity=1;
 bool operator==(const HighlightPreferences&)const=default;
 static constexpr const wchar_t* ColorNames[]{L"CYAN",L"WHITE",L"GREEN",L"AMBER",L"PURPLE"};
 static constexpr const wchar_t* OpacityNames[]{L"LOW",L"MEDIUM",L"HIGH",L"STRONG"};
 std::array<uint8_t,3> Color()const{constexpr std::array<uint8_t,3> colors[]{{90,210,255},{255,255,255},{100,255,150},{255,180,65},{210,135,255}};return colors[color<5?color:0];}
 float Strength()const{constexpr float values[]{.5f,1.f,1.5f,2.f};return values[opacity<4?opacity:1];}
 bool Select(int segment){if(segment==0)enabled=!enabled;else if(segment==2)color=(color+1)%5;else if(segment==4)opacity=(opacity+1)%4;else return false;return true;}
};
inline std::string SerializeHighlightPreferences(const HighlightPreferences& prefs){return SerializeDriverSettings({{"highlights","enabled "+std::to_string(prefs.enabled?1:0)+"\ncolor "+std::to_string(prefs.color)+"\nopacity "+std::to_string(prefs.opacity)+"\n"}});}
inline bool ParseHighlightPreferences(const std::string& text,HighlightPreferences& output){DriverSettingSections sections;if(!ParseDriverSettings(text,sections)||sections.size()!=1||!sections.count("highlights"))return false;HighlightPreferences candidate;std::set<std::string> seen;std::istringstream input(sections["highlights"]);std::string line;while(std::getline(input,line)){auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos||line[first]=='#'||line[first]==';')continue;std::istringstream row(line);std::string key,extra;int value;if(!(row>>key>>value)||(row>>extra)||!seen.insert(key).second)return false;if(key=="enabled"){if(value!=0&&value!=1)return false;candidate.enabled=value!=0;}else if(key=="color"){if(value<0||value>=5)return false;candidate.color=value;}else if(key=="opacity"){if(value<0||value>=4)return false;candidate.opacity=value;}else return false;}if(seen.size()!=3)return false;output=candidate;return true;}
struct LiveHighlightPreferences {
 HighlightPreferences settings;std::string prior;
 bool Poll(const std::filesystem::path& config){auto path=config/"highlights.ini";std::error_code ec;if(!std::filesystem::exists(path,ec))return !ec;std::string text;if(!ReadDriverSettingsFile(path,text))return false;if(text==prior)return true;HighlightPreferences candidate;if(!ParseHighlightPreferences(text,candidate))return false;settings=candidate;prior=text;return true;}
 bool Select(int segment,const std::filesystem::path& config){if(config.empty())return false;auto candidate=settings;if(!candidate.Select(segment))return false;auto path=config/"highlights.ini";std::error_code ec;std::filesystem::create_directories(config,ec);if(ec)return false;bool exists=std::filesystem::exists(path,ec);if(ec)return false;std::string before;if(exists&&(!ReadDriverSettingsFile(path,before)||before!=prior))return false;if(!exists&&!prior.empty())return false;auto text=SerializeHighlightPreferences(candidate);auto temporary=path;temporary+=L".tmp";{std::ofstream stream(temporary,std::ios::binary|std::ios::trunc);if(!stream)return false;stream<<text;stream.flush();if(!stream.good())return false;}std::string latest;if((exists&&(!ReadDriverSettingsFile(path,latest)||latest!=before))||(!exists&&std::filesystem::exists(path,ec))){std::filesystem::remove(temporary,ec);return false;}if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return false;settings=candidate;prior=text;return true;}
};
