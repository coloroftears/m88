//  $Id: soundsrc.h,v 1.2 2003/05/12 22:26:34 cisc Exp $

#pragma once

#include "common/types.h"

using Sample16 = int16_t;
using Sample32 = int32_t;

template <class T>
class SoundSource {
 public:
  virtual ~SoundSource() {}
  virtual int Get(T* dest, int size) = 0;
  virtual uint32_t GetRate() const = 0;
  virtual int GetChannels() const = 0;
  virtual int GetAvail() const = 0;
};
