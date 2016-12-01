// ---------------------------------------------------------------------------
//  M88 - PC-88 Emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  Sound Implemention for Win32
// ---------------------------------------------------------------------------
//  $Id: sounddrv.h,v 1.3 2002/05/31 09:45:21 cisc Exp $

#pragma once

#include <stdint.h>
#include "common/sound_source.h"

// ---------------------------------------------------------------------------

namespace WinSoundDriver {

class Driver {
 public:
  //  using Sample = SoundBuffer::Sample;

  Driver() {}
  virtual ~Driver() {}

  virtual bool Init(SoundSource<Sample16>* sb,
                    HWND hwnd,
                    uint32_t rate,
                    uint32_t ch,
                    uint32_t buflen) = 0;
  virtual bool Cleanup() = 0;
  void MixAlways(bool yes) { mixalways = yes; }

 protected:
  SoundSource<Sample16>* src;
  uint32_t buffersize;
  uint32_t sampleshift;
  volatile bool playing;
  bool mixalways;
};
}  // namespace WinSoundDriver
