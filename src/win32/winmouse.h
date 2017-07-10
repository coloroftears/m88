// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: winmouse.h,v 1.5 2002/04/07 05:40:11 cisc Exp $

#pragma once

#include "common/device.h"
#include "interface/ifui.h"

namespace m88win {

class WinUI;

class WinMouseUI final : public IMouseUI {
 public:
  WinMouseUI();
  ~WinMouseUI();

  bool Init(WinUI* ui);

  // Overrides IUnk.
  int32_t IFCALL QueryInterface(REFIID, void**) final;
  uint32_t IFCALL AddRef() final;
  uint32_t IFCALL Release() final;

  // Overrides IMouseUI.
  bool IFCALL Enable(bool en) final;
  bool IFCALL GetMovement(POINT*) final;
  uint32_t IFCALL GetButton() final;

 private:
  POINT GetWindowCenter();

  WinUI* ui;

  POINT move;
  int32_t activetime;
  bool enable;
  int orgmouseparams[3];

  uint32_t refcount;
};
}  // namespace m88win