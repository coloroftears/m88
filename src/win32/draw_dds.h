// ---------------------------------------------------------------------------
//  M88 - PC8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  DirectDraw による全画面描画
// ---------------------------------------------------------------------------
//  $Id: DrawDDS.h,v 1.10 2002/04/07 05:40:10 cisc Exp $

#pragma once

#include <ddraw.h>

#include "win32/windraw.h"

// ---------------------------------------------------------------------------

class WinDrawDDS final : public WinDrawSub {
 public:
  WinDrawDDS(bool force480 = true);
  ~WinDrawDDS();

  // Overrides WinDrawSub.
  bool Init(HWND hwnd, uint32_t w, uint32_t h, GUID* drv) final;
  bool Resize(uint32_t w, uint32_t h) final;
  bool Cleanup() final;
  void SetPalette(PALETTEENTRY* pal, int index, int nentries) final;
  void QueryNewPalette() final;
  void DrawScreen(const RECT& rect, bool refresh) final;
  bool Lock(uint8_t** pimage, int* pbpl) final;
  bool Unlock() final;
  void SetGUIMode(bool guimode) final;
  void Flip() final;
  bool SetFlipMode(bool f) final;

 private:
  void FillBlankArea();
  bool RestoreSurface();
  bool SetScreenMode();
  bool CreateDDPalette();
  bool CreateDD2(GUID* monitor);
  bool CreateDDS();
  bool Update(LPDIRECTDRAWSURFACE, const RECT& rect);

  static HRESULT WINAPI EDMCallBack(LPDDSURFACEDESC, LPVOID);

  HWND hwnd;

  LPDIRECTDRAW2 ddraw;
  LPDIRECTDRAWPALETTE ddpal;
  LPDIRECTDRAWSURFACE ddsscrn;
  LPDIRECTDRAWSURFACE ddsback;
  LPDIRECTDRAWCLIPPER ddcscrn;

  uint32_t lines;  // 400 or 480
  uint32_t width;
  uint32_t height;
  POINT ltc;

  uint8_t* image;

  bool palchanged;
  bool guimode;

  PALETTEENTRY palentry[256];
};
