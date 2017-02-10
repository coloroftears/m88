#include "devices/opna_rhythm.h"

#include <string.h>
#include <algorithm>

#include "common/file.h"

namespace fmgen {

// Used for sampling rate conversion.
const int kRhythmRateMultiplier = 1024;

bool OPNARhythm::LoadSample(const char* path, uint32_t master_rate) {
  FileIO file;
  uint32_t fsize;

  pos = ~0;

  if (!file.Open(path, FileIO::readonly))
    return false;

  struct {
    uint32_t chunksize;
    uint16_t tag;
    uint16_t nch;
    uint32_t rate;  // samples per second
    uint32_t avgbytes;
    uint16_t align;
    uint16_t bps;
    uint16_t size;
  } whdr;

  file.Seek(0x10, FileIO::begin);
  file.Read(&whdr, sizeof(whdr));

  uint8_t subchunkname[4];
  fsize = 4 + whdr.chunksize - sizeof(whdr);
  do {
    file.Seek(fsize, FileIO::current);
    file.Read(&subchunkname, 4);
    file.Read(&fsize, 4);
  } while (memcmp("data", subchunkname, 4));

  fsize /= 2;
  if (fsize >= 0x100000 || whdr.tag != 1 || whdr.nch != 1)
    return false;
  fsize = std::min(fsize, (1U << 31) / kRhythmRateMultiplier);

  sample.reset(new int16_t[fsize]);
  file.Read(sample.get(), fsize * 2);

  rate = whdr.rate;
  step = rate * kRhythmRateMultiplier / master_rate;
  pos = size = fsize * kRhythmRateMultiplier;

  return true;
}

void OPNARhythm::SetRate(uint32_t new_rate) {
  step = rate * kRhythmRateMultiplier / new_rate;
}

void OPNARhythm::Reset() {
  sample.reset();
}

void OPNARhythm::Mix(FMSample* buffer, uint32_t count, int vol, bool mute) {
  FMSample* limit = buffer + count * 2;
  const int maskl = -((pan >> 1) & 1);
  const int maskr = -(pan & 1);
  for (FMSample* dest = buffer; dest < limit && pos < size; dest += 2) {
    int sampledata = (sample[pos / kRhythmRateMultiplier] * vol) >> 12;
    pos += step;
    if (!mute) {
      StoreSample(dest[0], sampledata & maskl);
      StoreSample(dest[1], sampledata & maskr);
    }
  }
}

}  // namespace fmgen
