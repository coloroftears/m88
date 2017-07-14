// ---------------------------------------------------------------------------
//  M88 - PC-8801 Series Emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  Implementation of USART(uPD8251AF)
// ---------------------------------------------------------------------------
//  $Id: sio.cpp,v 1.6 2001/02/21 11:57:57 cisc Exp $

#include "pc88/sio.h"

#define LOGNAME "sio"
#include "common/diag.h"

namespace pc88core {

// ---------------------------------------------------------------------------
//  構築・破棄
//
SIO::SIO(const ID& id) : Device(id) {}

SIO::~SIO() {}

// ---------------------------------------------------------------------------
//  初期化
//
bool SIO::Init(IOBus* _bus, uint32_t _prxrdy, uint32_t _preq) {
  bus = _bus, prxrdy = _prxrdy, prequest = _preq;
  Log("SIO::Init\n");

  return true;
}

// ---------------------------------------------------------------------------
//  りせっと
//
void SIO::Reset(uint32_t, uint32_t) {
  mode_ = clear;
  status = TXRDY | TXE;
  baseclock = 1200 * 64;
}

// ---------------------------------------------------------------------------
//  こんとろーるぽーと
//
void IOCALL SIO::SetControl(uint32_t, uint32_t d) {
  Log("[%.2x] ", d);

  switch (mode_) {
    case clear:
      // Mode Instruction
      if (d & 3) {
        // Asynchronus mode
        mode_ = async;
        // b7 b6 b5 b4 b3 b2 b1 b0
        // STOP  EP PE CHAR  RATE
        static const int clockdiv[] = {1, 1, 16, 64};
        clock = baseclock / clockdiv[d & 3];
        datalen = 5 + ((d >> 2) & 3);
        parity = d & 0x10 ? (d & 0x20 ? even : odd) : none;
        stop = (d >> 6) & 3;
        Log("Async: %d baud, Parity:%c Data:%d Stop:%s\n", clock, parity,
            datalen, stop == 3 ? "2" : stop == 2 ? "1.5" : "1");
      } else {
        // Synchronus mode
        mode_ = sync1;
        clock = 0;
        parity = d & 0x10 ? (d & 0x20 ? even : odd) : none;
        Log("Sync: %d baud, Parity:%c / ", clock, parity);
      }
      break;

    case sync1:
      mode_ = sync2;
      break;

    case sync2:
      mode_ = sync;
      Log("\n");
      break;

    case async:
    case sync:
      // Command Instruction
      // b7 - enter hunt mode
      // b6 - internal reset
      if (d & 0x40) {
        // Reset!
        Log(" Internal Reset!\n");
        mode_ = clear;
        break;
      }
      // b5 - request to send
      // b4 - error reset
      if (d & 0x10) {
        Log(" ERRCLR");
        status &= ~(PE | OE | FE);
      }
      // b3 - send break charactor
      if (d & 8) {
        Log(" SNDBRK");
      }
      // b2 - receive enable
      rxen = (d & 4) != 0;
      // b1 - data terminal ready
      // b0 - send enable
      txen = (d & 1) != 0;

      Log(" RxE:%d TxE:%d\n", rxen, txen);
      break;
    default:
      Log("internal error? <%d>\n", mode_);
      break;
  }
}

// ---------------------------------------------------------------------------
//  でーたせっと
//
void IOCALL SIO::SetData(uint32_t, uint32_t /* d*/) {
  // Log("<%.2x ", d);
}

// ---------------------------------------------------------------------------
//  じょうたいしゅとく
//
uint32_t IOCALL SIO::GetStatus(uint32_t) {
  //  Log("!%.2x ", status      );
  return status;
}

// ---------------------------------------------------------------------------
//  でーたしゅとく
//
uint32_t IOCALL SIO::GetData(uint32_t) {
  Log(">%.2x ", data);

  int f = status & RXRDY;
  status &= ~RXRDY;

  if (f)
    bus->Out(prequest, 0);

  return data;
}

void IOCALL SIO::AcceptData(uint32_t, uint32_t d) {
  Log("Accept: [%.2x]", d);
  if (rxen) {
    data = d;
    if (status & RXRDY) {
      status |= OE;
      Log(" - overrun");
    }
    status |= RXRDY;
    bus->Out(prxrdy, 1);  // 割り込み
    Log("\n");
  } else {
    Log(" - ignored\n");
  }
}

// ---------------------------------------------------------------------------
//  状態保存
//
uint32_t IFCALL SIO::GetStatusSize() {
  return sizeof(Status);
}

bool IFCALL SIO::SaveStatus(uint8_t* s) {
  Status* ss = (Status*)s;
  ss->rev = SSREV;
  ss->baseclock = baseclock;
  ss->clock = clock;
  ss->datalen = datalen;
  ss->stop = stop;
  ss->data = data;
  ss->mode_ = mode_;
  ss->parity = parity;
  return true;
}

bool IFCALL SIO::LoadStatus(const uint8_t* s) {
  const Status* ss = (const Status*)s;
  if (ss->rev != SSREV)
    return false;
  baseclock = ss->baseclock;
  clock = ss->clock;
  datalen = ss->datalen;
  stop = ss->stop;
  data = ss->data;
  mode_ = ss->mode_;
  parity = ss->parity;
  return true;
}

// ---------------------------------------------------------------------------
//  device description
//
const Device::Descriptor SIO::descriptor = {indef, outdef};

const Device::OutFuncPtr SIO::outdef[] = {
    static_cast<Device::OutFuncPtr>(&SIO::Reset),
    static_cast<Device::OutFuncPtr>(&SIO::SetControl),
    static_cast<Device::OutFuncPtr>(&SIO::SetData),
    static_cast<Device::OutFuncPtr>(&SIO::AcceptData),
};

const Device::InFuncPtr SIO::indef[] = {
    static_cast<Device::InFuncPtr>(&SIO::GetStatus),
    static_cast<Device::InFuncPtr>(&SIO::GetData),
};
}  // namespace pc88core
