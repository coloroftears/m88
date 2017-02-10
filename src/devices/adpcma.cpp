#include "devices/adpcma.h"

#include "devices/opna.h"

#include <math.h>

namespace fmgen {

namespace {
int jedi_table[(48 + 1) * 16];
}  // namespace

ADPCMA::ADPCMA() {}

// static
void ADPCMA::InitTable() {
  const static int8_t table2[] = {
      1, 3, 5, 7, 9, 11, 13, 15, -1, -3, -5, -7, -9, -11, -13, -15,
  };

  for (int i = 0; i <= 48; i++) {
    int s = static_cast<int>(16.0 * pow(1.1, i) * 3);
    for (int j = 0; j < 16; j++)
      jedi_table[i * 16 + j] = s * table2[j] / 8;
  }
}

bool ADPCMA::Init(ADPCMAConfig* config, StatusSink* sink, int ch) {
  config_ = config;
  sink_ = sink;
  ch_ = ch;
  return true;
}

void ADPCMA::Reset() {
  pan = 0;
  level = 0;
  pos = 0;
  step = 0;

  start = 0;
  stop = 0;

  volume = 0;

  ResetDecoder();
}

void ADPCMA::ResetDecoder() {
  x_ = 0;
  delta_ = 0;
  nibble_ = 0;
}

void ADPCMA::Update(int32_t data) {
  const static int decode_tableA1[16] = {
      -1 * 16, -1 * 16, -1 * 16, -1 * 16, 2 * 16, 5 * 16, 7 * 16, 9 * 16,
      -1 * 16, -1 * 16, -1 * 16, -1 * 16, 2 * 16, 5 * 16, 7 * 16, 9 * 16};

  x_ += jedi_table[delta_ + data];
  x_ = Limit(x_, 2048 * 3 - 1, -2048 * 3);
  delta_ += decode_tableA1[data];
  delta_ = Limit(delta_, 48 * 16, 0);
}

// ---------------------------------------------------------------------------
//  ADPCMA 合成
//
void ADPCMA::Mix(FMSample* buffer, uint32_t count, int vol, bool mute) {
  FMSample* limit = buffer + count * 2;

  uint32_t maskl = pan & 2 ? -1 : 0;
  uint32_t maskr = pan & 1 ? -1 : 0;

  if (mute)
    maskl = maskr = 0;

  FMSample* dest = buffer;
  for (; dest < limit; dest += 2) {
    step += config_->adpcmastep;
    if (pos >= stop) {
      sink_->SetStatus(0x100 << ch_);
      config_->adpcmakey &= ~(1 << ch_);
      break;
    }

    for (; step > 0x10000; step -= 0x10000) {
      int32_t data;
      if (!(pos & 1)) {
        nibble_ = config_->adpcmabuf[pos >> 1];
        data = nibble_ >> 4;
      } else {
        data = nibble_ & 0x0f;
      }
      pos++;

      Update(data);
    }
    int32_t sample = (x_ * vol) >> 10;
    StoreSample(dest[0], sample & maskl);
    StoreSample(dest[1], sample & maskr);
  }
}

}  // namespace fmgen
