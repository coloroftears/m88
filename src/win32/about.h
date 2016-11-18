// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  About Dialog Box for M88
// ---------------------------------------------------------------------------
//  $Id: about.h,v 1.5 1999/12/28 11:14:05 cisc Exp $

#if !defined(WIN_ABOUT_H)
#define WIN_ABOUT_H

#include "win32/types.h"

// ---------------------------------------------------------------------------

class M88About {
 public:
  M88About();
  ~M88About() {}

  void Show(HINSTANCE, HWND);

 private:
  INT_PTR DlgProc(HWND, UINT, WPARAM, LPARAM);
  static INT_PTR DlgProcGate(HWND, UINT, WPARAM, LPARAM);

  static const char abouttext[];
  uint32 crc;
};

#endif  // !defined(WIN_ABOUT_H)
