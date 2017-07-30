// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//  $Id: opnif.cpp,v 1.24 2003/09/28 14:35:35 cisc Exp $

#include "pc88/opn_interface.h"

#include <string.h>

#include "common/scheduler.h"
#include "common/time_keeper.h"
#include "common/toast.h"
#include "pc88/config.h"

#define LOGNAME "opnif"
#include "common/diag.h"

namespace pc88core {

// プリスケーラの設定値
// static にするのは，FMGen の制限により，複数の OPN を異なるクロックに
// することが出来ないため．
int OPNInterface::prescaler_ = 0x2d;

OPNInterface::OPNInterface(const ID& id) : Device(id) {}

OPNInterface::~OPNInterface() {
  Connect(nullptr);
}

bool OPNInterface::Init(IOBus* b, int intrport, int port, Scheduler* s) {
  bus_ = b;
  scheduler_ = s;
  portio_ = port;
  opn_.SetIntr(bus_, intrport);
  clock = kBaseClock;

  if (!opn_.Init(clock, 8000, 0))
    return false;
  prevtime_ = scheduler_->GetTime();
  TimeEvent(1);

  return true;
}

void OPNInterface::SetIMask(uint32_t port, uint32_t bit) {
  imaskport_ = port;
  imaskbit_ = bit;
}

bool IFCALL OPNInterface::Connect(ISoundControl* c) {
  if (soundcontrol_)
    soundcontrol_->Disconnect(this);
  soundcontrol_ = c;
  if (soundcontrol_)
    soundcontrol_->Connect(this);
  return true;
}

bool IFCALL OPNInterface::SetRate(uint32_t rate) {
  opn_.SetReg(prescaler_, 0);
  opn_.SetRate(clock, rate, fmmixmode_);
  currentrate_ = rate;
  return true;
}

// FM 音源の合成モードを設定
void OPNInterface::SetFMMixMode(bool mm) {
  fmmixmode_ = mm;
  SetRate(currentrate_);
}

void IFCALL OPNInterface::Mix(int32_t* dest, int nsamples) {
  if (enable_)
    opn_.Mix(dest, nsamples);
}

// 音量設定
static inline int ConvertVolume(int volume) {
  return volume > -40 ? volume : -200;
}

void OPNInterface::SetVolume(const Config* config) {
  opn_.SetVolumeFM(ConvertVolume(config->volfm));
  opn_.SetVolumePSG(ConvertVolume(config->volssg));
  opn_.SetVolumeADPCM(ConvertVolume(config->voladpcm));
  opn_.SetVolumeRhythmTotal(ConvertVolume(config->volrhythm));
  opn_.SetVolumeRhythm(0, ConvertVolume(config->volbd));
  opn_.SetVolumeRhythm(1, ConvertVolume(config->volsd));
  opn_.SetVolumeRhythm(2, ConvertVolume(config->voltop));
  opn_.SetVolumeRhythm(3, ConvertVolume(config->volhh));
  opn_.SetVolumeRhythm(4, ConvertVolume(config->voltom));
  opn_.SetVolumeRhythm(5, ConvertVolume(config->volrim));
}

void IOCALL OPNInterface::Reset(uint32_t, uint32_t) {
  memset(regs_, 0, sizeof(regs_));

  regs_[0x29] = 0x1f;
  regs_[0x110] = 0x1c;
  for (int i = 0; i < 3; i++)
    regs_[0xb4 + i] = regs_[0x1b4 + i] = 0xc0;

  opn_.Reset();
  opn_.SetIntrMask(true);
  prescaler_ = 0x2d;
}

void OPNInterface::OPNUnit::Intr(bool flag) {
  bool prev = interrup_pending_ && interrupt_enabled_ && bus_;
  interrup_pending_ = flag;
  Log("OPN     :Interrupt %d %d %d\n", interrup_pending_, interrupt_enabled_, !prev);
  if (interrup_pending_ && interrupt_enabled_ && bus_ && !prev) {
    bus_->Out(pintr_, true);
  }
}

// 割り込み許可？
inline void OPNInterface::OPNUnit::SetIntrMask(bool en) {
  bool prev = interrup_pending_ && interrupt_enabled_ && bus_;
  interrupt_enabled_ = en;
  if (interrup_pending_ && interrupt_enabled_ && bus_ && !prev)
    bus_->Out(pintr_, true);
}

void OPNInterface::SetIntrMask(uint32_t port, uint32_t intrmask) {
  //  Log("Intr enabled (%.2x)[%.2x]\n", a, intrmask);
  if (port == imaskport_) {
    opn_.SetIntrMask(!(imaskbit_ & intrmask));
  }
}

void IOCALL OPNInterface::SetIndex0(uint32_t a, uint32_t data) {
  //  Log("Index0[%.2x] = %.2x\n", a, data);
  index0_ = data;
  if (enable_ && (data & 0xfc) == 0x2c) {
    regs_[0x2f] = 1;
    prescaler_ = data;
    opn_.SetReg(data, 0);
  }
}

void IOCALL OPNInterface::SetIndex1(uint32_t a, uint32_t data) {
  //  Log("Index1[%.2x] = %.2x\n", a, data);
  index1_ = data1_ = data;
}

void IOCALL OPNInterface::WriteData0(uint32_t a, uint32_t data) {
  //  Log("Write0[%.2x] = %.2x\n", a, data);
  if (enable_) {
    Log("%.8x:OPN[0%.2x] = %.2x\n", scheduler_->GetTime(), index0_, data);
    TimeEvent(0);

    if (!opnamode_) {
      if ((index0_ & 0xf0) == 0x10)
        return;
      if (index0_ == 0x22)
        data = 0;
      if (index0_ == 0x28 && (data & 4))
        return;
      if (index0_ == 0x29)
        data = 3;
      if (index0_ >= 0xb4)
        data = 0xc0;
    }
    regs_[index0_] = data;
    opn_.SetReg(index0_, data);
    if (index0_ == 0x27) {
      UpdateTimer();
    }
  }
}

void IOCALL OPNInterface::WriteData1(uint32_t a, uint32_t data) {
//  Log("Write1[%.2x] = %.2x\n", a, data);
  if (enable_ && opnamode_) {
    Log("%.8x:OPN[1%.2x] = %.2x\n", scheduler_->GetTime(), index1_, data);
    if (index1_ != 0x08 && index1_ != 0x10)
      TimeEvent(0);
    data1_ = data;
    regs_[0x100 | index1_] = data;
    opn_.SetReg(0x100 | index1_, data);
  }
}

uint32_t IOCALL OPNInterface::ReadData0(uint32_t a) {
  uint32_t ret;
  if (!enable_)
    ret = 0xff;
  else if ((index0_ & 0xfe) == 0x0e)
    ret = bus_->In(portio_ + (index0_ & 1));
  else if (index0_ == 0xff && !opnamode_)
    ret = 0;
  else
    ret = opn_.GetReg(index0_);
  //  Log("Read0 [%.2x] = %.2x\n", a, ret);
  return ret;
}

uint32_t IOCALL OPNInterface::ReadData1(uint32_t a) {
  uint32_t ret = 0xff;
  if (enable_ && opnamode_) {
    if (index1_ == 0x08)
      ret = opn_.GetReg(0x100 | index1_);
    else
      ret = data1_;
  }
//  Log("Read1 [%.2x] = %.2x  (d1:%.2x)\n", a, ret, data1_);
  return ret;
}

uint32_t IOCALL OPNInterface::ReadStatus(uint32_t a) {
  uint32_t ret = enable_ ? opn_.ReadStatus() : 0xff;
  //  Log("status[%.2x] = %.2x\n", a, ret);
  return ret;
}

uint32_t IOCALL OPNInterface::ReadStatusEx(uint32_t a) {
  uint32_t ret = enable_ && opnamode_ ? opn_.ReadStatusEx() : 0xff;
  //  Log("statex[%.2x] = %.2x\n", a, ret);
  return ret;
}

void OPNInterface::UpdateTimer() {
  scheduler_->DelEvent(this);
  int next_us = opn_.GetNextEvent();
  if (next_us > 0) {
    SchedTimeDelta nextcount = TimeKeeper::FromMicroSeconds(next_us);
    scheduler_->AddEvent(nextcount, this,
                        static_cast<TimeFunc>(&OPNInterface::TimeEvent), 1);
  }
}

void IOCALL OPNInterface::TimeEvent(uint32_t e) {
  int currenttime = scheduler_->GetTime();
  int diff = currenttime - prevtime_;
  prevtime_ = currenttime;

  if (enable_) {
    Log("%.8x:TimeEvent(%d) : diff:%d\n", currenttime, e, diff);

    if (soundcontrol_)
      soundcontrol_->Update(this);
    if (opn_.Count(diff * 10) || e)
      UpdateTimer();
  }
}

uint32_t IFCALL OPNInterface::GetStatusSize() {
  if (enable_)
    return sizeof(Status) + (opnamode_ ? 0x40000 : 0);
  else
    return 0;
}

bool IFCALL OPNInterface::SaveStatus(uint8_t* s) {
  Status* st = (Status*)s;
  st->rev = ssrev;
  st->i0 = index0_;
  st->i1 = index1_;
  st->d0 = 0;
  st->d1 = data1_;
  st->is = opn_.IntrStat();
  memcpy(st->regs, regs_, 0x200);
  if (opnamode_)
    memcpy(s + sizeof(Status), opn_.GetADPCMBuffer(), 0x40000);
  return true;
}

bool IFCALL OPNInterface::LoadStatus(const uint8_t* s) {
  const Status* st = (const Status*)s;
  if (st->rev != ssrev)
    return false;

  prevtime_ = scheduler_->GetTime();

  int i;
  for (i = 8; i <= 0x0a; i++)
    opn_.SetReg(i, 0);
  for (i = 0x40; i < 0x4f; i++)
    opn_.SetReg(i, 0x7f), opn_.SetReg(i + 0x100, 0x7f);

  for (i = 0; i < 0x10; i++)
    SetIndex0(0, i), WriteData0(0, st->regs[i]);

  opn_.SetReg(0x10, 0xdf);

  for (i = 11; i < 0x28; i++)
    SetIndex0(0, i), WriteData0(0, st->regs[i]);

  SetIndex0(0, 0x29), WriteData0(0, st->regs[0x29]);

  for (i = 0x30; i < 0xa0; i++) {
    index0_ = i, WriteData0(0, st->regs[i]);
    index1_ = i, WriteData1(0, st->regs[i + 0x100]);
  }
  for (i = 0xb0; i < 0xb7; i++) {
    index0_ = i, WriteData0(0, st->regs[i]);
    index1_ = i, WriteData1(0, st->regs[i + 0x100]);
  }
  for (i = 0; i < 3; i++) {
    index0_ = 0xa4 + i, WriteData0(0, st->regs[0xa4 + i]);
    index0_ = 0xa0 + i, WriteData0(0, st->regs[0xa0 + i]);
    index0_ = 0xac + i, WriteData0(0, st->regs[0xac + i]);
    index0_ = 0xa8 + i, WriteData0(0, st->regs[0xa8 + i]);
    index1_ = 0xa4 + i, WriteData1(0, st->regs[0x1a4 + i]);
    index1_ = 0xa0 + i, WriteData1(0, st->regs[0x1a0 + i]);
    index1_ = 0xac + i, WriteData1(0, st->regs[0x1ac + i]);
    index1_ = 0xa8 + i, WriteData1(0, st->regs[0x1a8 + i]);
  }
  for (index1_ = 0x00; index1_ < 0x08; index1_++)
    WriteData1(0, st->regs[0x100 | index1_]);
  for (index1_ = 0x09; index1_ < 0x0e; index1_++)
    WriteData1(0, st->regs[0x100 | index1_]);

  opn_.SetIntrMask(!!(st->is & 1));
  opn_.Intr(!!(st->is & 2));
  index0_ = st->i0;
  index1_ = st->i1;
  data1_ = st->d1;
  if (opnamode_)
    memcpy(opn_.GetADPCMBuffer(), s + sizeof(Status), 0x40000);
  UpdateTimer();
  return true;
}

// カウンタを同期
void IOCALL OPNInterface::Sync(uint32_t, uint32_t) {
  // TODO: This used to sync with external sound source (romeo).
}

const Device::Descriptor OPNInterface::descriptor = {indef, outdef};

const Device::OutFuncPtr OPNInterface::outdef[] = {
    static_cast<Device::OutFuncPtr>(&OPNInterface::Reset),
    static_cast<Device::OutFuncPtr>(&OPNInterface::SetIndex0),
    static_cast<Device::OutFuncPtr>(&OPNInterface::SetIndex1),
    static_cast<Device::OutFuncPtr>(&OPNInterface::WriteData0),
    static_cast<Device::OutFuncPtr>(&OPNInterface::WriteData1),
    static_cast<Device::OutFuncPtr>(&OPNInterface::SetIntrMask),
    static_cast<Device::OutFuncPtr>(&OPNInterface::Sync),
};

const Device::InFuncPtr OPNInterface::indef[] = {
    static_cast<Device::InFuncPtr>(&OPNInterface::ReadStatus),
    static_cast<Device::InFuncPtr>(&OPNInterface::ReadStatusEx),
    static_cast<Device::InFuncPtr>(&OPNInterface::ReadData0),
    static_cast<Device::InFuncPtr>(&OPNInterface::ReadData1),
};
}  // namespace pc88core
