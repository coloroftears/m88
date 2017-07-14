// ---------------------------------------------------------------------------
//  M88 - PC-8801 Series Emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  Implementation of USART(uPD8251AF)
// ---------------------------------------------------------------------------
//  $Id: sio.h,v 1.5 2000/06/26 14:05:30 cisc Exp $

#pragma once

#include "common/device.h"

class Scheduler;

namespace pc88core {

class SIO final : public Device {
 public:
  enum {
    reset = 0,
    setcontrol,
    setdata,
    acceptdata,
    getstatus = 0,
    getdata,
  };

 public:
  explicit SIO(const ID& id);
  ~SIO();
  bool Init(IOBus* bus, uint32_t prxrdy, uint32_t prequest);

  void IOCALL Reset(uint32_t = 0, uint32_t = 0);
  void IOCALL SetControl(uint32_t, uint32_t d);
  void IOCALL SetData(uint32_t, uint32_t d);
  uint32_t IOCALL GetStatus(uint32_t = 0);
  uint32_t IOCALL GetData(uint32_t = 0);

  void IOCALL AcceptData(uint32_t, uint32_t);

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final;
  bool IFCALL SaveStatus(uint8_t* s) final;
  bool IFCALL LoadStatus(const uint8_t* s) final;

 private:
  enum Mode { clear = 0, async, sync1, sync2, sync };
  enum Parity { none = 'N', odd = 'O', even = 'E' };

  IOBus* bus;
  uint32_t prxrdy;
  uint32_t prequest;

  uint32_t baseclock;
  uint32_t clock;
  uint32_t datalen;
  uint32_t stop;
  uint32_t status;
  uint32_t data;
  Mode mode_;
  Parity parity;
  bool rxen;
  bool txen;

 private:
  enum {
    TXRDY = 0x01,
    RXRDY = 0x02,
    TXE = 0x04,
    PE = 0x08,
    OE = 0x10,
    FE = 0x20,
    SYNDET = 0x40,
    DSR = 0x80,

    SSREV = 1,
  };
  struct Status {
    uint8_t rev;
    bool rxen;
    bool txen;

    uint32_t baseclock;
    uint32_t clock;
    uint32_t datalen;
    uint32_t stop;
    uint32_t status;
    uint32_t data;
    Mode mode_;
    Parity parity;
  };

 private:
  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};
}  // namespace pc88core
