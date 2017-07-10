// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: joypad.h,v 1.3 2003/05/19 01:10:31 cisc Exp $

#pragma once

#include "common/device.h"
#include "interface/ifui.h"

namespace pc88core {

class JoyPad final : public Device {
 public:
  enum {
    kVSync,
    kGetDir = 0,
    kGetButton,
  };
  enum ButtonMode { NORMAL, SWAPPED, DISABLED };

 public:
  JoyPad();
  ~JoyPad();

  bool Connect(IPadInput* ui);

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }

  void IOCALL Reset() {}
  uint32_t IOCALL GetDirection(uint32_t port);
  uint32_t IOCALL GetButton(uint32_t port);
  void IOCALL VSync(uint32_t = 0, uint32_t = 0);
  void SetButtonMode(ButtonMode mode);

 private:
  void Update();

  IPadInput* ui;
  bool paravalid;
  uint8_t data[2];

  uint8_t button1;
  uint8_t button2;
  uint32_t directionmask;

 private:
  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};
}  // namespace pc88core
