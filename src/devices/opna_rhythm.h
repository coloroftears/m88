#pragma once

#include <memory>
#include "devices/fmgen.h"

namespace fmgen {

struct OPNARhythm {
 public:
  void Mix(FMSample* buffer, uint32_t count, int vol, bool mute);
  bool LoadSample(const char* path, uint32_t master_rate);
  void SetRate(uint32_t new_rate);
  void KeyOn() { pos = 0; }
  void Reset();
  bool IsValid() const { return !!sample; }

  int volume = 0;  // おんりょうせってい
  // TODO: Investigate bit7 of |level| usage.
  uint8_t level = 0;  // おんりょう
  uint8_t pan = 0;    // ぱん

 private:
  std::unique_ptr<int16_t[]> sample;  // さんぷる
  uint32_t pos = 0;                   // いち
  uint32_t size = 0;                  // さいず
  uint32_t step = 0;                  // すてっぷち
  uint32_t rate = 0;                  // さんぷるのれーと
};

}  // namespace fmgen
