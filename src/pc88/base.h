// ---------------------------------------------------------------------------
//  PC-8801 emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: base.h,v 1.10 2000/06/26 14:05:30 cisc Exp $

#pragma once

#include "common/device.h"
#include "common/scheduler.h"
#include "devices/z80c.h"


namespace pc88core {

class Config;
class PC88;
class TapeManager;

class Base final : public Device {
 public:
  enum IDOut { kReset, kVRTC };
  enum IDIn { kIn30, kIn31, kIn40, kIn6e };

 public:
  explicit Base(const ID& id);
  ~Base() final;

  bool Init(PC88* pc88);
  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }

  void SetSwitch(const Config* cfg);
  uint32_t GetBasicMode() const { return bmode; }
  void IOCALL Reset(uint32_t = 0, uint32_t = 0);
  void SetFDBoot(bool autoboot_) { autoboot = autoboot_; }

  void IOCALL RTC(uint32_t = 0);
  void IOCALL VRTC(uint32_t, uint32_t en);

  uint32_t IOCALL In30(uint32_t);
  uint32_t IOCALL In31(uint32_t);
  uint32_t IOCALL In40(uint32_t);
  uint32_t IOCALL In6e(uint32_t);

 private:
  PC88* pc;

  int dipsw;
  int flags;
  int clock;
  int bmode;

  uint8_t port40;
  uint8_t sw30, sw31, sw6e;
  bool autoboot;
  bool fv15k;

  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};
}  // namespace pc88core
