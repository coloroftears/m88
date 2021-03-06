// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//  画面関係 for Windows
// ---------------------------------------------------------------------------
//  $Id: windraw.cpp,v 1.34 2003/04/22 13:16:36 cisc Exp $

#include "win32/windraw.h"

#include <assert.h>

#include <algorithm>

#include "common/error.h"
#include "common/toast.h"
#include "win32/draw_dds.h"
#include "win32/draw_ddw.h"
#include "win32/draw_gdi.h"
#include "win32/messages.h"
#include "win32/monitors/loadmon.h"
#include "win32/winexapi.h"

#define LOGNAME "windraw"
#include "common/diag.h"
//#define DRAW_THREAD

// ---------------------------------------------------------------------------
// 構築/消滅
//
WinDraw::WinDraw() {}

WinDraw::~WinDraw() {
  Cleanup();
}

// ---------------------------------------------------------------------------
//  初期化
//
bool WinDraw::Init0(HWND hwindow) {
  hwnd = hwindow;
  draw_sub_.reset();
  drawtype = None;

  hthread = 0;
  hevredraw = CreateEvent(nullptr, FALSE, FALSE, nullptr);

  HDC hdc = GetDC(hwnd);
  has_palette_ = !!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE);
  ReleaseDC(hwnd, hdc);

#ifdef DRAW_THREAD
  if (!hthread)
    hthread = HANDLE(_beginthreadex(
        nullptr, 0, ThreadEntry, reinterpret_cast<void*>(this), 0, &idthread));
  if (!hthread) {
    Error::SetError(Errno::ThreadInitFailed);
    return false;
  }
#endif

  palcngbegin = palrgnbegin = 0x100;
  palcngend = palrgnend = -1;

  return true;
}

bool WinDraw::Init(uint32_t w, uint32_t h, uint32_t /*bpp*/) {
  width = w;
  height = h;
  should_terminate_ = false;
  is_active_ = true;
  return true;
}

// ---------------------------------------------------------------------------
//  後片付
//
bool WinDraw::Cleanup() {
  if (hthread) {
    SetThreadPriority(hthread, THREAD_PRIORITY_NORMAL);

    int i = 300;
    do {
      should_terminate_ = true;
      SetEvent(hevredraw);
    } while (--i > 0 && WAIT_TIMEOUT == WaitForSingleObject(hthread, 10));

    if (!i)
      TerminateThread(hthread, 0);

    CloseHandle(hthread), hthread = 0;
  }
  if (hevredraw)
    CloseHandle(hevredraw), hevredraw = 0;

  return true;
}

// ---------------------------------------------------------------------------
//  画面描画用スレッド
//
uint32_t WinDraw::ThreadMain() {
  drawing_ = false;
  while (!should_terminate_) {
    PaintWindow();
    WaitForSingleObject(hevredraw, 1000);
  }
  return 0;
}

uint32_t __stdcall WinDraw::ThreadEntry(LPVOID arg) {
  if (arg)
    return reinterpret_cast<WinDraw*>(arg)->ThreadMain();
  else
    return 0;
}

// ---------------------------------------------------------------------------
//  スレッドの優先順位を下げる
//
void WinDraw::SetPriorityLow(bool low) {
  if (hthread) {
    SetThreadPriority(
        hthread, low ? THREAD_PRIORITY_BELOW_NORMAL : THREAD_PRIORITY_NORMAL);
  }
}

// ---------------------------------------------------------------------------
//  パレット反映
//
void WinDraw::QueryNewPalette(bool /*bkgnd*/) {
  CriticalSection::Lock lock(csdraw);
  if (draw_sub_)
    draw_sub_->QueryNewPalette();
}

// ---------------------------------------------------------------------------
//  ペイントさせる
//
void WinDraw::RequestPaint() {
#ifdef DRAW_THREAD
  CriticalSection::Lock lock(csdraw);
  drawall = true;
  drawing = true;
  SetEvent(hevredraw);
#else
  draw_all_ = true;
  drawing_ = true;
  PaintWindow();
#endif
  //  Log("Request at %d\n", GetTickCount());
}

