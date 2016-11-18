// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//  ��ʊ֌W for Windows
// ---------------------------------------------------------------------------
//  $Id: windraw.cpp,v 1.34 2003/04/22 13:16:36 cisc Exp $

#include "win32/headers.h"
#include "common/misc.h"
#include "win32/windraw.h"
#include "win32/drawgdi.h"
#include "win32/drawdds.h"
#include "win32/drawddw.h"
#include "win32/messages.h"
#include "common/error.h"
#include "win32/status.h"
#include "win32/loadmon.h"
#include "win32/winexapi.h"

#define LOGNAME "windraw"
#include "win32/diag.h"
//#define DRAW_THREAD

// ---------------------------------------------------------------------------
// �\�z/����
//
WinDraw::WinDraw() {
  draw = 0;
  idthread = 0;
  hthread = 0;
  hevredraw = 0;
  drawcount = 0;
  guicount = 0;
  shouldterminate = false;
  drawall = false;
  drawing = false;
  refresh = false;
  locked = false;
  active = false;
}

WinDraw::~WinDraw() {
  Cleanup();
}

// ---------------------------------------------------------------------------
//  ������
//
bool WinDraw::Init0(HWND hwindow) {
  hwnd = hwindow;
  draw = 0;
  drawtype = None;

  hthread = 0;
  hevredraw = CreateEvent(NULL, FALSE, FALSE, NULL);

  HDC hdc = GetDC(hwnd);
  haspalette = !!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE);
  ReleaseDC(hwnd, hdc);

#ifdef DRAW_THREAD
  if (!hthread)
    hthread = HANDLE(_beginthreadex(
        NULL, 0, ThreadEntry, reinterpret_cast<void*>(this), 0, &idthread));
  if (!hthread) {
    Error::SetError(Error::ThreadInitFailed);
    return false;
  }
#endif

  palcngbegin = palrgnbegin = 0x100;
  palcngend = palrgnend = -1;

  return true;
}

bool WinDraw::Init(uint w, uint h, uint /*bpp*/) {
  width = w;
  height = h;
  shouldterminate = false;
  active = true;
  return true;
}

// ---------------------------------------------------------------------------
//  ��Еt
//
bool WinDraw::Cleanup() {
  if (hthread) {
    SetThreadPriority(hthread, THREAD_PRIORITY_NORMAL);

    int i = 300;
    do {
      shouldterminate = true;
      SetEvent(hevredraw);
    } while (--i > 0 && WAIT_TIMEOUT == WaitForSingleObject(hthread, 10));

    if (!i)
      TerminateThread(hthread, 0);

    CloseHandle(hthread), hthread = 0;
  }
  if (hevredraw)
    CloseHandle(hevredraw), hevredraw = 0;

  delete draw;
  draw = 0;
  return true;
}

// ---------------------------------------------------------------------------
//  ��ʕ`��p�X���b�h
//
uint WinDraw::ThreadMain() {
  drawing = false;
  while (!shouldterminate) {
    PaintWindow();
    WaitForSingleObject(hevredraw, 1000);
  }
  return 0;
}

uint __stdcall WinDraw::ThreadEntry(LPVOID arg) {
  if (arg)
    return reinterpret_cast<WinDraw*>(arg)->ThreadMain();
  else
    return 0;
}

// ---------------------------------------------------------------------------
//  �X���b�h�̗D�揇�ʂ�������
//
void WinDraw::SetPriorityLow(bool low) {
  if (hthread) {
    SetThreadPriority(
        hthread, low ? THREAD_PRIORITY_BELOW_NORMAL : THREAD_PRIORITY_NORMAL);
  }
}

// ---------------------------------------------------------------------------
//  �p���b�g���f
//
void WinDraw::QueryNewPalette(bool /*bkgnd*/) {
  CriticalSection::Lock lock(csdraw);
  if (draw)
    draw->QueryNewPalette();
}

// ---------------------------------------------------------------------------
//  �y�C���g������
//
void WinDraw::RequestPaint() {
#ifdef DRAW_THREAD
  CriticalSection::Lock lock(csdraw);
  drawall = true;
  drawing = true;
  SetEvent(hevredraw);
#else
  drawall = true;
  drawing = true;
  PaintWindow();
#endif
  //  LOG1("Request at %d\n", GetTickCount());
}

// ---------------------------------------------------------------------------
//  �X�V
//
void WinDraw::DrawScreen(const Region& region) {
  if (!drawing) {
    LOG2("Draw %d to %d\n", region.top, region.bottom);
    drawing = true;
    drawarea.left = Max(0, region.left);
    drawarea.top = Max(0, region.top);
    drawarea.right = Min(width, region.right);
    drawarea.bottom = Min(height, region.bottom + 1);
#ifdef DRAW_THREAD
    SetEvent(hevredraw);
#else
    PaintWindow();
#endif
  }
}

