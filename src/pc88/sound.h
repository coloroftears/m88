// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1997, 2001.
// ---------------------------------------------------------------------------
//  $Id: sound.h,v 1.28 2003/05/12 22:26:35 cisc Exp $

#pragma once

#include "common/device.h"
#include "common/lpf.h"
#include "common/sampling_rate_converter.h"
#include "common/sound_source.h"

#include <limits.h>
#include <vector>

// ---------------------------------------------------------------------------

class PC88;
class Scheduler;

namespace pc88 {
class Sound;
class Config;

class Sound : public Device,
              public ISoundControl,
              protected SoundSource<Sample32> {
 public:
  Sound();
  ~Sound();

  bool Init(PC88* pc, uint32_t rate, int bufsize);
  void Cleanup();

  void ApplyConfig(const Config* config);
  bool SetRate(uint32_t rate, int bufsize);

  void IOCALL UpdateCounter(uint32_t);

  // Overrides ISoundControl.
  bool IFCALL Connect(ISoundSource* src) final;
  bool IFCALL Disconnect(ISoundSource* src) final;
  bool IFCALL Update(ISoundSource* src = nullptr) final;
  int IFCALL GetSubsampleTime(ISoundSource* src) final;

  void FillWhenEmpty(bool f) { soundbuf.FillWhenEmpty(f); }

  SoundSource<Sample16>* GetSoundSource() { return &soundbuf; }

  int Get(Sample16* dest, int size);

  // Overrides SoundSource<Sample32>.
  int Get(Sample32* dest, int size) final;
  uint32_t GetRate() const final { return mixrate; }
  int GetChannels() const final { return 2; }
  int GetAvail() const final { return INT_MAX; }

 protected:
  uint32_t mixrate;
  uint32_t samplingrate;  // サンプリングレート
  uint32_t rate50;        // samplingrate / 50

 private:
  //  SoundBuffer2 soundbuf;
  SamplingRateConverter soundbuf;

  PC88* pc;

  std::unique_ptr<int32_t[]> mixingbuf_;
  int buffersize;

  uint32_t prevtime;
  uint32_t cfgflg;
  int tdiff;
  uint32_t mixthreshold;

  bool enabled;

  std::vector<ISoundSource*> sslist_;
  CriticalSection cs_ss;
};
}  // namespace pc88
