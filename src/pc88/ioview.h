// ----------------------------------------------------------------------------
//  M88 - PC-8801 series emulator
//  Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//  IO 出力監視インターフェース
// ----------------------------------------------------------------------------
//  $Id: ioview.h,v 1.1 2001/02/21 11:57:57 cisc Exp $

#pragma once

#include "common/device.h"
#include "pc88/memory.h"
#include "pc88/subsys.h"
#include "pc88/pc88.h"

namespace pc88 {
// ----------------------------------------------------------------------------
//  0   N88 N80 RAM ERAM             SUB
//  60  N88 N80 RAM ERAM E0 E1 E2 E3 SUB
//  80  RAM
//  C0  RAM GV0 GV1 GV2
//  F0  RAM TV
//
class IOViewer final : public Device {
 public:
  enum ConnID {
    kOut,
  };

 public:
  IOViewer();
  ~IOViewer();

  bool Connect(IIOBus* bus);
  bool Disconnect();
  int Read(int c) { return buf[c]; }

  void Reset() {
    for (int i = 0; i < 256; i++)
      buf[i] |= ~0xff;
  }
  void Dim();

  void IOCALL Out(uint32_t = 0, uint32_t = 0);

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }

 private:
  IIOBus* bus;
  uint32_t buf[0x100];

 private:
  static const Descriptor descriptor;
  static const OutFuncPtr outdef[];
};
}  // namespace pc88