// ---------------------------------------------------------------------------
//  �`�悷��
//
void WinDraw::PaintWindow() {
  LOADBEGIN("WinDraw");
  CriticalSection::Lock lock(csdraw);
  if (drawing && draw && active) {
    RECT rect = drawarea;
    if (palcngbegin <= palcngend) {
      palrgnbegin = Min(palcngbegin, palrgnbegin);
      palrgnend = Max(palcngend, palrgnend);
      draw->SetPalette(palette + palcngbegin, palcngbegin,
                       palcngend - palcngbegin + 1);
      palcngbegin = 0x100;
      palcngend = -1;
    }
    draw->DrawScreen(rect, drawall);
    drawall = false;
    if (rect.left < rect.right && rect.top < rect.bottom) {
      drawcount++;
      LOG4("\t\t\t(%3d,%3d)-(%3d,%3d)\n", rect.left, rect.top, rect.right - 1,
           rect.bottom - 1);
      //          statusdisplay.Show(100, 0, "(%.3d, %.3d)-(%.3d, %.3d)",
      //          rect.left, rect.top, rect.right-1, rect.bottom-1);
    }
    drawing = false;
  }
  LOADEND("WinDraw");
}

// ---------------------------------------------------------------------------
//  �p���b�g���Z�b�g
//
void WinDraw::SetPalette(uint index, uint nents, const Palette* pal) {
  assert(0 <= index && index <= 0xff);
  assert(index + nents <= 0x100);
  assert(pal);

  memcpy(palette + index, pal, nents * sizeof(Palette));
  palcngbegin = Min(palcngbegin, index);
  palcngend = Max(palcngend, index + nents - 1);
}

// ---------------------------------------------------------------------------
//  Lock
//
bool WinDraw::Lock(uint8** pimage, int* pbpl) {
  assert(pimage && pbpl);

  if (!locked) {
    locked = true;
    csdraw.lock();
    if (draw && draw->Lock(pimage, pbpl)) {
      // Lock �Ɏ��s���邱�Ƃ�����H
      assert(**pimage >= 0);
      return true;
    }
    csdraw.unlock();
    locked = false;
  }
  return false;
}

// ---------------------------------------------------------------------------
//  unlock
//
bool WinDraw::Unlock() {
  bool result = false;
  if (locked) {
    if (draw)
      result = draw->Unlock();
    csdraw.unlock();
    locked = false;
    if (refresh == 1)
      refresh = 0;
  }
  return result;
}

// ---------------------------------------------------------------------------
//  ��ʃT�C�Y��ς���
//
void WinDraw::Resize(uint w, uint h) {
  //  statusdisplay.Show(50, 2500, "Resize (%d, %d)", width, height);
  width = w;
  height = h;
  if (draw)
    draw->Resize(width, height);
}

// ---------------------------------------------------------------------------
//  ��ʈʒu��ς���
//
void WinDraw::WindowMoved(int x, int y) {
  if (draw)
    draw->WindowMoved(x, y);
}

// ---------------------------------------------------------------------------
//  ��ʕ`��h���C�o�̕ύX
//
bool WinDraw::ChangeDisplayMode(bool fullscreen, bool force480) {
  DisplayType type = fullscreen ? DDFull : DDWin;

  // ���ݑ�(M88)���������郂�j�^�� GUID ���擾
  memset(&gmonitor, 0, sizeof(gmonitor));

  hmonitor = (*MonitorFromWin)(hwnd, MONITOR_DEFAULTTOPRIMARY);
  (*DDEnumerateEx)(DDEnumCallback, reinterpret_cast<LPVOID>(this),
                   DDENUM_ATTACHEDSECONDARYDEVICES);

  if (type != drawtype) {
    // ���܂ł̃h���C�o��p��
    if (draw)
      draw->SetGUIMode(true);
    {
      CriticalSection::Lock lock(csdraw);
      delete draw;
      draw = 0;
    }

    // �V�����h���C�o�̗p��
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
      // �������Ɏ��s�����ꍇ GDI �h���C�o�ōĒ���
      delete newdraw;

      if (type == DDFull)
        statusdisplay.Show(50, 2500, "��ʐ؂�ւ��Ɏ��s���܂���");
      else
        statusdisplay.Show(120, 3000, "GDI �h���C�o���g�p���܂�");

      newdraw = new WinDrawGDI, type = GDI;
      result = false;

      if (!newdraw || !newdraw->Init(hwnd, width, height, 0))
        newdraw = 0, drawtype = None;
    }

    if (newdraw) {
      newdraw->SetFlipMode(flipmode);
      newdraw->SetGUIMode(false);
    }

    // �V�����h���C�o���g�p�\�ɂ���
    {
      CriticalSection::Lock lock(csdraw);
      guicount = 0;
      draw = newdraw;
    }

    drawall = true, refresh = true;
    palrgnbegin = Min(palcngbegin, palrgnbegin);
    palrgnend = Max(palcngend, palrgnend);
    if (draw)
      draw->Resize(width, height);

    if (type == DDFull)
      ShowCursor(false);
    if (drawtype == DDFull)
      ShowCursor(true);
    drawtype = type;

    refresh = 2;

    return result;
  }
  return false;
}

