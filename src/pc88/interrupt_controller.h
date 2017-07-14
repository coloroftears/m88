// ---------------------------------------------------------------------------
//  PC-8801 emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  割り込みコントローラ周り(μPD8214)
// ---------------------------------------------------------------------------
//  $Id: intc.h,v 1.7 1999/10/10 01:47:06 cisc Exp $

#pragma once

#include "common/device.h"

namespace pc88core {

// ---------------------------------------------------------------------------

class InterruptController final : public Device {
 public:
  enum { kReset, kRequest, kSetMask, kSetReg, kIntAck = 0 };

 public:
  explicit InterruptController(const ID& id);
  ~InterruptController();
  bool Init(IOBus* bus, uint32_t irqport, uint32_t ipbase);

  void IOCALL Reset(uint32_t = 0, uint32_t = 0);
  void IOCALL Request(uint32_t port, uint32_t en);
  void IOCALL SetMask(uint32_t, uint32_t data);
  void IOCALL SetRegister(uint32_t, uint32_t data);
  uint32_t IOCALL IntAck(uint32_t);

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final;
  bool IFCALL SaveStatus(uint8_t* status) final;
  bool IFCALL LoadStatus(const uint8_t* status) final;

 private:
  struct Status {
    uint32_t mask;
    uint32_t mask2;
    uint32_t irq;
  };
  void IRQ(bool);

  IOBus* bus_;
  Status status_;
  uint32_t irq_port_;
  uint32_t i_portbase_;

  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};
}  // namespace pc88core
