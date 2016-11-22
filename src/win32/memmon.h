// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//  $Id: memmon.h,v 1.9 2003/05/19 02:33:56 cisc Exp $

#if !defined(win32_memmon_h)
#define win32_memmon_h

#include "common/device.h"
#include "win32/mvmon.h"
#include "pc88/memview.h"
#include "win32/wincore.h"

// ---------------------------------------------------------------------------

class PC88;

namespace PC8801 {

class MemoryMonitor : public MemViewMonitor {
 public:
  MemoryMonitor();
  ~MemoryMonitor();

  bool Init(WinCore*);

 private:
  INT_PTR DlgProc(HWND, UINT, WPARAM, LPARAM);
  INT_PTR EDlgProc(HWND, UINT, WPARAM, LPARAM);
  static INT_PTR CALLBACK EDlgProcGate(HWND, UINT, WPARAM, LPARAM);

  static uint MEMCALL MemRead(void* p, uint a);
  static void MEMCALL MemWrite(void* p, uint a, uint d);

  void Start();
  void Stop();

  void UpdateText();
  bool SaveImage();

  void SetWatch(uint, uint);

  void ExecCommand();
  void Search(uint key, int bytes);

  void SetBank();

  WinCore* core;
  IMemoryManager* mm;
  IGetMemoryBank* gmb;
  int mid;
  uint time;

  int prevaddr, prevlines;

  int watchflag;

  HWND hwndstatus;

  int editaddr;

  char line[0x100];
  uint8_t stat[0x10000];
  uint access[0x10000];

  static COLORREF col[0x100];
};
}

#endif  // !defined(win32_memmon_h)
