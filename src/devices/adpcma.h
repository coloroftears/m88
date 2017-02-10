#pragma once

#include <inttypes.h>

#include "devices/fmgen.h"

namespace fmgen {

class StatusSink;

struct ADPCMAConfig {
  ADPCMAConfig() {
    for (int i = 0; i < 32; ++i)
      adpcmareg[i] = 0;
  }

  uint8_t* adpcmabuf = nullptr;  // ADPCMA ROM
  int adpcmasize = 0;
  int adpcmatvol = 0;
  int adpcmastep = 0;
  int8_t adpcmatl = 0;  // ADPCMA 全体の音量
  uint8_t adpcmakey = 0;
  uint8_t adpcmareg[32];
};

class ADPCMA {
 public:
  ADPCMA();

  static void InitTable();
  bool Init(ADPCMAConfig* config, StatusSink* sink, int ch);
  void Reset();
  void ResetDecoder();
  void Mix(FMSample* buffer, uint32_t count, int vol, bool mute);

  // Decode data and update |x_| and |delta_|
  inline void Update(int32_t data);

  uint8_t pan = 0;
  // TODO: investigate usage of |level| bit7.
  uint8_t level = 0;
  uint32_t pos = 0;
  uint32_t step = 0;

  uint32_t start = 0;
  uint32_t stop = 0;

  int volume = 0;

  int x_ = 0;
  int delta_ = 0;
  uint32_t nibble_ = 0;  // next 4bit

 private:
  ADPCMAConfig* config_ = nullptr;
  StatusSink* sink_ = nullptr;

  int ch_;
};

}  // namespace fmgen
