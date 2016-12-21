// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: beep.h,v 1.2 1999/10/10 01:47:04 cisc Exp $

#pragma once

#include "common/device.h"

// ---------------------------------------------------------------------------

class PC88;

namespace PC8801 {
class Sound;
class Config;
class OPNInterface;

// ---------------------------------------------------------------------------
//
//
class Beep final : public Device, public ISoundSource {
 public:
  enum IDFunc {
    kOut40
  };

 public:
  explicit Beep(const ID& id);
  ~Beep();

  bool Init();
  void Cleanup();
  void EnableSING(bool s) {
    p40mask = s ? 0xa0 : 0x20;
    port40 &= p40mask;
  }

  // Overrides ISoundSource.
  bool IFCALL Connect(ISoundControl* sc) final;
  bool IFCALL SetRate(uint32_t rate) final;
  void IFCALL Mix(int32_t*, int) final;

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final;
  bool IFCALL SaveStatus(uint8_t* status) final;
  bool IFCALL LoadStatus(const uint8_t* status) final;

  void IOCALL Out40(uint32_t, uint32_t data);

 private:
  enum {
    ssrev = 1,
  };
  struct Status {
    uint8_t rev;
    uint8_t port40;
    uint32_t prevtime;
  };

  ISoundControl* soundcontrol;
  int bslice;
  int pslice;
  int bcount;
  int bperiod;

  uint32_t port40;
  uint32_t p40mask;

  static const Descriptor descriptor;
  static const OutFuncPtr outdef[];
};
}  // namespace PC8801
