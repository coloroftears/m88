// ---------------------------------------------------------------------------
//  PC-8801 emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: base.h,v 1.10 2000/06/26 14:05:30 cisc Exp $

#pragma once

#include "common/device.h"
#include "devices/z80c.h"

namespace pc88core {

class Config;
class PC88;

class Base final : public Device {
 public:
  enum IDOut { kReset, kVRTC };
  enum IDIn { kIn30, kIn31, kIn40, kIn6e };

  explicit Base(const ID& id);
  ~Base() final;

  bool Init(PC88* pc88);
  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }

  void SetSwitch(const Config* cfg);
  uint32_t GetBasicMode() const { return bmode_; }
  void IOCALL Reset(uint32_t = 0, uint32_t = 0);
  void SetFDBoot(bool autoboot) { autoboot_ = autoboot; }

  void IOCALL RTC(uint32_t = 0);
  void IOCALL VRTC(uint32_t, uint32_t en);

  uint32_t IOCALL In30(uint32_t);
  uint32_t IOCALL In31(uint32_t);
  uint32_t IOCALL In40(uint32_t);
  uint32_t IOCALL In6e(uint32_t);

 private:
  PC88* pc_ = nullptr;

  int dipsw_ = 0;
  int flags_ = 0;
  int clock_ = 0;
  int bmode_ = 0;

  uint8_t port40_ = 0;
  uint8_t sw30_ = 0;
  uint8_t sw31_ = 0;
  uint8_t sw6e_ = 0;

  bool autoboot_ = true;
  bool fv15k_ = false;

  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};
}  // namespace pc88core
