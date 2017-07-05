// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//  $Id: iomon.h,v 1.1 2001/02/21 11:58:54 cisc Exp $

#pragma once

#include "common/device.h"
#include "win32/monitors/mvmon.h"
#include "pc88/ioview.h"
#include "win32/wincore.h"

// ---------------------------------------------------------------------------

class PC88;

namespace m88win {

class IOMonitor final : public WinMonitor {
 public:
  IOMonitor();
  ~IOMonitor() final;

  bool Init(WinCore*);

 private:
  // Overrides WinMonitor.
  void UpdateText() final;
  INT_PTR DlgProc(HWND, UINT, WPARAM, LPARAM) final;
  void Start() final;
  void Stop() final;

  pc88::IOViewer iov;
  WinCore* pc;
  bool bank;

  static COLORREF ctbl[0x100];
};
}  // namespace m88win
