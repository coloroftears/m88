// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//  $Id: loadmon.h,v 1.1 2001/02/21 11:58:54 cisc Exp $

#pragma once

#include <map>
#include <string>

#include "common/critical_section.h"
#include "common/device.h"
#include "win32/monitors/winmon.h"

//#define ENABLE_LOADMONITOR

// ---------------------------------------------------------------------------

namespace m88win {

#ifdef ENABLE_LOADMONITOR

class LoadMonitor final : public WinMonitor {
 public:
  enum {
    presis = 10,
  };

  LoadMonitor();
  ~LoadMonitor() final;

  bool Init();

  void ProcBegin(const char* name);
  void ProcEnd(const char* name);
  static LoadMonitor* GetInstance() { return instance; }

 private:
  struct State {
    int total[presis];  // 累計
    DWORD timeentered;  // 開始時刻
  };

  using States = std::map<std::string, State>;

  // Overrides WinMonitor.
  BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);
  void DrawMain(HDC, bool);
  void UpdateText();

  States states;
  int base;
  int tidx;
  int tprv;

  CriticalSection cs;
  static LoadMonitor* instance;
};

inline void LOADBEGIN(const char* key) {
  LoadMonitor* lm = LoadMonitor::GetInstance();
  if (lm)
    lm->ProcBegin(key);
}

inline void LOADEND(const char* key) {
  LoadMonitor* lm = LoadMonitor::GetInstance();
  if (lm)
    lm->ProcEnd(key);
}

#else  // ENABLE_LOADMONITOR

class LoadMonitor {
 public:
  LoadMonitor() {}
  ~LoadMonitor() {}

  bool Show(HINSTANCE, HWND, bool) { return false; }
  bool IsOpen() { return false; }
  bool Init() { return false; }
};

#define LOADBEGIN(a)
#define LOADEND(a)

#endif  // ENABLE_LOADMONITOR

}  // namespace m88win