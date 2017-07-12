// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//  $Id: memmon.h,v 1.9 2003/05/19 02:33:56 cisc Exp $

#pragma once

#include "common/device.h"
#include "win32/monitors/mvmon.h"
#include "pc88/memview.h"
#include "win32/wincore.h"

namespace m88win {

class MemoryMonitor final : public MemViewMonitor {
 public:
  MemoryMonitor();
  ~MemoryMonitor() final;

  bool Init(WinCore*);

 private:
  // Overrides WinMonitor.
  INT_PTR DlgProc(HWND, UINT, WPARAM, LPARAM) final;
  void UpdateText() final;
  void Start() final;
  void Stop() final;

  INT_PTR EDlgProc(HWND, UINT, WPARAM, LPARAM);
  static INT_PTR CALLBACK EDlgProcGate(HWND, UINT, WPARAM, LPARAM);

  static uint32_t MEMCALL MemRead(void* p, uint32_t a);
  static void MEMCALL MemWrite(void* p, uint32_t a, uint32_t d);

  bool SaveImage();

  void SetWatch(uint32_t, uint32_t);

  void ExecCommand();
  void Search(uint32_t key, int bytes);

  // Overrides MemViewMonotor.
  void SetBank() final;

  WinCore* core;
  IMemoryManager* mm;
  IGetMemoryBank* gmb;
  int mid;
  uint32_t time;

  int prevaddr, prevlines;

  int watchflag;

  HWND hwndstatus;

  int editaddr;

  char line[0x100];
  uint8_t stat[0x10000];
  uint32_t access[0x10000];

  static COLORREF col[0x100];
};
}  // namespace m88win
