// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  GDI による画面描画
// ---------------------------------------------------------------------------
//  $Id: DrawGDI.h,v 1.6 2001/02/21 11:58:53 cisc Exp $

#pragma once

// ---------------------------------------------------------------------------

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

  HBITMAP hbitmap;
  uint8_t* bitmapimage;
  HWND hwnd;
  uint8_t* image;
  int bpl;
  uint32_t width;
  uint32_t height;
  bool updatepal;
  BI256 binfo;
};