// ---------------------------------------------------------------------------
//  更新
//
void WinDraw::DrawScreen(const Region& region) {
  if (!drawing_) {
    Log("Draw %d to %d\n", region.top, region.bottom);
    drawing_ = true;
    drawarea.left = std::max(0, region.left);
    drawarea.top = std::max(0, region.top);
    drawarea.right = std::min(width, region.right);
    drawarea.bottom = std::min(height, region.bottom + 1);
#ifdef DRAW_THREAD
    SetEvent(hevredraw);
#else
    PaintWindow();
#endif
  }
}

// ---------------------------------------------------------------------------
//  描画する
//
void WinDraw::PaintWindow() {
  LOADBEGIN("WinDraw");
  CriticalSection::Lock lock(csdraw);
  if (drawing_ && draw_sub_ && is_active_) {
    RECT rect = drawarea;
    if (palcngbegin <= palcngend) {
      palrgnbegin = std::min(palcngbegin, palrgnbegin);
      palrgnend = std::max(palcngend, palrgnend);
      draw_sub_->SetPalette(palette + palcngbegin, palcngbegin,
                            palcngend - palcngbegin + 1);
      palcngbegin = 0x100;
      palcngend = -1;
    }
    draw_sub_->DrawScreen(rect, draw_all_);
    draw_all_ = false;
    if (rect.left < rect.right && rect.top < rect.bottom) {
      drawcount++;
      Log("\t\t\t(%3d,%3d)-(%3d,%3d)\n", rect.left, rect.top, rect.right - 1,
          rect.bottom - 1);
      // Toast::Show(100, 0, "(%.3d, %.3d)-(%.3d, %.3d)",
      //             rect.left, rect.top, rect.right-1, rect.bottom-1);
    }
    drawing_ = false;
  }
  LOADEND("WinDraw");
}

// ---------------------------------------------------------------------------
//  パレットをセット
//
void WinDraw::SetPalette(uint32_t index, uint32_t nents, const Palette* pal) {
  assert(0 <= index && index <= 0xff);
  assert(index + nents <= 0x100);
  assert(pal);

  memcpy(palette + index, pal, nents * sizeof(Palette));
  palcngbegin = std::min(palcngbegin, static_cast<int>(index));
  palcngend = std::max(palcngend, static_cast<int>(index + nents) - 1);
}

// ---------------------------------------------------------------------------
//  Lock
//
bool WinDraw::Lock(uint8_t** pimage, int* pbpl) {
  assert(pimage && pbpl);

  if (!locked_) {
    locked_ = true;
    csdraw.lock();
    if (draw_sub_ && draw_sub_->Lock(pimage, pbpl)) {
      // Lock に失敗することがある？
      assert(**pimage >= 0);
      return true;
    }
    csdraw.unlock();
    locked_ = false;
  }
  return false;
}

// ---------------------------------------------------------------------------
//  unlock
//
bool WinDraw::Unlock() {
  bool result = false;
  if (locked_) {
    if (draw_sub_)
      result = draw_sub_->Unlock();
    csdraw.unlock();
    locked_ = false;
    if (refresh_)
      refresh_ = false;
  }
  return result;
}

// ---------------------------------------------------------------------------
//  画面サイズを変える
//
void WinDraw::Resize(uint32_t w, uint32_t h) {
  // Toast::Show(50, 2500, "Resize (%d, %d)", width, height);
  width = w;
  height = h;
  if (draw_sub_)
    draw_sub_->Resize(width, height);
}

// ---------------------------------------------------------------------------
//  画面位置を変える
//
void WinDraw::WindowMoved(int x, int y) {
  if (draw_sub_)
    draw_sub_->WindowMoved(x, y);
}

