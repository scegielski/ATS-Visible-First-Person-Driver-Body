#pragma once
#include <array>
#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>
#include <cmath>
#include <algorithm>
struct ControllerMeshVertex {float position[3],normal[3],uv[2];};
static_assert(sizeof(ControllerMeshVertex)==32);
struct ControllerMeshData {std::vector<ControllerMeshVertex> vertices;std::vector<uint16_t> indices;std::vector<uint8_t> texture;uint32_t width=0,height=0,format=0,mips=1;uint64_t revision=0;};
struct ControllerMeshPose {std::array<float,12> matrix{};bool visible=false;};
inline uint64_t ControllerTextureBytes(uint32_t width,uint32_t height,uint32_t format,uint32_t mips){if(!width||!height||width>8192||height>8192||!mips||mips>14||format>5)return 0;uint64_t total=0;for(uint32_t i=0;i<mips;++i){if(format==0||format==5)total+=uint64_t(width)*height*(format==0?4:8);else total+=uint64_t((width+3)/4)*((height+3)/4)*(format==2?8:16);width=std::max(1u,width/2);height=std::max(1u,height/2);}return total<=256*1024*1024?total:0;}
inline bool ValidControllerMesh(const ControllerMeshVertex* vertices,uint32_t vertexCount,const uint16_t* indices,uint32_t indexCount){if(!vertices||!indices||!vertexCount||vertexCount>65536||!indexCount||indexCount%3||indexCount>300000)return false;for(uint32_t i=0;i<vertexCount;++i){for(float x:vertices[i].position)if(!std::isfinite(x)||std::abs(x)>2)return false;for(float x:vertices[i].normal)if(!std::isfinite(x))return false;for(float x:vertices[i].uv)if(!std::isfinite(x))return false;}for(uint32_t i=0;i<indexCount;++i)if(indices[i]>=vertexCount)return false;return true;}
using ControllerModelMatrix=std::array<float,16>;
inline ControllerModelMatrix ControllerModelIdentity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
inline ControllerModelMatrix ControllerModelMultiply(const ControllerModelMatrix& a,const ControllerModelMatrix& b){ControllerModelMatrix o{};for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)for(unsigned k=0;k<4;++k)o[r*4+c]+=a[r*4+k]*b[k*4+c];return o;}
inline ControllerModelMatrix ControllerModelFrom34(const std::array<float,12>& m){auto o=ControllerModelIdentity();for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)o[c*4+r]=m[r*4+c];return o;}
inline ControllerModelMatrix ControllerModelProjection(float left,float right,float down,float up,float nearZ=.01f,float farZ=10){ControllerModelMatrix o{};o[0]=2/(right-left);o[5]=2/(up-down);o[8]=(right+left)/(right-left);o[9]=(up+down)/(up-down);o[10]=farZ/(nearZ-farZ);o[11]=-1;o[14]=nearZ*farZ/(nearZ-farZ);return o;}
inline constexpr unsigned ControllerModelSlotCount=112;
inline constexpr unsigned InteractionBoxMeshStart=64,InteractionBoxMeshCount=48;
inline constexpr unsigned InteractionHoverMeshStart=108;
struct ControllerModelSnapshot {std::array<std::shared_ptr<const ControllerMeshData>,ControllerModelSlotCount> meshes;std::array<ControllerMeshPose,ControllerModelSlotCount> poses;uint64_t tick=0,setupTick=0;bool setupTrackingSpace=false;};
inline bool ControllerModelSlotVisible(const ControllerModelSnapshot& state,unsigned slot,uint64_t now){auto stamp=slot<InteractionBoxMeshStart?state.tick:state.setupTick;return slot<ControllerModelSlotCount&&stamp&&now>=stamp&&now-stamp<=250&&state.poses[slot].visible;}
struct ControllerModelStore {std::mutex mutex;ControllerModelSnapshot state;uint64_t revision=0;bool Snapshot(ControllerModelSnapshot& out){std::unique_lock lock(mutex,std::try_to_lock);if(!lock.owns_lock())return false;out=state;return true;}};
