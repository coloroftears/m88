// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  カレンダ時計(μPD1990) のエミュレーション
// ---------------------------------------------------------------------------
//  $Id: calender.h,v 1.3 1999/10/10 01:47:04 cisc Exp $

#pragma once

#include <time.h>

#include "common/device.h"

namespace pc88core {

class Calendar final : public Device {
 public:
  enum {
    kReset,
    kOut10,
    kOut40,
    kIn40 = 0,
  };

 public:
  explicit Calendar(const ID& id);
  ~Calendar();
  bool Init() { return true; }

  // Overrides Device
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final;
  bool IFCALL SaveStatus(uint8_t* status) final;
  bool IFCALL LoadStatus(const uint8_t* status) final;

  void IOCALL Out10(uint32_t, uint32_t data);
  void IOCALL Out40(uint32_t, uint32_t data);
  uint32_t IOCALL In40(uint32_t);
  void IOCALL Reset(uint32_t = 0, uint32_t = 0);

 private:
  enum { ssrev = 1 };
  struct Status {
    uint8_t rev;
    bool dataoutmode;
    bool hold;
    uint8_t datain;
    uint8_t strobe;
    uint8_t cmd, scmd, pcmd;
    uint8_t reg[6];
    time_t t;
  };

  void ShiftData();
  void Command();

  void SetTime();
  void GetTime();

  time_t diff_;

  bool dataoutmode_;
  bool hold_;
  uint8_t datain_;
  uint8_t strobe_;
  uint8_t cmd_;
  uint8_t scmd_;
  uint8_t pcmd_;
  uint8_t reg[6];

  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};
}  // namespace pc88core
