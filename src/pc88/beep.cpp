// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: beep.cpp,v 1.2 1999/10/10 01:47:04 cisc Exp $

#include "pc88/beep.h"

namespace pc88core {

// ---------------------------------------------------------------------------
//  生成・破棄
//
Beep::Beep(const ID& id) : Device(id), soundcontrol_(0) {}

Beep::~Beep() {
  Cleanup();
}

// ---------------------------------------------------------------------------
//  初期化とか
//
bool Beep::Init() {
  port40_ = 0;
  p40mask_ = 0xa0;
  return true;
}

// ---------------------------------------------------------------------------
//  後片付け
//
void Beep::Cleanup() {
  Connect(0);
}

bool IFCALL Beep::Connect(ISoundControl* control) {
  if (soundcontrol_)
    soundcontrol_->Disconnect(this);
  soundcontrol_ = control;
  if (soundcontrol_)
    soundcontrol_->Connect(this);
  return true;
}

// ---------------------------------------------------------------------------
//  レート設定
//
bool Beep::SetRate(uint32_t rate) {
  pslice_ = 0;
  bslice_ = 0;
  bcount_ = 0;
  bperiod_ = static_cast<int>(2400.0 / rate * (1 << 14));
  return true;
}

// ---------------------------------------------------------------------------
//  ビープ音合成
//
//  0-2000
//   0 -  4     1111
//   5 -  9     0111
//  10 - 14     0011
//  15 - 19     0001
//
void IFCALL Beep::Mix(int32_t* dest, int nsamples) {
  int i;
  int p = port40_ & 0x80 ? 0 : 0x10000;
  int b = port40_ & 0x20 ? 0 : 0x10000;

  uint32_t ps = pslice_, bs = bslice_;
  pslice_ = bslice_ = 0;

  int sample = 0;
  for (i = 0; i < 8; i++) {
    if (ps < 500)
      p ^= 0x10000;
    if (bs < 500)
      b ^= 0x10000;

    sample += (b & bcount_) | p;
    bcount_ += bperiod_;
    ps -= 500;
    bs -= 500;
  }
  sample >>= 6;
  *dest++ += sample;
  *dest++ += sample;

  if (p | b) {
    for (i = nsamples - 1; i > 0; i--) {
      sample = 0;
      for (int j = 0; j < 8; j++) {
        sample += (b & bcount_) | p;
        bcount_ += bperiod_;
      }
      sample >>= 6;
      *dest++ += sample;
      *dest++ += sample;
    }
  }
}

// ---------------------------------------------------------------------------
//  BEEP Port への Out
//
void IOCALL Beep::Out40(uint32_t, uint32_t data) {
  data &= p40mask_;
  int i = data ^ port40_;
  if (i & 0xa0) {
    if (soundcontrol_) {
      soundcontrol_->Update(this);

      int tdiff = soundcontrol_->GetSubsampleTime(this);
      if (i & 0x80)
        pslice_ = tdiff;
      if (i & 0x20)
        bslice_ = tdiff;
    }
    port40_ = data;
  }
}

// ---------------------------------------------------------------------------
//  状態保存
//
uint32_t IFCALL Beep::GetStatusSize() {
  return sizeof(Status);
}

bool IFCALL Beep::SaveStatus(uint8_t* s) {
  Status* st = (Status*)s;
  st->rev = ssrev;
  st->port40 = port40_;
  return true;
}

bool IFCALL Beep::LoadStatus(const uint8_t* s) {
  const Status* st = (const Status*)s;
  if (st->rev != ssrev)
    return false;
  port40_ = st->port40;
  return true;
}

// ---------------------------------------------------------------------------
//  device description
//
const Device::Descriptor Beep::descriptor = {0, outdef};

const Device::OutFuncPtr Beep::outdef[] = {
    static_cast<Device::OutFuncPtr>(&Beep::Out40),
};

}  // namespace pc88core
