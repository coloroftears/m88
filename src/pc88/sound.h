// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1997, 2001.
// ---------------------------------------------------------------------------
//  $Id: sound.h,v 1.28 2003/05/12 22:26:35 cisc Exp $

#pragma once

#include "common/device.h"
#include "common/sampling_rate_converter.h"
#include "common/sound_source.h"

#include <limits.h>
#include <vector>

// ---------------------------------------------------------------------------

class Scheduler;

namespace pc88core {

class Config;
class Sound;
class PC88;

class Sound : public Device,
              public ISoundControl,
              protected SoundSource<Sample32> {
 public:
  ~Sound();

  void IOCALL UpdateCounter(uint32_t);

  // Overrides ISoundControl.
  bool IFCALL Connect(ISoundSource* src) final;
  bool IFCALL Disconnect(ISoundSource* src) final;
  bool IFCALL Update(ISoundSource* src = nullptr) final;
  int IFCALL GetSubsampleTime(ISoundSource* src) final;

  void FillWhenEmpty(bool f) { soundbuf_.FillWhenEmpty(f); }

  int Get(Sample16* dest, int size);

  // Overrides SoundSource<Sample32>.
  int Get(Sample32* dest, int size) final;
  uint32_t GetRate() const final { return mix_rate_; }
  int GetChannels() const final { return 2; }
  int GetAvail() const final { return INT_MAX; }

 protected:
  Sound();

  bool Init(PC88* pc, uint32_t rate, int bufsize);
  void Cleanup();

  void ApplyConfig(const Config* config);
  bool SetRate(uint32_t rate, int bufsize);

  SoundSource<Sample16>* GetSoundSource() { return &soundbuf_; }

 private:
  //  SoundBuffer2 soundbuf_;
  SamplingRateConverter soundbuf_;

  PC88* pc_;

  std::unique_ptr<int32_t[]> mixingbuf_;
  int buffer_size_;

  uint32_t prev_time_;
  int tdiff_;
  uint32_t mix_threashold_;

  bool enabled_ = false;

  uint32_t mix_rate_;
  uint32_t sampling_rate_;  // サンプリングレート
  uint32_t rate50_;        // sampling_rate_ / 50

  std::vector<ISoundSource*> sslist_;
  CriticalSection cs_;
};
}  // namespace pc88core
