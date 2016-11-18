// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//  $Id: iomon.h,v 1.1 2001/02/21 11:58:54 cisc Exp $

#if !defined(win32_iomon_h)
#define win32_iomon_h

#include "common/device.h"
#include "win32/mvmon.h"
#include "pc88/ioview.h"
#include "win32/wincore.h"

// ---------------------------------------------------------------------------

class PC88;

namespace PC8801 {

class IOMonitor : public WinMonitor {
 public:
  IOMonitor();
  ~IOMonitor();

  bool Init(WinCore*);

 private:
  void Start();
  void Stop();
  void UpdateText();
  BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);
  IOViewer iov;
  WinCore* pc;
  bool bank;

  static COLORREF ctbl[0x100];
};
}

#endif  // !defined(win32_iomon_h)
