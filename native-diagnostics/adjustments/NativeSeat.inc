// Reuse the game's seat apply routine with a private menu-value snapshot.
// Its native camera/vehicle writes and refresh run unchanged; only its UI tail is skipped.
using SeatApplyFn=void(__fastcall*)(void*);
SeatApplyFn originalSeatApply=nullptr;
SPF_Hook_Handle* seatApplyHook=nullptr;
SPF_Hook_Handle* seatUiHook=nullptr;
constexpr char kSeatApplyPattern[]="48 89 5C 24 08 57 48 83 EC 70 48 8B F9 E8 4E E6 FF FF F3 0F 10 8F 78 03 00 00 F3 0F 5C 8F 9C 03 00 00 F3 0F 10 87 80 03 00 00";
constexpr char kSeatUiPattern[]="48 8B 4F 10 BA A0 00 00 00 E8 7F 7A 23 FF 48 8B D8";
std::mutex seatMutex;
std::array<unsigned char,0x400> seatTemplate{};
uintptr_t seatVehicleSettings=0;
bool seatCalibrated=false;
uintptr_t LiveSeatSettings() {
 uintptr_t global=0,service=0,actor=0,settings=0;
 const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 if(!Read(base+0x36bad90,global) || !global || !Read(global+0x31b0,service) || !service ||
    !Read(service+0x18,actor) || !actor || !Read(actor+0x1f8,settings)) return 0;
 return settings;
}
void ClearSeatCalibration() { std::lock_guard lock(seatMutex); seatCalibrated=false; seatVehicleSettings=0; }
float SeatFloat(const std::array<unsigned char,0x400>& data,size_t offset) { float v; std::memcpy(&v,data.data()+offset,4); return v; }
void SeatPut(std::array<unsigned char,0x400>& data,size_t offset,float v) { std::memcpy(data.data()+offset,&v,4); }
bool ValidSeatTemplate(const std::array<unsigned char,0x400>& data) {
 for(size_t axis=0;axis<3;++axis) {
  const float minimum=SeatFloat(data,0x384+axis*4),maximum=SeatFloat(data,0x390+axis*4),baseline=SeatFloat(data,0x39c+axis*4);
  if(!std::isfinite(minimum) || !std::isfinite(maximum) || !std::isfinite(baseline) || minimum>maximum || maximum-minimum>5) return false;
 }
 for(size_t offset:{0x3b4,0x3c4,0x3d4,0x3e4,0x3f4}) if(!std::isfinite(SeatFloat(data,offset))) return false;
 return true;
}
// The native F4 initializer obtains these values from the vehicle and camera,
// before any adjustment. Read the same sources without constructing menu UI.
bool ReadSeatDefaults(uintptr_t global,uintptr_t manager,std::array<unsigned char,0x400>& data,uintptr_t& settings) {
 uintptr_t service=0,actor=0,vehicleConfig=0,array=0,camera=0;
 uint64_t count=0; uint32_t flags=0;
 settings=0; data.fill(0);
 if(!global || !manager || !Read(global+0x31b0,service) || !service ||
    !Read(service+8,flags) || !(flags&0x80000000u) ||
    !Read(service+0x18,actor) || !actor || !Read(actor+0x1f8,settings) || !settings ||
    !Read(service+0x20,vehicleConfig) || !vehicleConfig || !Read(manager+0x40,count) || count<3 || count>64 ||
    !Read(manager+0x38,array) || !array || !Read(array+16,camera) || !camera ||
    !Read(camera+8,flags) || !(flags&0x80000000u)) return false;
 const size_t cameraSources[]={0x4a4,0x4a8,0x4ac,0x5b0,0x5c0,0x408};
 const size_t cameraTargets[]={0x39c,0x3a0,0x3a4,0x3b4,0x3c4,0x3d4};
 const size_t configSources[]={0x1284,0x1290,0x1294,0x1288,0x128c,0x1298,0x129c,0x12a0};
 const size_t configTargets[]={0x384,0x388,0x38c,0x390,0x394,0x398,0x3e4,0x3f4};
 float value=0;
 for(size_t i=0;i<6;++i) { if(!Read(camera+cameraSources[i],value) || !std::isfinite(value)) return false; SeatPut(data,cameraTargets[i],value); }
 for(size_t i=0;i<8;++i) { if(!Read(vehicleConfig+configSources[i],value) || !std::isfinite(value)) return false; SeatPut(data,configTargets[i],value); }
 SeatPut(data,0x3dc,0.f); SeatPut(data,0x3e0,1.f);
 SeatPut(data,0x3ec,0.f); SeatPut(data,0x3f0,1.f);
 // The camera helper uses this handedness flag when refreshing head angles.
 if(!Read(camera+0x4b1,data[0x3f8])) return false;
 return ValidSeatTemplate(data);
}
bool RefreshSeatDefaults() {
 const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 uintptr_t global=0,manager=0,settings=0;
 std::array<unsigned char,0x400> fresh{};
 const bool valid=Read(base+0x36bad90,global) && Read(base+0x36bae00,manager) && ReadSeatDefaults(global,manager,fresh,settings);
 bool first=false;
 { std::lock_guard lock(seatMutex);
   if(!valid) { seatCalibrated=false; seatVehicleSettings=0; return false; }
   first=!seatCalibrated || seatVehicleSettings!=settings;
   seatTemplate=fresh; seatVehicleSettings=settings; seatCalibrated=true;
 }
 if(first) Log(SPF_LOG_INFO,"SEAT: native defaults and limits initialized automatically; seat mode ready.");
 return true;
}
void DetourSeatApply(void* context) {
 originalSeatApply(context);
 if(reinterpret_cast<uintptr_t>(context)==gSyntheticSeatContext) return;
 std::array<unsigned char,0x400> captured{};
 const auto settings=LiveSeatSettings();
 if(!settings || !SafeRead(reinterpret_cast<uintptr_t>(context),captured.data(),captured.size()) || !ValidSeatTemplate(captured)) return;
 bool first=false;
 { std::lock_guard lock(seatMutex); first=!seatCalibrated || settings!=seatVehicleSettings; seatTemplate=captured; seatVehicleSettings=settings; seatCalibrated=true; }
 if(first) Log(SPF_LOG_INFO,"SEAT: native defaults and limits captured; seat mode ready for this vehicle.");
}
bool SeatReady() {
 if(!core || !core->hooks || !seatApplyHook || !seatUiHook || !originalSeatApply) return false;
 for(auto h:{seatApplyHook,seatUiHook}) if(!core->hooks->Hook_IsInstalled(h) || !core->hooks->Hook_IsEnabled(h)) return false;
 return RefreshSeatDefaults();
}
bool PrepareSeatValues(std::array<unsigned char,0x400>& data,const std::array<float,8>& current,float dt,unsigned char mask,int mode=6) {
 if(!std::isfinite(dt) || dt<=0 || dt>0.1f || !ValidSeatTemplate(data)) return false;
 for(float v:current) if(!std::isfinite(v)) return false;
 if(!(mask&31) || mode<6 || mode>8) return false;
 const float step=0.1f*dt;
 const float vertical=((mask&4)?1.f:0.f)-((mask&8)?1.f:0.f);
 const float depth=((mask&2)?1.f:0.f)-((mask&1)?1.f:0.f);
 if(!(mask&16) && (mode==6?(vertical==0 && depth==0):mode==7?depth==0:vertical==0)) return false;
 for(size_t axis=0;axis<3;++axis) {
  const float baseline=SeatFloat(data,0x39c+axis*4);
  float value=baseline+current[axis];
  if(mode==6 && axis==1) value=(mask&16)?baseline:value+vertical*step;
  if(mode==6 && axis==2) value=(mask&16)?baseline:value+depth*step;
  // Preserve lateral position, which this VR prototype does not adjust.
  if(mode==6 && axis>0) value=std::clamp(value,SeatFloat(data,0x384+axis*4),SeatFloat(data,0x390+axis*4));
  SeatPut(data,0x378+axis*4,value);
 }
 // Native apply also writes these adjustments; reconstruct all from current state.
 const size_t valueOffsets[]={0x3c8,0x3d8,0x3e8,0x3a8,0x3b8};
 const size_t baseOffsets[]={0x3d4,0x3e4,0x3f4,0x3b4,0x3c4};
 for(size_t i=0;i<5;++i) SeatPut(data,valueOffsets[i],current[i+3]+SeatFloat(data,baseOffsets[i]));
 if(mode>=7) {
  const size_t valueOffset=mode==7?0x3e8:0x3d8;
  const size_t baseOffset=mode==7?0x3f4:0x3e4;
  const float minimum=SeatFloat(data,valueOffset+4),maximum=SeatFloat(data,valueOffset+8);
  if(!std::isfinite(minimum) || !std::isfinite(maximum) || minimum>maximum || minimum<0 || maximum>1) return false;
  const float baseline=SeatFloat(data,baseOffset);
  const float value=(mask&16)?baseline:SeatFloat(data,valueOffset)+(mode==7?depth:vertical)*0.25f*dt;
  SeatPut(data,valueOffset,std::clamp(value,minimum,maximum));
 }

 return true;
}
void MoveNativeSeat(float dt,unsigned char mask,int mode) {
 if(!SeatReady()) return;
 std::lock_guard lock(seatMutex);
 std::array<float,8> current{};
 const auto settings=LiveSeatSettings();
 if(settings!=seatVehicleSettings || !SafeRead(settings+0x1f0,current.data(),sizeof(current))) { seatCalibrated=false; return; }
 auto values=seatTemplate;
 if(!PrepareSeatValues(values,current,dt,mask,mode)) return;
 gSyntheticSeatContext=reinterpret_cast<uintptr_t>(values.data());
 originalSeatApply(values.data());
 gSyntheticSeatContext=0;
}
void RegisterSeatHooks() {
 const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 if(core->hooks->Hook_FindPattern(kSeatApplyPattern)!=base+0x114f3d0 || core->hooks->Hook_FindPattern(kSeatUiPattern)!=base+0x114f4c3) { Log(SPF_LOG_WARN,"SEAT: native signatures mismatch; seat controls disabled."); return; }
 gSeatApplyReturn=reinterpret_cast<void*>(base+0x114f56a);
 seatUiHook=core->hooks->Hook_Register(kName,"SeatUiTail","Skip seat UI only during hotkey call",reinterpret_cast<void*>(SeatUiStub),&gSeatUiOriginal,kSeatUiPattern,true);
 seatApplyHook=core->hooks->Hook_Register(kName,"SeatApply","Capture native seat defaults and limits",reinterpret_cast<void*>(DetourSeatApply),reinterpret_cast<void**>(&originalSeatApply),kSeatApplyPattern,true);
 Log(SPF_LOG_INFO,"SEAT: hooks requested; native defaults initialize automatically when seat mode is selected.");
}
