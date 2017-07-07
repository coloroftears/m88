// ---------------------------------------------------------------------------
//  PC-8801 emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  割り込みコントローラ周り(μPD8214)
// ---------------------------------------------------------------------------
//  $Id: intc.cpp,v 1.15 2000/06/22 16:22:18 cisc Exp $

#include "pc88/interrupt_controller.h"

#include <algorithm>

//#define LOGNAME "intc"
#include "common/diag.h"

namespace pc88 {

// ---------------------------------------------------------------------------
//  構築破壊
//
InterruptController::InterruptController(const ID& id) : Device(id) {}

InterruptController::~InterruptController() {}

// ---------------------------------------------------------------------------
//  Init
//
bool InterruptController::Init(IOBus* b, uint32_t ip, uint32_t ipbase) {
  bus = b;
  irqport = ip;
  iportbase = ipbase;
  Reset();
  return true;
}

// ---------------------------------------------------------------------------
//  割り込み状況の更新
//
inline void InterruptController::IRQ(bool flag) {
  bus->Out(irqport, flag);
  Log("irq(%d)\n", flag);
}

// ---------------------------------------------------------------------------
//  Reset
//
void IOCALL InterruptController::Reset(uint32_t, uint32_t) {
  stat.irq = stat.mask = stat.mask2 = 0;
  IRQ(false);
}

// ---------------------------------------------------------------------------
//  割り込み要請
//
void IOCALL InterruptController::Request(uint32_t port, uint32_t en) {
  uint32_t bit = 1 << (port - iportbase);
  if (en) {
    bit &= stat.mask2;
    // request
    Log("INT%d REQ - %s :", port - iportbase,
        bit ? (bit & stat.mask ? "accept" : "denied") : "discarded");
    if (!(stat.irq & bit)) {
      stat.irq |= bit;
      IRQ((stat.irq & stat.mask & stat.mask2) != 0);
    } else
      Log("\n");
  } else {
    // cancel
    if (stat.irq & bit) {
      stat.irq &= ~bit;
      IRQ((stat.irq & stat.mask & stat.mask2) != 0);
    }
  }
}

// ---------------------------------------------------------------------------
//  CPU が割り込みを受け取った
//
uint32_t IOCALL InterruptController::IntAck(uint32_t) {
  uint32_t ai = stat.irq & stat.mask & stat.mask2;
  for (int i = 0; i < 8; i++, ai >>= 1) {
    if (ai & 1) {
      stat.irq &= ~(1 << i);
      stat.mask = 0;
      Log("INT%d ACK  : ", i);
      IRQ(false);

      return i * 2;
    }
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  マスク設定(porte6)
//
void IOCALL InterruptController::SetMask(uint32_t, uint32_t data) {
  static const int8_t table[8] = {~7, ~3, ~5, ~1, ~6, ~2, ~4, ~0};
  stat.mask2 = table[data & 7];
  stat.irq &= stat.mask2;
  Log("p[e6] = %.2x (%.2x) : ", data, stat.mask2);
  IRQ((stat.irq & stat.mask & stat.mask2) != 0);
}

// ---------------------------------------------------------------------------
//  レジスタ設定(porte4)
//
void IOCALL InterruptController::SetRegister(uint32_t, uint32_t data) {
  stat.mask = ~(-1 << std::min(8U, data));
  //  mode = (data & 7) != 0;
  Log("p[e4] = %.2x  : ", data);
  IRQ((stat.irq & stat.mask & stat.mask2) != 0);
}

// ---------------------------------------------------------------------------
//  状態保存
//
uint32_t IFCALL InterruptController::GetStatusSize() {
  return sizeof(Status);
}

bool IFCALL InterruptController::SaveStatus(uint8_t* s) {
  *(Status*)s = stat;
  return true;
}

bool IFCALL InterruptController::LoadStatus(const uint8_t* s) {
  stat = *(const Status*)s;
  return true;
}

// ---------------------------------------------------------------------------
//  device description
//
const Device::Descriptor InterruptController::descriptor = {
    InterruptController::indef, InterruptController::outdef};

const Device::OutFuncPtr InterruptController::outdef[] = {
    static_cast<Device::OutFuncPtr>(&InterruptController::Reset),
    static_cast<Device::OutFuncPtr>(&InterruptController::Request),
    static_cast<Device::OutFuncPtr>(&InterruptController::SetMask),
    static_cast<Device::OutFuncPtr>(&InterruptController::SetRegister),
};

const Device::InFuncPtr InterruptController::indef[] = {
    static_cast<Device::InFuncPtr>(&InterruptController::IntAck),
};
}  // namespace pc88