// ---------------------------------------------------------------------------
//  画面描画ドライバの変更
//
bool WinDraw::ChangeDisplayMode(bool fullscreen, bool force480) {
  DisplayType type = fullscreen ? DDFull : DDWin;

  // 現在窓(M88)が所属するモニタの GUID を取得
  memset(&gmonitor, 0, sizeof(gmonitor));

  hmonitor = (*MonitorFromWin)(hwnd, MONITOR_DEFAULTTOPRIMARY);
  (*DDEnumerateEx)(DDEnumCallback, reinterpret_cast<LPVOID>(this),
                   DDENUM_ATTACHEDSECONDARYDEVICES);

  if (type != drawtype) {
    // 今までのドライバを廃棄
    if (draw_sub_)
      draw_sub_->SetGUIMode(true);
    {
      CriticalSection::Lock lock(csdraw);
      draw_sub_.reset();
    }

    // 新しいドライバの用意
    WinDrawSub* newdraw;
    switch (type) {
      case GDI:
      default:
        newdraw = new WinDrawGDI;
        break;
      case DDWin:
        newdraw = new WinDrawDDW;
        break;
      case DDFull:
        newdraw = new WinDrawDDS(force480);
        break;
    }

    bool result = true;

    if (!newdraw || !newdraw->Init(hwnd, width, height, &gmonitor)) {
      // 初期化に失敗した場合 GDI ドライバで再挑戦
      delete newdraw;

      if (type == DDFull)
        Toast::Show(50, 2500, "画面切り替えに失敗しました");
      else
        Toast::Show(120, 3000, "GDI ドライバを使用します");

      newdraw = new WinDrawGDI;
      type = GDI;
      result = false;

      if (!newdraw || !newdraw->Init(hwnd, width, height, 0)) {
        newdraw = nullptr;
        drawtype = None;
      }
    }

    if (newdraw) {
      newdraw->SetFlipMode(flipmode_);
      newdraw->SetGUIMode(false);
    }

    // 新しいドライバを使用可能にする
    {
      CriticalSection::Lock lock(csdraw);
      guicount = 0;
      draw_sub_.reset(newdraw);
    }

    draw_all_ = true;
    refresh_ = true;

    palrgnbegin = std::min(palcngbegin, palrgnbegin);
    palrgnend = std::max(palcngend, palrgnend);

    if (draw_sub_)
      draw_sub_->Resize(width, height);

    if (type == DDFull)
      ShowCursor(false);
    if (drawtype == DDFull)
      ShowCursor(true);
    drawtype = type;

    // refresh_ = true;

    return result;
  }
  return false;
}

