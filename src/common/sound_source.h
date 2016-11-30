//  $Id: soundsrc.h,v 1.2 2003/05/12 22:26:34 cisc Exp $

#pragma once

#include "common/types.h"

using Sample = int16_t;

class SoundSource {
 public:
  virtual int Get(Sample* dest, int size) = 0;
  virtual uint32_t GetRate() = 0;
  virtual int GetChannels() = 0;
  virtual int GetAvail() = 0;
};

using SampleL = int32_t;

class SoundSourceL {
 public:
  virtual int Get(SampleL* dest, int size) = 0;
  virtual uint32_t GetRate() = 0;
  virtual int GetChannels() = 0;
  virtual int GetAvail() = 0;
};
