// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//  $Id: draw.h,v 1.7 2000/02/11 00:41:52 cisc Exp $

#pragma once

#include <stdint.h>

#include <algorithm>

// ---------------------------------------------------------------------------

class Draw {
 public:
  struct Palette {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t rsvd;
  };

  struct Region {
    void Reset() {
      top = left = 32767;
      bottom = right = -1;
    }
    bool Valid() const { return top <= bottom; }
    void Update(int l, int t, int r, int b) {
      left = std::min(left, l);
      right = std::max(right, r);
      top = std::min(top, t);
      bottom = std::max(bottom, b);
    }
    void Update(int t, int b) { Update(0, t, 640, b); }

    int left;
    int top;
    int right;
    int bottom;
  };

  enum Status {
    readytodraw = 1 << 0,    // 更新できることを示す
    shouldrefresh = 1 << 1,  // DrawBuffer をまた書き直す必要がある
    flippable = 1 << 2,      // flip が実装してあることを示す
  };

 public:
  Draw() {}
  virtual ~Draw() {}

  virtual bool Init(uint32_t width, uint32_t height, uint32_t bpp) = 0;
  virtual bool Cleanup() = 0;

  virtual bool Lock(uint8_t** pimage, int* pbpl) = 0;
  virtual bool Unlock() = 0;

  virtual uint32_t GetStatus() = 0;
  virtual void Resize(uint32_t width, uint32_t height) = 0;
  virtual void DrawScreen(const Region& region) = 0;
  virtual void SetPalette(uint32_t index,
                          uint32_t nents,
                          const Palette* pal) = 0;
  virtual void Flip() {}
  virtual bool SetFlipMode(bool flip) = 0;
};
