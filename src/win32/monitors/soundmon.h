// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: soundmon.h,v 1.11 2003/06/12 13:14:37 cisc Exp $

#pragma once

#include "common/device.h"
#include "common/sound_source.h"
#include "win32/monitors/winmon.h"

// ---------------------------------------------------------------------------

namespace PC8801 {
class OPNIF;
}

class OPNMonitor final : public WinMonitor, public ISoundSource {
 public:
  OPNMonitor();
  ~OPNMonitor() final;

  bool Init(PC8801::OPNIF* opn, ISoundControl* soundcontrol);

  // Overrides ISoundSource.
  bool IFCALL SetRate(uint32_t rate) final { return true; }
  void IFCALL Mix(int32_t* s, int length) final;

 private:
  enum {
    bufsize = 2048,
  };

  // Overrides WinMonitor.
  void DrawMain(HDC, bool) final;
  INT_PTR DlgProc(HWND, UINT, WPARAM, LPARAM) final;
  void UpdateText() final;

  // Overrides ISoundSource.
  bool IFCALL Connect(ISoundControl* sc);

  // Overrides WinMonitor.
  void Start() final;
  void Stop() final;

  PC8801::OPNIF* opn;
  const uint8_t* regs;

  ISoundControl* soundcontrol;

  uint32_t mask;
  uint32_t read;
  uint32_t write;
  int dim;
  int dimvector;
  int width;
  int buf[2][bufsize];
};
