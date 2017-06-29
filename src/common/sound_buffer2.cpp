//  $Id: sndbuf2.cpp,v 1.2 2003/05/12 22:26:34 cisc Exp $

#include "common/sound_buffer2.h"

#include <string.h>

#include <algorithm>

// ---------------------------------------------------------------------------
//  Sound Buffer
//
SoundBuffer2::SoundBuffer2()
    : source_(nullptr),
      buffer_(nullptr),
      buffersize_(0),
      read_(0),
      write_(0),
      ch_(0),
      fillwhenempty_(true) {}

SoundBuffer2::~SoundBuffer2() {
  Cleanup();
}

bool SoundBuffer2::Init(SoundSource<Sample16>* source, int buffersize) {
  CriticalSection::Lock lock(cs_);

  delete[] buffer_;
  buffer_ = nullptr;

  source_ = nullptr;
  if (!source)
    return true;

  buffersize_ = buffersize;

  ch_ = source->GetChannels();
  read_ = 0;
  write_ = 0;

  if (!ch_ || buffersize_ <= 0)
    return false;

  buffer_ = new Sample16[ch_ * buffersize_];
  if (!buffer_)
    return false;

  memset(buffer_, 0, ch_ * buffersize_ * sizeof(Sample16));
  source_ = source;
  return true;
}

void SoundBuffer2::Cleanup() {
  CriticalSection::Lock lock(cs_);

  delete[] buffer_;
  buffer_ = nullptr;
}

// ---------------------------------------------------------------------------
//  バッファに音を追加
//
int SoundBuffer2::Fill(int samples) {
  CriticalSection::Lock lock(cs_);
  if (source_)
    return FillMain(samples);
  return 0;
}

int SoundBuffer2::FillMain(int samples) {
  // リングバッファの空きを計算
  int free = buffersize_ - GetAvail();

  if (!fillwhenempty_ && (samples > free - 1)) {
    int skip = std::min(samples - free + 1, buffersize_ - free);
    free += skip;
    read_ += skip;
    if (read_ > buffersize_)
      read_ -= buffersize_;
  }

  // 書きこむべきデータ量を計算
  samples = std::min(samples, free - 1);
  if (samples > 0) {
    // 書きこむ
    if (buffersize_ - write_ >= samples) {
      // 一度で書ける場合
      source_->Get(buffer_ + write_ * ch_, samples);
    } else {
      // ２度に分けて書く場合
      source_->Get(buffer_ + write_ * ch_, buffersize_ - write_);
      source_->Get(buffer_, samples - (buffersize_ - write_));
    }
    write_ += samples;
    if (write_ >= buffersize_)
      write_ -= buffersize_;
  }
  return samples;
}

// ---------------------------------------------------------------------------
//  バッファから音を貰う
//
int SoundBuffer2::Get(Sample16* dest, int samples) {
  CriticalSection::Lock lock(cs_);
  if (!buffer_)
    return 0;

  for (int s = samples; s > 0;) {
    int xsize = std::min(s, buffersize_ - read_);

    int avail = GetAvail();

    // 供給不足なら追加
    if (xsize <= avail || fillwhenempty_) {
      if (xsize > avail)
        FillMain(xsize - avail);
      memcpy(dest, buffer_ + read_ * ch_, xsize * ch_ * sizeof(Sample16));
      dest += xsize * ch_;
      read_ += xsize;
    } else {
      if (avail > 0) {
        memcpy(dest, buffer_ + read_ * ch_, avail * ch_ * sizeof(Sample16));
        dest += avail * ch_;
        read_ += avail;
      }
      memset(dest, 0, (xsize - avail) * ch_ * sizeof(Sample16));
      dest += (xsize - avail) * ch_;
    }

    s -= xsize;
    if (read_ >= buffersize_)
      read_ -= buffersize_;
  }
  return samples;
}

// ---------------------------------------------------------------------------
//  バッファが空か，空に近い状態か?
//
bool SoundBuffer2::IsEmpty() {
  return GetAvail() == 0;
}
