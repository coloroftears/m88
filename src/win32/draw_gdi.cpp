// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  GDI による画面描画 (HiColor 以上)
// ---------------------------------------------------------------------------
//  $Id: DrawGDI.cpp,v 1.13 2003/04/22 13:16:35 cisc Exp $

//  bug:パレット～(T-T

#include "win32/draw_gdi.h"

// ---------------------------------------------------------------------------
//  構築/消滅
//
WinDrawGDI::WinDrawGDI() {}

WinDrawGDI::~WinDrawGDI() {
  Cleanup();
}

// ---------------------------------------------------------------------------
//  初期化処理
//
bool WinDrawGDI::Init(HWND hwindow, uint32_t w, uint32_t h, GUID*) {
  hwnd = hwindow;
  return Resize(w, h);
}

// ---------------------------------------------------------------------------
//  画面有効範囲を変更
//
bool WinDrawGDI::Resize(uint32_t w, uint32_t h) {
  width_ = w;
  height_ = h;

  Cleanup();
  if (!MakeBitmap())
    return false;

  memset(image_, 0x40, width_ * height_);
  status |= Draw::shouldrefresh;
  return true;
}

// ---------------------------------------------------------------------------
//  後片付け
//
bool WinDrawGDI::Cleanup() {
  if (hbitmap) {
    DeleteObject(hbitmap);
    hbitmap = 0;
  }
  return true;
}

// ---------------------------------------------------------------------------
//  BITMAP 作成
//
bool WinDrawGDI::MakeBitmap() {
  binfo.header.biSize = sizeof(BITMAPINFOHEADER);
  binfo.header.biWidth = width_;
  binfo.header.biHeight = -(int)height_;
  binfo.header.biPlanes = 1;
  binfo.header.biBitCount = 8;
  binfo.header.biCompression = BI_RGB;
  binfo.header.biSizeImage = 0;
  binfo.header.biXPelsPerMeter = 0;
  binfo.header.biYPelsPerMeter = 0;
  binfo.header.biClrUsed = 256;
  binfo.header.biClrImportant = 0;

  // パレットない場合

  HDC hdc = GetDC(hwnd);
  memset(binfo.colors, 0, sizeof(RGBQUAD) * 256);

  if (hbitmap)
    DeleteObject(hbitmap);
  uint8_t* bitmapimage = nullptr;
  hbitmap = CreateDIBSection(hdc, (BITMAPINFO*)&binfo, DIB_RGB_COLORS,
                             (void**)(&bitmapimage), nullptr, 0);

  ReleaseDC(hwnd, hdc);

  if (!hbitmap)
    return false;

  bpl_ = width_;
  image_ = bitmapimage;
  return true;
}

// ---------------------------------------------------------------------------
//  パレット設定
//  index 番目のパレットに pe をセット
//
void WinDrawGDI::SetPalette(PALETTEENTRY* pe, int index, int nentries) {
  for (; nentries > 0; nentries--) {
    binfo.colors[index].rgbRed = pe->peRed;
    binfo.colors[index].rgbBlue = pe->peBlue;
    binfo.colors[index].rgbGreen = pe->peGreen;
    index++, pe++;
  }
  updatepal_ = true;
}

// ---------------------------------------------------------------------------
//  描画
//
void WinDrawGDI::DrawScreen(const RECT& _rect, bool refresh) {
  RECT rect = _rect;
  bool valid = rect.left < rect.right && rect.top < rect.bottom;

  if (refresh || updatepal_)
    SetRect(&rect, 0, 0, width_, height_), valid = true;

  if (valid) {
    HDC hdc = GetDC(hwnd);
    HDC hmemdc = CreateCompatibleDC(hdc);
    HBITMAP oldbitmap = (HBITMAP)SelectObject(hmemdc, hbitmap);
    if (updatepal_) {
      updatepal_ = false;
      SetDIBColorTable(hmemdc, 0, 0x100, binfo.colors);
    }

    BitBlt(hdc, rect.left, rect.top, rect.right - rect.left,
           rect.bottom - rect.top, hmemdc, rect.left, rect.top, SRCCOPY);

    SelectObject(hmemdc, oldbitmap);
    DeleteDC(hmemdc);
    ReleaseDC(hwnd, hdc);
  }
}

// ---------------------------------------------------------------------------
//  画面イメージの使用要求
//
bool WinDrawGDI::Lock(uint8_t** pimage, int* pbpl) {
  *pimage = image_;
  *pbpl = bpl_;
  return !!image_;
}

// ---------------------------------------------------------------------------
//  画面イメージの使用終了
//
bool WinDrawGDI::Unlock() {
  status &= ~Draw::shouldrefresh;
  return true;
}
