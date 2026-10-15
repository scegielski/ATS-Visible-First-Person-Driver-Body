#pragma once
#include <Windows.h>
#include <mmsystem.h>
#include <fstream>
#include <vector>
#include <filesystem>
#pragma pack(push,1)
struct ServoWaveHeader {
 char riff[4]; uint32_t fileSize; char wave[4]; char fmt[4]; uint32_t fmtSize;
 uint16_t format,channels; uint32_t rate,byteRate; uint16_t align,bits;
 char data[4]; uint32_t bytes;
};
#pragma pack(pop)
static_assert(sizeof(ServoWaveHeader)==44);
inline bool ValidServoHeader(const ServoWaveHeader& h) {
 return !std::memcmp(h.riff,"RIFF",4) && !std::memcmp(h.wave,"WAVE",4) &&
  !std::memcmp(h.fmt,"fmt ",4) && !std::memcmp(h.data,"data",4) && h.fmtSize==16 &&
  h.format==1 && (h.channels==1 || h.channels==2) && h.bits==16 && h.rate==44100 && h.align==h.channels*2 &&
  h.byteRate==h.rate*h.align && h.bytes>0 && h.bytes<=1048576 && h.bytes%h.align==0;
}
inline DWORD ServoChannelVolume(int slot) { return slot==0?0x00004000:slot==4?0x40004000:slot==2?0x40000000:0; }
inline DWORD NextServoVolume(DWORD current,int slot) {
 const auto requested=ServoChannelVolume(slot);
 // Inactive mode pauses the motor; never mute the output device on exit.
 return requested?requested:current;
}
class ServoLoop {
 HWAVEOUT device=nullptr;
 WAVEHDR header{};
 std::vector<char> samples;
 bool playing=false;
 DWORD channelVolume=0x40004000;
public:
 bool Open(const std::filesystem::path& path) {
  Close();
  std::ifstream file(path,std::ios::binary); ServoWaveHeader wav{};
  if(!file.read(reinterpret_cast<char*>(&wav),sizeof(wav)) || !ValidServoHeader(wav)) return false;
  samples.resize(wav.bytes);
  if(!file.read(samples.data(),wav.bytes)) { samples.clear(); return false; }
  WAVEFORMATEX format{}; format.wFormatTag=WAVE_FORMAT_PCM; format.nChannels=wav.channels;
  format.nSamplesPerSec=44100; format.wBitsPerSample=16; format.nBlockAlign=wav.align; format.nAvgBytesPerSec=wav.byteRate;
  if(waveOutOpen(&device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR) { device=nullptr; samples.clear(); return false; }
  waveOutSetVolume(device,channelVolume);
  header.lpData=samples.data(); header.dwBufferLength=wav.bytes;
  header.dwFlags=WHDR_BEGINLOOP|WHDR_ENDLOOP; header.dwLoops=0xffffffff;
  if(waveOutPrepareHeader(device,&header,sizeof(header))!=MMSYSERR_NOERROR) { Close(); return false; }
  waveOutPause(device);
  if(waveOutWrite(device,&header,sizeof(header))!=MMSYSERR_NOERROR) { Close(); return false; }
  return true;
 }
 void SetMirror(int slot) {
  const auto volume=NextServoVolume(channelVolume,slot);
  if(volume!=channelVolume) { channelVolume=volume; if(device) waveOutSetVolume(device,volume); }
 }
 void SetPlaying(bool value) {
  if(!device || value==playing) return;
  if(value) waveOutRestart(device); else waveOutPause(device);
  playing=value;
 }
 void Close() {
  if(device) { waveOutReset(device); if(header.dwFlags&WHDR_PREPARED) waveOutUnprepareHeader(device,&header,sizeof(header)); waveOutClose(device); }
  device=nullptr; header={}; playing=false; samples.clear();
 }
 ~ServoLoop() { Close(); }
};
inline bool ServoMovement(int slot,unsigned char mask,bool enabled,bool allowed) {
 if(slot==7) mask&=3; // Extension uses only the horizontal adjustment pair.
 if(slot==8) mask&=12; // Tilt uses only the vertical adjustment pair.
 const bool horizontal=((mask&1)!=0)!=((mask&2)!=0);
 const bool vertical=((mask&4)!=0)!=((mask&8)!=0);
 return enabled && allowed && slot>=0 && (horizontal||vertical);
}
