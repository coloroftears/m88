//  $Id: srcbuf.h,v 1.2 2003/05/12 22:26:34 cisc Exp $

#pragma once

#include <memory>

#include "common/critical_section.h"
#include "common/sound_source.h"

// ---------------------------------------------------------------------------
//  SamplingRateConverter
//
class SamplingRateConverter final : public SoundSource<Sample16> {
 public:
  SamplingRateConverter() {}
  ~SamplingRateConverter() final {}

  bool Init(SoundSource<Sample32>* source,
            int bufsize,
            uint32_t outrate);  // bufsize はサンプル単位

  // Overrides SoundSource<Sample16>.
  int Get(Sample16* dest, int size) final;
  uint32_t GetRate() const final;
  int GetChannels() const final;
  int GetAvail() const final;

  int Fill(int samples);  // バッファに最大 sample 分データを追加
  bool IsEmpty() const;
  void FillWhenEmpty(bool f);  // バッファが空になったら補充するか

 private:
  enum {
    osmax = 500,
    osmin = 100,
    M = 30,  // M
  };

  int FillInternal(int samples);
  void MakeFilter(uint32_t outrate);
  int Avail() const;

  SoundSource<Sample32>* source = nullptr;
  std::unique_ptr<Sample32[]> buffer;
  std::unique_ptr<float[]> h2;

  int buffersize = 0;  // バッファのサイズ (in samples)
  int read = 0;        // 読込位置 (in samples)
  int write = 0;       // 書き込み位置 (in samples)
  int ch = 2;          // チャネル数(1sample = ch*Sample)
  bool fillwhenempty = true;

  int n = 0;
  int oo = 0;
  int ic = 0;
  int oc = 0;

  int outputrate = 0;

  mutable CriticalSection cs;
};

// ---------------------------------------------------------------------------

inline void SamplingRateConverter::FillWhenEmpty(bool f) {
  fillwhenempty = f;
}

inline uint32_t SamplingRateConverter::GetRate() const {
  return source ? outputrate : 0;
}

inline int SamplingRateConverter::GetChannels() const {
  return source ? ch : 0;
}

// ---------------------------------------------------------------------------
//  バッファが空か，空に近い状態か?
//
inline int SamplingRateConverter::Avail() const {
  if (write >= read)
    return write - read;
  return buffersize + write - read;
}

inline int SamplingRateConverter::GetAvail() const {
  CriticalSection::Lock lock(cs);
  return Avail();
}

inline bool SamplingRateConverter::IsEmpty() const {
  return GetAvail() == 0;
}
