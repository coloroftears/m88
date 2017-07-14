// ---------------------------------------------------------------------------
//  PC-8801 emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: base.cpp,v 1.19 2003/09/28 14:35:35 cisc Exp $

#include "pc88/base.h"

#include <stdlib.h>
#include <string.h>

#include "common/draw.h"
#include "common/status.h"
#include "common/toast.h"
#include "pc88/config.h"
#include "pc88/pc88.h"
#include "pc88/tape_manager.h"

#define LOGNAME "base"
#include "common/diag.h"

namespace pc88core {

// ---------------------------------------------------------------------------
//  構築・破壊
//
Base::Base(const ID& id) : Device(id) {
  port40_ = 0;
  autoboot_ = true;
}

Base::~Base() {}

// ---------------------------------------------------------------------------
//  初期化
//
bool Base::Init(PC88* pc88) {
  pc_ = pc88;
  RTC();
  sw30_ = 0xcb;
  sw31_ = 0x79;
  sw6e_ = 0xff;

  // 1/600 RTC interrupt.
  const SchedTimeDelta kRTCInterval =
      Scheduler::SchedTimeDeltaFromMS(1000000 / 600);
  pc_->GetScheduler()->AddEvent(kRTCInterval, this,
                                static_cast<TimeFunc>(&Base::RTC), 0, true);
  return true;
}

// ---------------------------------------------------------------------------
//  スイッチ更新
//
void Base::SetSwitch(const Config* cfg) {
  bmode_ = cfg->basicmode;
  clock_ = cfg->clock();
  dipsw_ = cfg->dipsw();
  flags_ = cfg->flags;
  fv15k_ = cfg->IsFV15k();
}

// ---------------------------------------------------------------------------
//  りせっと
//
void IOCALL Base::Reset(uint32_t, uint32_t) {
  port40_ =
      0xc0 + (fv15k_ ? 2 : 0) + ((dipsw_ & (1 << 11)) || !autoboot_ ? 8 : 0);
  sw6e_ = (sw6e_ & 0x7f) | ((!clock_ || abs(clock_) >= 60) ? 0 : 0x80);
  sw31_ = ((dipsw_ >> 5) & 0x3f) | (bmode_ & 1 ? 0x40 : 0) |
          (bmode_ & 0x10 ? 0 : 0x80);

  if (bmode_ & 2) {
    //  N80モードのときもDipSWを返すようにする(Xanadu対策)
    sw30_ = ~((bmode_ & 0x10) >> 3);
  } else {
    sw30_ = 0xc0 | ((dipsw_ << 1) & 0x3e) | (bmode_ & 0x22 ? 1 : 0);
  }

  const char* mode;
  switch (bmode_) {
    case Config::N80:
      mode = "N";
      break;
    case Config::N802:
      mode = "N80";
      break;
    case Config::N80V2:
      mode = "N80-V2";
      break;
    case Config::N88V1:
      mode = "N88-V1(S)";
      break;
    case Config::N88V1H:
      mode = "N88-V1(H)";
      break;
    case Config::N88V2:
      mode = "N88-V2";
      break;
    case Config::N88V2CD:
      mode = "N88-V2 with CD";
      break;
    default:
      mode = "Unknown";
      break;
  };
  Toast::Show(100, 2000, "%s mode", mode);
}

// ---------------------------------------------------------------------------
//  Real Time Clock Interrupt (600Hz)
//
void IOCALL Base::RTC(uint32_t) {
  pc_->main_bus_.Out(PC88::kPortInt2, 1);
  //  Log("RTC\n");
}

// ---------------------------------------------------------------------------
//  Vertical Retrace Interrupt
//
void IOCALL Base::VRTC(uint32_t, uint32_t en) {
  if (en) {
    pc_->VSync();
    pc_->main_bus_.Out(PC88::kPortInt1, 1);
    port40_ |= 0x20;
    //      Log("CRTC: Retrace\n");
  } else {
    port40_ &= ~0x20;
    //      Log("CRTC: Display\n");
  }
}

// ---------------------------------------------------------------------------
//  In
//
uint32_t IOCALL Base::In30(uint32_t) {
  return sw30_;
}

uint32_t IOCALL Base::In31(uint32_t) {
  return sw31_;
}

uint32_t IOCALL Base::In40(uint32_t) {
  return IOBus::Active(port40_, 0x2a);
}

uint32_t IOCALL Base::In6e(uint32_t) {
  return sw6e_ | 0x7f;
}

// ---------------------------------------------------------------------------
//  device description
//
const Device::Descriptor Base::descriptor = {indef, outdef};

const Device::OutFuncPtr Base::outdef[] = {
    static_cast<Device::OutFuncPtr>(&Base::Reset),
    static_cast<Device::OutFuncPtr>(&Base::VRTC),
};

const Device::InFuncPtr Base::indef[] = {
    static_cast<Device::InFuncPtr>(&Base::In30),
    static_cast<Device::InFuncPtr>(&Base::In31),
    static_cast<Device::InFuncPtr>(&Base::In40),
    static_cast<Device::InFuncPtr>(&Base::In6e),
};

}  // namespace pc88core
