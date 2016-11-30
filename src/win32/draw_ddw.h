// ---------------------------------------------------------------------------
//  M88 - PC8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  DirectDraw によるウインドウ画面描画
// ---------------------------------------------------------------------------
//  $Id: drawddw.h,v 1.7 2002/04/07 05:40:10 cisc Exp $

#pragma once

#include <ddraw.h>

#include "win32/windraw.h"

// ---------------------------------------------------------------------------

class WinDrawDDW final : public WinDrawSub {
 public:
  WinDrawDDW();
  ~WinDrawDDW();

  // Overrides WinDrawSub.
  bool Init(HWND hwnd, uint32_t w, uint32_t h, GUID*) final;
  bool Resize(uint32_t width, uint32_t height) final;
  bool Cleanup() final;
  void SetPalette(PALETTEENTRY* pal, int index, int nentries) final;
  void QueryNewPalette() final;
  void DrawScreen(const RECT& rect, bool refresh) final;
  bool Lock(uint8_t** pimage, int* pbpl) final;
  bool Unlock() final;

 private:
  bool CreateDDPalette();
  bool CreateDD2();
  bool CreateDDSPrimary();
  bool CreateDDSWork();
  bool RestoreSurface();
  HWND hwnd;

  static void Convert8bpp(void* dest,
                          const uint8_t* src,
                          RECT* rect,
                          int pitch);

  LPDIRECTDRAW2 ddraw;
  LPDIRECTDRAWPALETTE ddpal;

  LPDIRECTDRAWSURFACE ddsprimary;
  LPDIRECTDRAWSURFACE ddsscrn;
  LPDIRECTDRAWCLIPPER ddcscrn;

  LPDIRECTDRAWSURFACE ddswork;

  uint32_t redmask;
  uint32_t greenmask;
  uint32_t bluemask;
  uint8_t redshift;
  uint8_t greenshift;
  uint8_t blueshift;
  bool scrnhaspal;
  bool palchanged;
  bool locked;

  uint32_t width;
  uint32_t height;

  PALETTEENTRY palentry[256];
};
