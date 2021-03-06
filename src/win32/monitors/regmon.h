// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: regmon.h,v 1.1 2000/11/02 12:43:51 cisc Exp $

#pragma once

#include "common/device.h"
#include "win32/monitors/winmon.h"
#include "pc88/pc88.h"

namespace m88win {

class Z80RegMonitor final : public WinMonitor {
 public:
  Z80RegMonitor();
  ~Z80RegMonitor();

  bool Init(pc88core::PC88* pc);

 private:
  // Overrides WinMonitor.
  void UpdateText();
  INT_PTR DlgProc(HWND, UINT, WPARAM, LPARAM);
  void DrawMain(HDC, bool);

  pc88core::PC88* pc;
};

}  // namespace m88win