// ---------------------------------------------------------------------------
//  現在の状態を得る
//
uint32_t WinDraw::GetStatus() {
  if (draw_sub_) {
    return (!drawing_ && is_active_ ? readytodraw : 0) |
           (refresh_ ? shouldrefresh : 0) | draw_sub_->GetStatus();
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  flip モードの設定
//
bool WinDraw::SetFlipMode(bool f) {
  flipmode_ = f;
  if (draw_sub_)
    return draw_sub_->SetFlipMode(flipmode_);
  return false;
}

// ---------------------------------------------------------------------------
//  flip する
//
void WinDraw::Flip() {
  if (draw_sub_)
    draw_sub_->Flip();
}

// ---------------------------------------------------------------------------
//  GUI 使用フラグ設定
//
void WinDraw::SetGUIFlag(bool usegui) {
  CriticalSection::Lock lock(csdraw);
  if (usegui) {
    if (!guicount++ && draw_sub_) {
      draw_sub_->SetGUIMode(true);
      ShowCursor(true);
    }
  } else {
    if (!--guicount && draw_sub_) {
      draw_sub_->SetGUIMode(false);
      ShowCursor(false);
    }
  }
}

// ---------------------------------------------------------------------------
//  画面を 640x400x4 の BMP に変換する
//  dest    変換した BMP の置き場所、置けるだけの領域が必要。
//  ret     巧くできたかどうか
//
int WinDraw::CaptureScreen(uint8_t* dest) {
  const bool half = false;
  if (!draw_sub_)
    return false;

  uint8_t* src = new uint8_t[640 * 400];
  if (!src)
    return false;

  uint8_t* s;
  int bpl;
  if (draw_sub_->Lock(&s, &bpl)) {
    uint8_t* d = src;
    for (int y = 0; y < 400; y++) {
      memcpy(d, s, 640);
      d += 640, s += bpl;
    }
    draw_sub_->Unlock();
  }

  // 構造体の準備

  BITMAPFILEHEADER* filehdr = (BITMAPFILEHEADER*)dest;
  BITMAPINFO* binfo = (BITMAPINFO*)(filehdr + 1);

  // headers
  ((char*)&filehdr->bfType)[0] = 'B';
  ((char*)&filehdr->bfType)[1] = 'M';
  filehdr->bfReserved1 = 0;
  filehdr->bfReserved2 = 0;
  binfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  binfo->bmiHeader.biWidth = 640;
  binfo->bmiHeader.biHeight = half ? 200 : 400;
  binfo->bmiHeader.biPlanes = 1;
  binfo->bmiHeader.biBitCount = 4;
  binfo->bmiHeader.biCompression = BI_RGB;
  binfo->bmiHeader.biSizeImage = 0;
  binfo->bmiHeader.biXPelsPerMeter = 0;
  binfo->bmiHeader.biYPelsPerMeter = 0;

  // １６色パレットの作成
  RGBQUAD* pal = binfo->bmiColors;
  memset(pal, 0, sizeof(RGBQUAD) * 16);

  uint8_t ctable[256];
  memset(ctable, 0, sizeof(ctable));

  int colors = 0;
  for (int index = 0; index < 144; index++) {
    RGBQUAD rgb;
    rgb.rgbBlue = palette[0x40 + index].peBlue;
    rgb.rgbRed = palette[0x40 + index].peRed;
    rgb.rgbGreen = palette[0x40 + index].peGreen;
    //      Log("c[%.2x] = G:%.2x R:%.2x B:%.2x\n", index, rgb.rgbGreen,
    //      rgb.rgbRed, rgb.rgbBlue);
    uint32_t entry = *((uint32_t*)&rgb);

    int k;
    for (k = 0; k < colors; k++) {
      if (!((*((uint32_t*)&pal[k]) ^ entry) & 0xffffff))
        goto match;
    }
    if (colors < 15) {
      //          Log("pal[%.2x] = G:%.2x R:%.2x B:%.2x\n", colors,
      //          rgb.rgbGreen, rgb.rgbRed, rgb.rgbBlue);
      pal[colors++] = rgb;
    } else
      k = 15;
  match:
    ctable[64 + index] = k;
  }

  binfo->bmiHeader.biClrImportant = colors;

  colors = 16;  // やっぱ固定じゃなきゃ駄目か？
  uint8_t* image = ((uint8_t*)(binfo + 1)) + (colors - 1) * sizeof(RGBQUAD);
  filehdr->bfSize = image + 640 * 400 / 2 - dest;
  binfo->bmiHeader.biClrUsed = colors;
  filehdr->bfOffBits = image - dest;

  // 色変換
  uint8_t* d = image;
  for (int y = 0; y < 400; y += half ? 2 : 1) {
    uint8_t* s = src + 640 * (399 - y);

    for (int x = 0; x < 320; x++, s += 2)
      *d++ = ctable[s[0]] * 16 + ctable[s[1]];
  }

  delete[] src;
  return d - dest;
}

BOOL WINAPI WinDraw::DDEnumCallback(GUID FAR* guid,
                                    LPSTR desc,
                                    LPSTR name,
                                    LPVOID context,
                                    HMONITOR hm) {
  WinDraw* draw = reinterpret_cast<WinDraw*>(context);
  if (hm == draw->hmonitor) {
    draw->gmonitor = *guid;
    return 0;
  }
  return 1;
}
