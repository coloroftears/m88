//  $Id: sndbuf2.h,v 1.2 2003/05/12 22:26:34 cisc Exp $

#pragma once

#include "common/critical_section.h"
#include "common/sound_source.h"
#include "interface/ifcommon.h"

// ---------------------------------------------------------------------------
//  SoundBuffer2
//
class SoundBuffer2 final : public SoundSource<Sample16> {
 public:
  SoundBuffer2();
  ~SoundBuffer2();

  bool Init(SoundSource<Sample16>* source,
            int bufsize);  // bufsize はサンプル単位
  void Cleanup();

  // Overrides SoundSource<Sample16>.
  int Get(Sample16* dest, int size) final;
  uint32_t GetRate() const final;
  int GetChannels() const final;
  int GetAvail() const final;

  int Fill(int samples);  // バッファに最大 sample 分データを追加
  bool IsEmpty();
  void FillWhenEmpty(bool f);  // バッファが空になったら補充するか

 private:
  int FillMain(int samples);

  CriticalSection cs;

  SoundSource<Sample16>* source;
  Sample16* buffer;
  int buffersize;  // バッファのサイズ (in samples)
  int read;        // 読込位置 (in samples)
  int write;       // 書き込み位置 (in samples)
  int ch;          // チャネル数(1sample = ch*Sample)
  bool fillwhenempty;
};

// ---------------------------------------------------------------------------

inline void SoundBuffer2::FillWhenEmpty(bool f) {
  fillwhenempty = f;
}

inline uint32_t SoundBuffer2::GetRate() const {
  return source ? source->GetRate() : 0;
}

inline int SoundBuffer2::GetChannels() const {
  return source ? ch : 0;
}

inline int SoundBuffer2::GetAvail() const {
  int avail = 0;
  if (write >= read)
    avail = write - read;
  else
    avail = buffersize + write - read;
  return avail;
}
