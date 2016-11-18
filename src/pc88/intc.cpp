// ---------------------------------------------------------------------------
//  PC-8801 emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  ���荞�݃R���g���[������(��PD8214)
// ---------------------------------------------------------------------------
//  $Id: intc.cpp,v 1.15 2000/06/22 16:22:18 cisc Exp $

#include "win32/headers.h"
#include "common/misc.h"
#include "pc88/intc.h"

//#define LOGNAME "intc"
#include "win32/diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//  �\�z�j��
//
INTC::INTC(const ID& id) : Device(id) {}

INTC::~INTC() {}

// ---------------------------------------------------------------------------
//  Init
//
bool INTC::Init(IOBus* b, uint ip, uint ipbase) {
  bus = b;
  irqport = ip;
  iportbase = ipbase;
  Reset();
  return true;
}

// ---------------------------------------------------------------------------
//  ���荞�ݏ󋵂̍X�V
//
inline void INTC::IRQ(bool flag) {
  bus->Out(irqport, flag);
  LOG1("irq(%d)\n", flag);
}

// ---------------------------------------------------------------------------
//  Reset
//
void IOCALL INTC::Reset(uint, uint) {
  stat.irq = stat.mask = stat.mask2 = 0;
  IRQ(false);
}

// ---------------------------------------------------------------------------
//  ���荞�ݗv��
//
void IOCALL INTC::Request(uint port, uint en) {
  uint bit = 1 << (port - iportbase);
  if (en) {
    bit &= stat.mask2;
    // request
    LOG2("INT%d REQ - %s :", port - iportbase,
         bit ? (bit & stat.mask ? "accept" : "denied") : "discarded");
    if (!(stat.irq & bit)) {
      stat.irq |= bit;
      IRQ((stat.irq & stat.mask & stat.mask2) != 0);
    } else
      LOG0("\n");
  } else {
    // cancel
    if (stat.irq & bit) {
      stat.irq &= ~bit;
      IRQ((stat.irq & stat.mask & stat.mask2) != 0);
    }
  }
}

// ---------------------------------------------------------------------------
//  CPU �����荞�݂��󂯎����
//
uint IOCALL INTC::IntAck(uint) {
  uint ai = stat.irq & stat.mask & stat.mask2;
  for (int i = 0; i < 8; i++, ai >>= 1) {
    if (ai & 1) {
      stat.irq &= ~(1 << i);
      stat.mask = 0;
      LOG1("INT%d ACK  : ", i);
      IRQ(false);

      return i * 2;
    }
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  �}�X�N�ݒ�(porte6)
//
void IOCALL INTC::SetMask(uint, uint data) {
  const static int8 table[8] = {~7, ~3, ~5, ~1, ~6, ~2, ~4, ~0};
  stat.mask2 = table[data & 7];
  stat.irq &= stat.mask2;
  LOG2("p[e6] = %.2x (%.2x) : ", data, stat.mask2);
  IRQ((stat.irq & stat.mask & stat.mask2) != 0);
}

// ---------------------------------------------------------------------------
//  ���W�X�^�ݒ�(porte4)
//
void IOCALL INTC::SetRegister(uint, uint data) {
  stat.mask = ~(-1 << Min(8, data));
  //  mode = (data & 7) != 0;
  LOG1("p[e4] = %.2x  : ", data);
  IRQ((stat.irq & stat.mask & stat.mask2) != 0);
}

// ---------------------------------------------------------------------------
//  ��ԕۑ�
//
uint IFCALL INTC::GetStatusSize() {
  return sizeof(Status);
}

bool IFCALL INTC::SaveStatus(uint8* s) {
  *(Status*)s = stat;
  return true;
}

bool IFCALL INTC::LoadStatus(const uint8* s) {
  stat = *(const Status*)s;
  return true;
}

// ---------------------------------------------------------------------------
//  device description
//
const Device::Descriptor INTC::descriptor = {INTC::indef, INTC::outdef};

const Device::OutFuncPtr INTC::outdef[] = {
    STATIC_CAST(Device::OutFuncPtr, &INTC::Reset),
    STATIC_CAST(Device::OutFuncPtr, &INTC::Request),
    STATIC_CAST(Device::OutFuncPtr, &INTC::SetMask),
    STATIC_CAST(Device::OutFuncPtr, &INTC::SetRegister),
};

const Device::InFuncPtr INTC::indef[] = {
    STATIC_CAST(Device::InFuncPtr, &INTC::IntAck),
};
