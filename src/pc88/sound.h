// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1997, 2001.
// ---------------------------------------------------------------------------
//  $Id: sound.h,v 1.28 2003/05/12 22:26:35 cisc Exp $

#pragma once

#include "common/device.h"
#include "common/sound_buffer2.h"
#include "common/sampling_rate_converter.h"
#include "common/lpf.h"

#include <limits.h>

// ---------------------------------------------------------------------------

class PC88;
class Scheduler;

namespace PC8801 {
class Sound;
class Config;

class Sound : public Device, public ISoundControl, protected SoundSourceL {
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
  bool IFCALL Update(ISoundSource* src = 0) final;
  int IFCALL GetSubsampleTime(ISoundSource* src) final;

  void FillWhenEmpty(bool f) { soundbuf.FillWhenEmpty(f); }

  SoundSource* GetSoundSource() { return &soundbuf; }

  int Get(Sample* dest, int size);
  int Get(SampleL* dest, int size);
  uint32_t GetRate() { return mixrate; }
  int GetChannels() { return 2; }

  int GetAvail() { return INT_MAX; }

 protected:
  uint32_t mixrate;
  uint32_t samplingrate;  // サンプリングレート
  uint32_t rate50;        // samplingrate / 50

 private:
  struct SSNode {
    ISoundSource* ss;
    SSNode* next;
  };

  //  SoundBuffer2 soundbuf;
  SamplingRateConverter soundbuf;

  PC88* pc;

  int32_t* mixingbuf;
  int buffersize;

  uint32_t prevtime;
  uint32_t cfgflg;
  int tdiff;
  uint32_t mixthreshold;

  bool enabled;

  SSNode* sslist;
  CriticalSection cs_ss;
};
}  // namespace PC8801
