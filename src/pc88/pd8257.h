// ---------------------------------------------------------------------------
//  M88 - PC-88 Emulator.
//  Copyright (C) cisc 1998.
// ---------------------------------------------------------------------------
//  DMAC (uPD8257) のエミュレーション
// ---------------------------------------------------------------------------
//  $Id: pd8257.h,v 1.10 1999/10/10 01:47:10 cisc Exp $

#pragma once

#include "common/device.h"
#include "interface/ifpc88.h"

// ---------------------------------------------------------------------------

namespace pc88core {

class PD8257 final : public Device, public IDMAAccess {
 public:
  enum IDOut { kReset, kSetAddr, kSetCount, kSetMode };
  enum IDIn { kGetAddr, kGetCount, kGetStat };

 public:
  explicit PD8257(const ID&);
  ~PD8257();

  bool ConnectRd(uint8_t* mem, uint32_t addr, uint32_t length);
  bool ConnectWr(uint8_t* mem, uint32_t addr, uint32_t length);

  void IOCALL Reset(uint32_t = 0, uint32_t = 0);
  void IOCALL SetAddr(uint32_t port, uint32_t data);
  void IOCALL SetCount(uint32_t port, uint32_t data);
  void IOCALL SetMode(uint32_t, uint32_t data);
  uint32_t IOCALL GetAddr(uint32_t port);
  uint32_t IOCALL GetCount(uint32_t port);
  uint32_t IOCALL GetStatus(uint32_t);

  // Overrides IDMAAccess.
  uint32_t IFCALL RequestRead(uint32_t bank,
                              uint8_t* data,
                              uint32_t nbytes) final;
  uint32_t IFCALL RequestWrite(uint32_t bank,
                               uint8_t* data,
                               uint32_t nbytes) final;

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final;
  bool IFCALL SaveStatus(uint8_t* status) final;
  bool IFCALL LoadStatus(const uint8_t* status) final;

 private:
  enum {
    ssrev = 1,
  };
  struct Status {
    uint8_t rev;
    bool autoinit;
    bool ff;
    uint8_t status;
    uint8_t enabled;
    uint32_t addr[4];
    int count[4];
    uint32_t ptr[4];
    uint8_t mode[4];
  };

  Status stat;

  uint8_t* mread;
  uint32_t mrbegin, mrend;
  uint8_t* mwrite;
  uint32_t mwbegin, mwend;

  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};
}  // namespace pc88core
