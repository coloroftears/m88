// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//  $Id: mvmon.h,v 1.3 2003/05/19 02:33:56 cisc Exp $

#pragma once

#include "common/device.h"
#include "win32/monitors/winmon.h"
#include "pc88/memview.h"

// ---------------------------------------------------------------------------

class PC88;

namespace m88win {

class MemViewMonitor : public WinMonitor {
 public:
  MemViewMonitor();
  ~MemViewMonitor() override;

  bool Init(LPCTSTR tmpl, PC88*);

 protected:
  MemoryBus* GetBus() { return bus; }

  // Overrides WinMonitor.
  INT_PTR DlgProc(HWND, UINT, WPARAM, LPARAM) override;

  void StatClear();
  uint32_t StatExec(uint32_t a);
  virtual void SetBank();

  using MemoryViewer = pc88::MemoryViewer;

  MemoryViewer mv;

  MemoryViewer::Type GetA0() { return a0; }
  MemoryViewer::Type GetA6() { return a6; }
  MemoryViewer::Type GetAf() { return af; }

 private:
  MemoryBus* bus;
  MemoryViewer::Type a0;
  MemoryViewer::Type a6;
  MemoryViewer::Type af;
};

}  // namespace m88win
