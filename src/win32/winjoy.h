// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: WinJoy.h,v 1.5 2003/04/22 13:16:35 cisc Exp $

#pragma once

#include "common/device.h"
#include "interface/ifui.h"

class WinPadIF final : public IPadInput {
 public:
  WinPadIF();
  ~WinPadIF();

  bool Init();

  // Overrides IPadInput
  void IFCALL GetState(PadState*) final;

 private:
  bool enabled;
};
