#pragma once
#include "ServoLoop.hpp"

// A separate output stream lets the click overlap speech and the motor loop.
class SelectionClick {
 HWAVEOUT device=nullptr;
 WAVEHDR header{};
 std::vector<char> samples;
public:
 bool Open(const std::filesystem::path& path) {
  Close();
  std::ifstream file(path,std::ios::binary); ServoWaveHeader wav{};
  if(!file.read(reinterpret_cast<char*>(&wav),sizeof(wav)) || !ValidServoHeader(wav)) return false;
  samples.resize(wav.bytes);
  if(!file.read(samples.data(),wav.bytes)) { samples.clear(); return false; }
  WAVEFORMATEX format{}; format.wFormatTag=WAVE_FORMAT_PCM; format.nChannels=wav.channels;
  format.nSamplesPerSec=wav.rate; format.wBitsPerSample=wav.bits;
  format.nBlockAlign=wav.align; format.nAvgBytesPerSec=wav.byteRate;
  if(waveOutOpen(&device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR) { device=nullptr; samples.clear(); return false; }
  header.lpData=samples.data(); header.dwBufferLength=wav.bytes;
  if(waveOutPrepareHeader(device,&header,sizeof(header))!=MMSYSERR_NOERROR) { Close(); return false; }
  return true;
 }
 bool Play(int slot,DWORD volume=0) {
  if(!device) return false;
  waveOutReset(device);
  waveOutSetVolume(device,volume?volume:ServoChannelVolume(slot));
  return waveOutWrite(device,&header,sizeof(header))==MMSYSERR_NOERROR;
 }
 void Stop() { if(device) waveOutReset(device); }
 void Close() {
  if(device) { waveOutReset(device); if(header.dwFlags&WHDR_PREPARED) waveOutUnprepareHeader(device,&header,sizeof(header)); waveOutClose(device); }
  device=nullptr; header={}; samples.clear();
 }
 ~SelectionClick() { Close(); }
};
