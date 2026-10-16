#pragma once
#include "DriverSettings.hpp"
#include "InteractionBoxEditor.hpp"

struct VehicleBoxIdentity {
 std::string brand,model;
 bool operator==(const VehicleBoxIdentity&)const=default;
 bool Valid()const{if(model.empty()||model.size()>256||brand.size()>256)return false;for(const auto* value:{&brand,&model})for(unsigned char c:*value)if(c<32||c==127)return false;return true;}
 bool MatchesMetadata(std::string value)const{auto expected=Metadata();while(!value.empty()&&(value.back()=='\n'||value.back()=='\r'))value.pop_back();while(!expected.empty()&&(expected.back()=='\n'||expected.back()=='\r'))expected.pop_back();return value==expected;}
 std::string Metadata()const{return "brand_id "+brand+"\nmodel_id "+model+"\n";}
 std::string Filename()const{
  if(!Valid())return {};auto identity=brand+'\x1f'+model;uint64_t hash=14695981039346656037ull;for(unsigned char c:identity){hash^=c;hash*=1099511628211ull;}
  std::string slug;for(unsigned char c:brand+'_'+model){if(slug.size()>=56)break;if(c>='A'&&c<='Z')c+=32;slug+=((c>='a'&&c<='z')||(c>='0'&&c<='9'))?static_cast<char>(c):'_';}
  std::ostringstream out;out<<slug<<'_'<<std::hex<<std::setfill('0')<<std::setw(16)<<hash<<"_boxes.ini";return out.str();
 }
 bool Mustang()const{auto id=model;for(char& c:id)if(c>='A'&&c<='Z')c+=32;return id.find("mustang")!=std::string::npos;}
};
inline std::filesystem::path VehicleBoxPath(const std::filesystem::path& config,const VehicleBoxIdentity& id){return id.Valid()?config/"vehicles"/id.Filename():std::filesystem::path{};}
inline bool ReadVehicleBoxes(const std::filesystem::path& path,const VehicleBoxIdentity& id,InteractionBoxEditor& editor,std::string& placement){
 std::string text;DriverSettingSections sections;if(!id.Valid()||!ReadDriverSettingsFile(path,text)||!ParseDriverSettings(text,sections)||!id.MatchesMetadata(sections["vehicle"]))return false;
 auto found=sections.find("interaction_boxes");if(found==sections.end()||!ParseInteractionBoxes(found->second,editor))return false;placement=found->second;return true;
}
inline bool WriteVehicleBoxes(const std::filesystem::path& path,const VehicleBoxIdentity& id,const std::string& placement){
 InteractionBoxEditor checked;if(!id.Valid()||path.empty()||!ParseInteractionBoxes(placement,checked))return false;
 std::error_code ec;std::filesystem::create_directories(path.parent_path(),ec);if(ec)return false;bool exists=std::filesystem::exists(path,ec);if(ec)return false;
 std::string before;DriverSettingSections sections;if(exists&&(!ReadDriverSettingsFile(path,before)||!ParseDriverSettings(before,sections)||!id.MatchesMetadata(sections["vehicle"])))return false;
 sections["vehicle"]=id.Metadata();sections["interaction_boxes"]=placement;auto temporary=path;temporary+=L".tmp";
 {std::ofstream file(temporary,std::ios::binary|std::ios::trunc);if(!file)return false;file<<SerializeDriverSettings(sections);file.flush();if(!file.good())return false;}
 std::string latest;if((exists&&(!ReadDriverSettingsFile(path,latest)||latest!=before))||(!exists&&std::filesystem::exists(path,ec))){std::filesystem::remove(temporary,ec);return false;}
 return MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
}
inline InteractionBoxEditor GenericVehicleBoxes(){InteractionBoxEditor editor;for(unsigned i=0;i<12;++i){editor.boxes[i].center={-.7f+.2f*static_cast<float>(i%4),1.6f-.24f*static_cast<float>(i/4),-.40f};editor.boxes[i].rotation={1,0,0,0};editor.boxes[i].scale=1;for(unsigned axis=0;axis<3;++axis)editor.boxes[i].stretch[axis]=.05f/editor.boxes[i].half[axis];}return editor;}
inline void ResetVehicleBoxLayout(InteractionBoxEditor& editor){auto defaults=GenericVehicleBoxes();defaults.enabled=editor.enabled;for(auto& authored:defaults.authored)authored=true;defaults.dirty=true;editor=defaults;}
// First-use seed only. Never modify the legacy Mustang file or another vehicle.
inline bool InitializeVehicleBoxes(const std::filesystem::path& config,const VehicleBoxIdentity& id,InteractionBoxEditor& editor,std::string& placement,bool& migrated){
 migrated=false;auto path=VehicleBoxPath(config,id);if(path.empty())return false;std::error_code ec;
 if(std::filesystem::exists(path,ec)){
  if(!ReadVehicleBoxes(path,id,editor,placement))return false;
  // Upgrade only the untouched generic seed emitted before independent rendering.
  if(!id.Mustang()&&SerializeInteractionBoxes(editor)==SerializeInteractionBoxes(InteractionBoxEditor{})){auto defaults=GenericVehicleBoxes();placement=SerializeInteractionBoxes(defaults);if(!WriteVehicleBoxes(path,id,placement))return false;return ReadVehicleBoxes(path,id,editor,placement);}
  return true;
 }if(ec)return false;
 InteractionBoxEditor initial=GenericVehicleBoxes();
 if(id.Mustang()){
  auto legacy=config/"mustang_boxes.ini";if(std::filesystem::exists(legacy,ec)){
   std::string text;DriverSettingSections sections;if(!ReadDriverSettingsFile(legacy,text)||!ParseDriverSettings(text,sections)||!ParseInteractionBoxes(sections["interaction_boxes"],initial))return false;migrated=true;
  }if(ec)return false;
 }
 placement=SerializeInteractionBoxes(initial);if(!WriteVehicleBoxes(path,id,placement))return false;return ReadVehicleBoxes(path,id,editor,placement);
}
