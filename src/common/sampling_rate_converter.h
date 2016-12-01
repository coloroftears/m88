//  $Id: srcbuf.h,v 1.2 2003/05/12 22:26:34 cisc Exp $

#pragma once

#include "common/critical_section.h"
#include "common/sound_source.h"

// ---------------------------------------------------------------------------
//  SamplingRateConverter
//
class SamplingRateConverter final : public SoundSource<Sample16> {
 public:
  SamplingRateConverter();
  ~SamplingRateConverter();

  bool Init(SoundSource<Sample32>* source,
            int bufsize,
            uint32_t outrate);  // bufsize はサンプル単位
  void Cleanup();

  // Overrides SoundSource.
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

  int FillMain(int samples);
  void MakeFilter(uint32_t outrate);
  int Avail() const;

  SoundSource<Sample32>* source;
  Sample32* buffer;
  float* h2;

  int buffersize;  // バッファのサイズ (in samples)
  int read;        // 読込位置 (in samples)
  int write;       // 書き込み位置 (in samples)
  int ch;          // チャネル数(1sample = ch*Sample)
  bool fillwhenempty;

  int n;
  int nch;
  int oo;
  int ic;
  int oc;

  int outputrate;

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
