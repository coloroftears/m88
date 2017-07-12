// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 2000.
// ---------------------------------------------------------------------------
//  $Id: basmon.h,v 1.1 2000/06/26 14:05:44 cisc Exp $

#pragma once

#include <windows.h>

#include "win32/monitors/winmon.h"
#include "common/device.h"
#include "pc88/memview.h"

namespace pc88core {
class PC88;
}  // namespace pc88core

namespace m88win {

class BasicMonitor final : public WinMonitor {
 public:
  BasicMonitor();
  ~BasicMonitor() final;

  bool Init(pc88core::PC88*);

 private:
  void Decode(bool always);

  // Overrides WinMonitor.
  INT_PTR DlgProc(HWND, UINT, WPARAM, LPARAM) final;
  void UpdateText() final;

  char basictext[0x10000];
  int line[0x4000];
  int nlines;

  pc88core::MemoryViewer mv;
  MemoryBus* bus;

  uint32_t Read8(uint32_t adr);
  uint32_t Read16(uint32_t adr);
  uint32_t Read32(uint32_t adr);

  uint32_t prvs;

  static const char* rsvdword[];
};
}  // namespace m88win
