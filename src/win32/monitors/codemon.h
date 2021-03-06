// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//  $Id: codemon.h,v 1.7 2003/05/19 02:33:56 cisc Exp $

#pragma once

#include "win32/monitors/mvmon.h"
#include "common/device.h"
#include "pc88/memview.h"
#include "devices/z80diag.h"

#include <stdio.h>

namespace pc88core {
class PC88;
}  // namespace pc88core

namespace m88win {

class CodeMonitor final : public MemViewMonitor {
 public:
  CodeMonitor();
  ~CodeMonitor();

  bool Init(pc88core::PC88*);

 private:
  // Overrides WinMonitor
  INT_PTR DlgProc(HWND, UINT, WPARAM, LPARAM) final;
  void UpdateText() final;
  int VerticalScroll(int msg) final;

  bool Dump(FILE* fp, int from, int to);
  bool DumpImage();

  Z80Diag diag;
};
}  // namespace m88win
