// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: beep.h,v 1.2 1999/10/10 01:47:04 cisc Exp $

#pragma once

#include "common/device.h"

class PC88;

namespace pc88core {
class Config;

class Beep final : public Device, public ISoundSource {
 public:
  enum IDFunc { kOut40 };

 public:
  explicit Beep(const ID& id);
  ~Beep();

  bool Init();
  void Cleanup();
  void EnableSING(bool s) {
    p40mask_ = s ? 0xa0 : 0x20;
    port40_ &= p40mask_;
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

  ISoundControl* soundcontrol_ = nullptr;
  int bslice_ = 0;
  int pslice_ = 0;
  int bcount_ = 0;
  int bperiod_ = 0;

  uint32_t port40_ = 0;
  uint32_t p40mask_ = 0xa0;

  static const Descriptor descriptor;
  static const OutFuncPtr outdef[];
};
}  // namespace pc88core
