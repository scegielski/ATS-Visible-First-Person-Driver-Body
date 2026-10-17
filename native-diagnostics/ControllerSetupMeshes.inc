// Uses the bridge's existing stereo compositor renderer. No game scene objects.
using SetupPoseSubmit=int(__cdecl*)(const ControllerMeshPose*,uint32_t);
using SetupMeshUpload=int(__cdecl*)(unsigned,const ControllerMeshVertex*,uint32_t,const uint16_t*,uint32_t,const uint8_t*,uint32_t,uint32_t,uint32_t,uint32_t);
int cabinHoverTargets[2]{-1,-1};uint64_t cabinHoverTick=0;
SetupPoseSubmit setupPoseSubmit=nullptr;bool setupMeshesReady=false;
ControllerMeshData SetupLabelMesh(unsigned box){
 ControllerMeshData mesh;mesh.vertices={{{-1,-1,0},{0,0,1},{0,1}},{{1,-1,0},{0,0,1},{1,1}},{{-1,1,0},{0,0,1},{0,0}},{{1,1,0},{0,0,1},{1,0}}};mesh.indices={0,1,2,1,3,2};mesh.width=1024;mesh.height=128;mesh.mips=1;mesh.texture.resize(mesh.width*mesh.height*4);
 const wchar_t* names[]{L"IGNITION",L"HORN",L"LEFT MIRROR",L"CENTER MIRROR",L"RIGHT MIRROR",L"SEAT",L"LEFT WINDOW",L"RIGHT WINDOW",L"STEERING WHEEL ADJUSTMENTS",L"LIGHTS",L"WIPERS",L"RADIO",L"LEFT HAND",L"RIGHT HAND",L"DASHBOARD",L"INFOTAINMENT",L"LEFT FOOT",L"RIGHT FOOT",L"LEFT ELBOW TARGET",L"RIGHT ELBOW TARGET",L"LEFT KNEE TARGET",L"RIGHT KNEE TARGET"};
 BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=mesh.width;info.bmiHeader.biHeight=-static_cast<LONG>(mesh.height);info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 auto dc=CreateCompatibleDC(nullptr);void* bits=nullptr;auto bitmap=dc?CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0):nullptr;
 if(!bitmap){if(dc)DeleteDC(dc);mesh.texture.clear();return mesh;}
 auto old=SelectObject(dc,bitmap);memset(bits,0,mesh.texture.size());auto font=CreateFontW((box==8||box>=18)?-54:-78,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");auto oldFont=font?SelectObject(dc,font):nullptr;SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,255,255));RECT rect{0,0,1024,128};DrawTextW(dc,names[box],-1,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
 auto pixels=static_cast<unsigned char*>(bits);for(size_t i=0;i<mesh.texture.size();i+=4){mesh.texture[i]=mesh.texture[i+1]=mesh.texture[i+2]=255;mesh.texture[i+3]=std::max({pixels[i],pixels[i+1],pixels[i+2]});}
 if(oldFont)SelectObject(dc,oldFont);if(font)DeleteObject(font);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return mesh;
}
unsigned hoverMeshStyle=~0u;
bool RefreshHoverMeshes(SetupMeshUpload upload){auto& prefs=highlightPreferences.settings;unsigned style=prefs.color+prefs.opacity*8;if(style==hoverMeshStyle)return true;for(unsigned side=0;side<2;++side){auto mesh=InteractionHoverFillMesh(prefs.Color(),prefs.Strength());if(upload(InteractionHoverMeshStart+side*2,mesh.vertices.data(),static_cast<uint32_t>(mesh.vertices.size()),mesh.indices.data(),static_cast<uint32_t>(mesh.indices.size()),mesh.texture.data(),mesh.width,mesh.height,mesh.format,1))return false;}hoverMeshStyle=style;return true;}
bool EnsureSetupMeshes(){
auto module=GetModuleHandleW(L"spf_controller_xr_layer.dll");auto upload=module?reinterpret_cast<SetupMeshUpload>(GetProcAddress(module,"SPF_SetControllerRenderModel")):nullptr;setupPoseSubmit=module?reinterpret_cast<SetupPoseSubmit>(GetProcAddress(module,"SPF_SetInteractionBoxTrackingPoses")):nullptr;if(!upload||!setupPoseSubmit)return false;if(setupMeshesReady)return RefreshHoverMeshes(upload);
 const std::array<std::array<uint8_t,3>,22> colors{{{255,90,70},{90,255,120},{80,210,255},{80,210,255},{80,210,255},{255,220,90},{220,130,255},{220,130,255},{255,180,70},{255,255,180},{150,220,255},{255,160,220},{90,150,255},{90,150,255},{140,255,210},{140,255,210},{90,150,255},{90,150,255},{255,160,100},{255,160,100},{150,255,120},{150,255,120}}};
 for(unsigned i=0;i<InteractionBoxCount;++i){auto wire=SetupBoxWireMesh();for(unsigned c=0;c<3;++c)wire.texture[c]=colors[i][c];auto label=SetupLabelMesh(i);if(label.texture.empty())return false;
  for(unsigned part=0;part<2;++part){const auto& mesh=part?label:wire;if(upload(InteractionBoxMeshStart+i+part*InteractionBoxCount,mesh.vertices.data(),static_cast<uint32_t>(mesh.vertices.size()),mesh.indices.data(),static_cast<uint32_t>(mesh.indices.size()),mesh.texture.data(),mesh.width,mesh.height,0,1))return false;}
 }
 for(unsigned j=0;j<144;++j){auto mesh=j<72?JointAxesMesh():JointBoneMesh();if(upload(DriverJointAxesMeshStart+j,mesh.vertices.data(),static_cast<uint32_t>(mesh.vertices.size()),mesh.indices.data(),static_cast<uint32_t>(mesh.indices.size()),mesh.texture.data(),mesh.width,mesh.height,0,1))return false;}
 if(!RefreshHoverMeshes(upload))return false;
 setupMeshesReady=true;return true;
}
void SubmitSetupBoxes(bool active,bool hover=false,bool skeletonActive=false){
 std::array<ControllerMeshPose,InteractionBoxMeshCount> poses{};
 if(active&&EnsureSetupMeshes())for(unsigned i=0;i<InteractionBoxCount;++i){if((i==12||i==13||i>=16)&&!handBoxes.valid)continue;for(unsigned part=0;part<2;++part){auto pose=SetupBoxTrackingPose(boxEditor.boxes[i],actualSceneFrame,part!=0);for(float v:pose.matrix)if(!std::isfinite(v)||std::abs(v)>10)pose.visible=false;poses[i+part*InteractionBoxCount]=pose;}}
 if(skeletonActive&&skeletonVisible&&EnsureSetupMeshes()&&jointAxesTick&&GetTickCount64()-jointAxesTick<250){auto basis=actualSceneFrame.CabinToTracking();for(unsigned j=0;j<144;++j){ControllerMeshPose pose;pose.visible=true;for(unsigned col=0;col<3;++col){auto v=DriverIK::Rotate(basis,{driverJointAxes[j][col],driverJointAxes[j][4+col],driverJointAxes[j][8+col]});for(unsigned row=0;row<3;++row)pose.matrix[row*4+col]=v[row];}auto p=actualSceneFrame.CabinPointToTracking({driverJointAxes[j][3],driverJointAxes[j][7],driverJointAxes[j][11]});for(unsigned row=0;row<3;++row)pose.matrix[row*4+3]=p[row];poses[DriverJointAxesMeshStart-InteractionBoxMeshStart+j]=pose;}}
 if(hover&&EnsureSetupMeshes()){auto now=GetTickCount64();auto highlight=InteractionHoverPoses(boxEditor,actualSceneFrame,cabinHoverTargets,cabinHoverTick&&now>=cabinHoverTick&&now-cabinHoverTick<150,handBoxes.valid);std::copy(highlight.begin(),highlight.end(),poses.begin()+InteractionHoverMeshStart-InteractionBoxMeshStart);}
 if(setupPoseSubmit)setupPoseSubmit(poses.data(),static_cast<uint32_t>(poses.size()));
 static int prior=-1;int state=active?(setupMeshesReady?1:2):0;if(state!=prior){prior=state;ReportControllerSetupGraphics(state,handBoxes.valid?22:14);}
}
