// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  GDI による画面描画
// ---------------------------------------------------------------------------
//  $Id: DrawGDI.h,v 1.6 2001/02/21 11:58:53 cisc Exp $

#pragma once

// ---------------------------------------------------------------------------

#include <stdint.h>

#include <memory>

#include "win32/windraw.h"

class WinDrawGDI final : public WinDrawSub {
 public:
  WinDrawGDI();
  ~WinDrawGDI();

  // Overrides WinDrawSub.
  bool Init(HWND hwnd, uint32_t w, uint32_t h, GUID*) final;
  bool Resize(uint32_t width, uint32_t height) final;
  bool Cleanup() final;
  void SetPalette(PALETTEENTRY* pal, int index, int nentries) final;
  void DrawScreen(const RECT& rect, bool refresh) final;
  bool Lock(uint8_t** pimage, int* pbpl) final;
  bool Unlock() final;

 private:
  struct BI256  // BITMAPINFO
  {
    BITMAPINFOHEADER header;
    RGBQUAD colors[256];
  };

 private:
  bool MakeBitmap();

  HWND hwnd = 0;
  HBITMAP hbitmap = 0;
  uint8_t* image_;

  // Bytes per Line
  int bpl_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  bool updatepal_;
  BI256 binfo;
};
