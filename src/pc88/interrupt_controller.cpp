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

namespace pc88core {

// ---------------------------------------------------------------------------
//  構築破壊
//
InterruptController::InterruptController(const ID& id) : Device(id) {}

InterruptController::~InterruptController() {}

// ---------------------------------------------------------------------------
//  Init
//
bool InterruptController::Init(IOBus* b, uint32_t ip, uint32_t ipbase) {
  bus_ = b;
  irq_port_ = ip;
  i_portbase_ = ipbase;
  Reset();
  return true;
}

// ---------------------------------------------------------------------------
//  割り込み状況の更新
//
inline void InterruptController::IRQ(bool flag) {
  bus_->Out(irq_port_, flag);
  Log("irq(%d)\n", flag);
}

// ---------------------------------------------------------------------------
//  Reset
//
void IOCALL InterruptController::Reset(uint32_t, uint32_t) {
  status_.irq = status_.mask = status_.mask2 = 0;
  IRQ(false);
}

// ---------------------------------------------------------------------------
//  割り込み要請
//
void IOCALL InterruptController::Request(uint32_t port, uint32_t en) {
  uint32_t bit = 1 << (port - i_portbase_);
  if (en) {
    bit &= status_.mask2;
    // request
    Log("INT%d REQ - %s :", port - i_portbase_,
        bit ? (bit & status_.mask ? "accept" : "denied") : "discarded");
    if (!(status_.irq & bit)) {
      status_.irq |= bit;
      IRQ((status_.irq & status_.mask & status_.mask2) != 0);
    } else
      Log("\n");
  } else {
    // cancel
    if (status_.irq & bit) {
      status_.irq &= ~bit;
      IRQ((status_.irq & status_.mask & status_.mask2) != 0);
    }
  }
}

// ---------------------------------------------------------------------------
//  CPU が割り込みを受け取った
//
uint32_t IOCALL InterruptController::IntAck(uint32_t) {
  uint32_t ai = status_.irq & status_.mask & status_.mask2;
  for (int i = 0; i < 8; i++, ai >>= 1) {
    if (ai & 1) {
      status_.irq &= ~(1 << i);
      status_.mask = 0;
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
  const static int8_t table[8] = {~7, ~3, ~5, ~1, ~6, ~2, ~4, ~0};
  status_.mask2 = table[data & 7];
  status_.irq &= status_.mask2;
  Log("p[e6] = %.2x (%.2x) : ", data, status_.mask2);
  IRQ((status_.irq & status_.mask & status_.mask2) != 0);
}

// ---------------------------------------------------------------------------
//  レジスタ設定(porte4)
//
void IOCALL InterruptController::SetRegister(uint32_t, uint32_t data) {
  status_.mask = ~(-1 << std::min(8U, data));
  //  mode = (data & 7) != 0;
  Log("p[e4] = %.2x  : ", data);
  IRQ((status_.irq & status_.mask & status_.mask2) != 0);
}

// ---------------------------------------------------------------------------
//  状態保存
//
uint32_t IFCALL InterruptController::GetStatusSize() {
  return sizeof(Status);
}

bool IFCALL InterruptController::SaveStatus(uint8_t* s) {
  *(Status*)s = status_;
  return true;
}

bool IFCALL InterruptController::LoadStatus(const uint8_t* s) {
  status_ = *(const Status*)s;
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
}  // namespace pc88core
