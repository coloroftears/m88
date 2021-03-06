// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  画面描画関係
// ---------------------------------------------------------------------------
//  $Id: windraw.h,v 1.19 2002/04/07 05:40:11 cisc Exp $

#pragma once

#include <windows.h>

#include <stdint.h>

#include <memory>

#include "common/critical_section.h"
#include "common/draw.h"
#include "common/guid.h"

// ---------------------------------------------------------------------------

class WinDrawSub {
 public:
  WinDrawSub() : status(0) {}
  virtual ~WinDrawSub() {}

  virtual bool Init(HWND hwnd, uint32_t w, uint32_t h, GUID* display) = 0;
  virtual bool Resize(uint32_t width, uint32_t height) { return false; }
  virtual bool Cleanup() = 0;
  virtual void SetPalette(PALETTEENTRY* pal, int index, int nentries) {}
  virtual void QueryNewPalette() {}
  virtual void DrawScreen(const RECT& rect, bool refresh) = 0;

  virtual bool Lock(uint8_t** pimage, int* pbpl) { return false; }
  virtual bool Unlock() { return true; }

  virtual void SetGUIMode(bool gui) {}
  virtual uint32_t GetStatus() { return status; }
  virtual void Flip() {}
  virtual bool SetFlipMode(bool flip) { return false; }
  virtual void WindowMoved(int cx, int cy) { return; }

 protected:
  uint32_t status;
};

// ---------------------------------------------------------------------------

class WinDraw final : public Draw {
 public:
  WinDraw();
  ~WinDraw() final;
  bool Init0(HWND hwindow);

  // - Draw Common Interface
  // Overrides Draw.
  bool Init(uint32_t w, uint32_t h, uint32_t bpp) final;
  bool Cleanup() final;
  bool Lock(uint8_t** pimage, int* pbpl) final;
  bool Unlock() final;
  uint32_t GetStatus() final;
  void Resize(uint32_t width, uint32_t height) final;
  void DrawScreen(const Region& region) final;
  void SetPalette(uint32_t index, uint32_t nents, const Palette* pal) final;
  void Flip() final;
  bool SetFlipMode(bool f) final;

  // - Unique Interface
  int GetDrawCount() {
    int ret = drawcount;
    drawcount = 0;
    return ret;
  }
  void RequestPaint();
  void QueryNewPalette(bool background);
  void Activate(bool f) { is_active_ = f; }

  void SetPriorityLow(bool low);
  void SetGUIFlag(bool flag);
  bool ChangeDisplayMode(bool fullscreen, bool force480 = true);
  void Refresh() { refresh_ = true; }
  void WindowMoved(int cx, int cy);

  int CaptureScreen(uint8_t* dest);

 private:
  enum DisplayType { None, GDI, DDWin, DDFull };
  void PaintWindow();

  static BOOL WINAPI DDEnumCallback(GUID FAR* guid,
                                    LPSTR desc,
                                    LPSTR name,
                                    LPVOID context,
                                    HMONITOR hm);

  uint32_t ThreadMain();
  static uint32_t __stdcall ThreadEntry(LPVOID arg);

  uint32_t idthread = 0;
  HANDLE hthread = 0;

  DisplayType drawtype = None;

  int palcngbegin;  // パレット変更エントリの最初
  int palcngend;    // パレット変更エントリの最後
  int palrgnbegin;  // 使用中パレットの最初
  int palrgnend;    // 使用中パレットの最後

  volatile bool should_terminate_ = false;  // スレッド終了要求
  volatile bool drawing_ = false;           // 画面を書き換え中

  bool draw_all_ = false;  // 画面全体を書き換える
  bool is_active_ = false;
  bool has_palette_ = false;  // パレットを持っている
  bool refresh_ = false;
  bool locked_ = false;
  bool flipmode_ = false;

  RECT drawarea;  // 書き換える領域
  int drawcount = 0;
  int guicount = 0;

  int width;
  int height;

  std::unique_ptr<WinDrawSub> draw_sub_;

  CriticalSection csdraw;

  HWND hwnd = 0;
  HANDLE hevredraw = 0;
  HMONITOR hmonitor;  // 探索中の hmonitor
  GUID gmonitor;      // hmonitor に対応する GUID

  PALETTEENTRY palette[0x100];
};