// ---------------------------------------------------------------------------
//  ���݂̏�Ԃ𓾂�
//
uint WinDraw::GetStatus() {
  if (draw) {
    if (refresh)
      refresh = 1;
    return (!drawing && active ? readytodraw : 0) |
           (refresh ? shouldrefresh : 0) | draw->GetStatus();
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  flip ���[�h�̐ݒ�
//
bool WinDraw::SetFlipMode(bool f) {
  flipmode = f;
  if (draw)
    return draw->SetFlipMode(flipmode);
  return false;
}

// ---------------------------------------------------------------------------
//  flip ����
//
void WinDraw::Flip() {
  if (draw)
    draw->Flip();
}

// ---------------------------------------------------------------------------
//  GUI �g�p�t���O�ݒ�
//
void WinDraw::SetGUIFlag(bool usegui) {
  CriticalSection::Lock lock(csdraw);
  if (usegui) {
    if (!guicount++ && draw) {
      draw->SetGUIMode(true);
      ShowCursor(true);
    }
  } else {
    if (!--guicount && draw) {
      draw->SetGUIMode(false);
      ShowCursor(false);
    }
  }
}

// ---------------------------------------------------------------------------
//  ��ʂ� 640x400x4 �� BMP �ɕϊ�����
//  dest    �ϊ����� BMP �̒u���ꏊ�A�u���邾���̗̈悪�K�v�B
//  ret     �I���ł������ǂ���
//
int WinDraw::CaptureScreen(uint8* dest) {
  const bool half = false;
  if (!draw)
    return false;

  uint8* src = new uint8[640 * 400];
  if (!src)
    return false;

  uint8* s;
  int bpl;
  if (draw->Lock(&s, &bpl)) {
    uint8* d = src;
    for (int y = 0; y < 400; y++) {
      memcpy(d, s, 640);
      d += 640, s += bpl;
    }
    draw->Unlock();
  }

  // �\���̂̏���

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

  // �P�U�F�p���b�g�̍쐬
  RGBQUAD* pal = binfo->bmiColors;
  memset(pal, 0, sizeof(RGBQUAD) * 16);

  uint8 ctable[256];
  memset(ctable, 0, sizeof(ctable));

  int colors = 0;
  for (int index = 0; index < 144; index++) {
    RGBQUAD rgb;
    rgb.rgbBlue = palette[0x40 + index].peBlue;
    rgb.rgbRed = palette[0x40 + index].peRed;
    rgb.rgbGreen = palette[0x40 + index].peGreen;
    //      LOG4("c[%.2x] = G:%.2x R:%.2x B:%.2x\n", index, rgb.rgbGreen,
    //      rgb.rgbRed, rgb.rgbBlue);
    uint32 entry = *((uint32*)&rgb);

    int k;
    for (k = 0; k < colors; k++) {
      if (!((*((uint32*)&pal[k]) ^ entry) & 0xffffff))
        goto match;
    }
    if (colors < 15) {
      //          LOG4("pal[%.2x] = G:%.2x R:%.2x B:%.2x\n", colors,
      //          rgb.rgbGreen, rgb.rgbRed, rgb.rgbBlue);
      pal[colors++] = rgb;
    } else
      k = 15;
  match:
    ctable[64 + index] = k;
  }

  binfo->bmiHeader.biClrImportant = colors;

  colors = 16;  // ����όŒ肶��Ȃ���ʖڂ��H
  uint8* image = ((uint8*)(binfo + 1)) + (colors - 1) * sizeof(RGBQUAD);
  filehdr->bfSize = image + 640 * 400 / 2 - dest;
  binfo->bmiHeader.biClrUsed = colors;
  filehdr->bfOffBits = image - dest;

  // �F�ϊ�
  uint8* d = image;
  for (int y = 0; y < 400; y += half ? 2 : 1) {
    uint8* s = src + 640 * (399 - y);

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
