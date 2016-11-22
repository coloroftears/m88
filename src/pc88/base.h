// ---------------------------------------------------------------------------
//  PC-8801 emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: base.h,v 1.10 2000/06/26 14:05:30 cisc Exp $

#ifndef pc88_base_h
#define pc88_base_h

#include "common/schedule.h"
#include "common/device.h"
#include "devices/z80c.h"

class PC88;
class TapeManager;

namespace PC8801 {

class Config;

class Base : public Device {
 public:
  enum IDOut { reset = 0, vrtc };
  enum IDIn { in30 = 0, in31, in40, in6e };

 public:
  Base(const ID& id);
  ~Base();

  bool Init(PC88* pc88);
  const Descriptor* IFCALL GetDesc() const { return &descriptor; }

  void SetSwitch(const Config* cfg);
  uint32_t GetBasicMode() { return bmode; }
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
}

#endif  // pc88_base_h
