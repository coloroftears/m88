// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: mouse.h,v 1.1 2002/04/07 05:40:10 cisc Exp $

#pragma once

#include "common/device.h"
#include "interface/ifui.h"

class PC88;

namespace PC8801 {

class Config;

class Mouse final : public Device {
 public:
  enum {
    kStrobe,
    kVSync,
    kGetMove = 0,
    kGetButton,
  };

 public:
  explicit Mouse(const ID& id);
  ~Mouse();

  bool Init(PC88* pc);
  bool Connect(IUnk* ui);

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }

  uint32_t IOCALL GetMove(uint32_t);
  uint32_t IOCALL GetButton(uint32_t);
  void IOCALL Strobe(uint32_t, uint32_t data);
  void IOCALL VSync(uint32_t, uint32_t);

  void ApplyConfig(const Config* config);

 private:
  PC88* pc;
  POINT move;
  uint8_t port40;
  bool joymode;
  int phase;
  SchedTime triggertime;
  int sensibility;
  int data;

  IMouseUI* ui;

 private:
  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};
}  // namespace PC8801
