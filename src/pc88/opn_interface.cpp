// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//  $Id: opnif.cpp,v 1.24 2003/09/28 14:35:35 cisc Exp $

#include "pc88/opn_interface.h"

#include "common/scheduler.h"
#include "common/time_keeper.h"
#include "common/toast.h"
#include "pc88/config.h"

#include "win32/romeo/piccolo.h"

//#include "romeo/juliet.h"

#define LOGNAME "opnif"
#include "common/diag.h"

using namespace PC8801;

// OPNInterface* OPNInterface::romeo_user = 0;

#define ROMEO_JULIET 0

// ---------------------------------------------------------------------------
//  プリスケーラの設定値
//  static にするのは，FMGen の制限により，複数の OPN を異なるクロックに
//  することが出来ないため．
//
int OPNInterface::prescaler = 0x2d;

// ---------------------------------------------------------------------------
//  生成・破棄
//
OPNInterface::OPNInterface(const ID& id) : Device(id), chip(0) {
  Log("Hello\n");
  scheduler = 0;
  soundcontrol = 0;
  opnamode = false;
  fmmixmode = true;
  imaskport = 0;

  delay = 100000;
}

OPNInterface::~OPNInterface() {
  Connect(0);
}

// ---------------------------------------------------------------------------
//  初期化
//
bool OPNInterface::Init(IOBus* b, int intrport, int io, Scheduler* s) {
  bus = b;
  scheduler = s;
  portio = io;
  opn.SetIntr(bus, intrport);
  clock = baseclock;

  if (!opn.Init(clock, 8000, 0))
    return false;
  prevtime = scheduler->GetTime();
  TimeEvent(1);

  piccolo = Piccolo::GetInstance();
  if (piccolo) {
    Log("asking piccolo to obtain YMF288 instance\n");
    if (piccolo->GetChip(PICCOLO_YMF288, &chip) >= 0) {
      Log(" success.\n");
      if (piccolo->IsDriverBased())
        Toast::Show(100, 10000, "ROMEO_PICCOLO: YMF288 enabled");
      else
        Toast::Show(100, 10000, "ROMEO_JULIET: YMF288 enabled");
#ifdef USE_OPN
      clock = 4000000;
#else
      clock = 8000000;
#endif
      //  opn.Init(clock, 8000, 0);
    }
  }

  return true;
}

void OPNInterface::SetIMask(uint32_t port, uint32_t bit) {
  imaskport = port;
  imaskbit = bit;
}

// ---------------------------------------------------------------------------
//
//
bool IFCALL OPNInterface::Connect(ISoundControl* c) {
  if (soundcontrol)
    soundcontrol->Disconnect(this);
  soundcontrol = c;
  if (soundcontrol)
    soundcontrol->Connect(this);
  return true;
}

// ---------------------------------------------------------------------------
//  合成・再生レート設定
//
bool IFCALL OPNInterface::SetRate(uint32_t rate) {
  opn.SetReg(prescaler, 0);
  opn.SetRate(clock, rate, fmmixmode);
  currentrate = rate;
  return true;
}

// ---------------------------------------------------------------------------
//  FM 音源の合成モードを設定
//
void OPNInterface::SetFMMixMode(bool mm) {
  fmmixmode = mm;
  SetRate(currentrate);
}

// ---------------------------------------------------------------------------
//  合成
//
void IFCALL OPNInterface::Mix(int32_t* dest, int nsamples) {
  if (enable)
    opn.Mix(dest, nsamples);
}

// ---------------------------------------------------------------------------
//  音量設定
//
static inline int ConvertVolume(int volume) {
  return volume > -40 ? volume : -200;
}

void OPNInterface::SetVolume(const Config* config) {
  opn.SetVolumeFM(ConvertVolume(config->volfm));
  opn.SetVolumePSG(ConvertVolume(config->volssg));
#ifndef USE_OPN
  opn.SetVolumeADPCM(ConvertVolume(config->voladpcm));
  opn.SetVolumeRhythmTotal(ConvertVolume(config->volrhythm));
  opn.SetVolumeRhythm(0, ConvertVolume(config->volbd));
  opn.SetVolumeRhythm(1, ConvertVolume(config->volsd));
  opn.SetVolumeRhythm(2, ConvertVolume(config->voltop));
  opn.SetVolumeRhythm(3, ConvertVolume(config->volhh));
  opn.SetVolumeRhythm(4, ConvertVolume(config->voltom));
  opn.SetVolumeRhythm(5, ConvertVolume(config->volrim));
#endif

  if (chip) {
    delay = config->romeolatency * 1000;
  }
}

// ---------------------------------------------------------------------------
//  Reset
//
void IOCALL OPNInterface::Reset(uint32_t, uint32_t) {
  memset(regs, 0, sizeof(regs));

  regs[0x29] = 0x1f;
  regs[0x110] = 0x1c;
  for (int i = 0; i < 3; i++)
    regs[0xb4 + i] = regs[0x1b4 + i] = 0xc0;

  opn.Reset();
  opn.SetIntrMask(true);
  prescaler = 0x2d;

  if (chip)
    chip->Reset();
}

// ---------------------------------------------------------------------------
//  割り込み
//
void OPNInterface::OPNUnit::Intr(bool flag) {
  bool prev = intrpending && intrenabled && bus;
  intrpending = flag;
  LOG3("OPN     :Interrupt %d %d %d\n", intrpending, intrenabled, !prev);
  if (intrpending && intrenabled && bus && !prev) {
    bus->Out(pintr, true);
  }
}

// ---------------------------------------------------------------------------
//  割り込み許可？
//
inline void OPNInterface::OPNUnit::SetIntrMask(bool en) {
  bool prev = intrpending && intrenabled && bus;
  intrenabled = en;
  if (intrpending && intrenabled && bus && !prev)
    bus->Out(pintr, true);
}

void OPNInterface::SetIntrMask(uint32_t port, uint32_t intrmask) {
  //  LOG2("Intr enabled (%.2x)[%.2x]\n", a, intrmask);
  if (port == imaskport) {
    opn.SetIntrMask(!(imaskbit & intrmask));
  }
}

// ---------------------------------------------------------------------------
//  SetRegisterIndex
//
void IOCALL OPNInterface::SetIndex0(uint32_t a, uint32_t data) {
  //  LOG2("Index0[%.2x] = %.2x\n", a, data);
  index0 = data;
  if (enable && (data & 0xfc) == 0x2c) {
    regs[0x2f] = 1;
    prescaler = data;
    opn.SetReg(data, 0);
  }
}

void IOCALL OPNInterface::SetIndex1(uint32_t a, uint32_t data) {
  //  LOG2("Index1[%.2x] = %.2x\n", a, data);
  index1 = data1 = data;
}

// ---------------------------------------------------------------------------
//
//
inline uint32_t OPNInterface::ChipTime() {
  return basetime + (scheduler->GetTime() - basetick) * 10 + delay;
}

// ---------------------------------------------------------------------------
//  WriteRegister
//
void IOCALL OPNInterface::WriteData0(uint32_t a, uint32_t data) {
  //  LOG2("Write0[%.2x] = %.2x\n", a, data);
  if (enable) {
    LOG3("%.8x:OPN[0%.2x] = %.2x\n", scheduler->GetTime(), index0, data);
    TimeEvent(0);

    if (!opnamode) {
      if ((index0 & 0xf0) == 0x10)
        return;
      if (index0 == 0x22)
        data = 0;
      if (index0 == 0x28 && (data & 4))
        return;
      if (index0 == 0x29)
        data = 3;
      if (index0 >= 0xb4)
        data = 0xc0;
    }
    regs[index0] = data;
    opn.SetReg(index0, data);
#if ROMEO_JULIET
    if (ROMEOEnabled())
      juliet_YMF288A(index0, data);
#endif
    if (chip && index0 != 0x20)
      chip->SetReg(ChipTime(), index0, data);

    if (index0 == 0x27) {
      UpdateTimer();
    }
  }
}

void IOCALL OPNInterface::WriteData1(uint32_t a, uint32_t data) {
//  LOG2("Write1[%.2x] = %.2x\n", a, data);
#ifndef USE_OPN
  if (enable && opnamode) {
    LOG3("%.8x:OPN[1%.2x] = %.2x\n", scheduler->GetTime(), index1, data);
    if (index1 != 0x08 && index1 != 0x10)
      TimeEvent(0);
    data1 = data;
    regs[0x100 | index1] = data;
    opn.SetReg(0x100 | index1, data);

    if (chip && index1 >= 0x30)
      chip->SetReg(ChipTime(), 0x100 | index1, data);
  }
#endif
}

// ---------------------------------------------------------------------------
//  ReadRegister
//
uint32_t IOCALL OPNInterface::ReadData0(uint32_t a) {
  uint32_t ret;
  if (!enable)
    ret = 0xff;
  else if ((index0 & 0xfe) == 0x0e)
    ret = bus->In(portio + (index0 & 1));
  else if (index0 == 0xff && !opnamode)
    ret = 0;
  else
    ret = opn.GetReg(index0);
  //  LOG2("Read0 [%.2x] = %.2x\n", a, ret);
  return ret;
}

uint32_t IOCALL OPNInterface::ReadData1(uint32_t a) {
  uint32_t ret = 0xff;
#ifndef USE_OPN
  if (enable && opnamode) {
    if (index1 == 0x08)
      ret = opn.GetReg(0x100 | index1);
    else
      ret = data1;
  }
//  LOG3("Read1 [%.2x] = %.2x  (d1:%.2x)\n", a, ret, data1);
#endif
  return ret;
}

// ---------------------------------------------------------------------------
//  ReadStatus
//
uint32_t IOCALL OPNInterface::ReadStatus(uint32_t a) {
  uint32_t ret = enable ? opn.ReadStatus() : 0xff;
  //  LOG2("status[%.2x] = %.2x\n", a, ret);
  return ret;
}

uint32_t IOCALL OPNInterface::ReadStatusEx(uint32_t a) {
  uint32_t ret = enable && opnamode ? opn.ReadStatusEx() : 0xff;
  //  LOG2("statex[%.2x] = %.2x\n", a, ret);
  return ret;
}

// ---------------------------------------------------------------------------
//  タイマー更新
//
void OPNInterface::UpdateTimer() {
  scheduler->DelEvent(this);
  int next_us = opn.GetNextEvent();
  if (next_us > 0) {
    SchedTimeDelta nextcount = TimeKeeper::FromMicroSeconds(next_us);
    scheduler->AddEvent(nextcount, this,
                        static_cast<TimeFunc>(&OPNInterface::TimeEvent), 1);
  }
}

// ---------------------------------------------------------------------------
//  タイマー
//
void IOCALL OPNInterface::TimeEvent(uint32_t e) {
  int currenttime = scheduler->GetTime();
  int diff = currenttime - prevtime;
  prevtime = currenttime;

  if (enable) {
    LOG3("%.8x:TimeEvent(%d) : diff:%d\n", currenttime, e, diff);

    if (soundcontrol)
      soundcontrol->Update(this);
    if (opn.Count(diff * 10) || e)
      UpdateTimer();
  }
}

// ---------------------------------------------------------------------------
//  状態のサイズ
//
uint32_t IFCALL OPNInterface::GetStatusSize() {
  if (enable)
    return sizeof(Status) + (opnamode ? 0x40000 : 0);
  else
    return 0;
}

// ---------------------------------------------------------------------------
//  状態保存
//
bool IFCALL OPNInterface::SaveStatus(uint8_t* s) {
  Status* st = (Status*)s;
  st->rev = ssrev;
  st->i0 = index0;
  st->i1 = index1;
  st->d0 = 0;
  st->d1 = data1;
  st->is = opn.IntrStat();
  memcpy(st->regs, regs, 0x200);
#ifndef USE_OPN
  if (opnamode)
    memcpy(s + sizeof(Status), opn.GetADPCMBuffer(), 0x40000);
#endif
  return true;
}

// ---------------------------------------------------------------------------
//  状態復帰
//
bool IFCALL OPNInterface::LoadStatus(const uint8_t* s) {
  const Status* st = (const Status*)s;
  if (st->rev != ssrev)
    return false;

  prevtime = scheduler->GetTime();

  int i;
  for (i = 8; i <= 0x0a; i++)
    opn.SetReg(i, 0);
  for (i = 0x40; i < 0x4f; i++)
    opn.SetReg(i, 0x7f), opn.SetReg(i + 0x100, 0x7f);

  for (i = 0; i < 0x10; i++)
    SetIndex0(0, i), WriteData0(0, st->regs[i]);

  opn.SetReg(0x10, 0xdf);

  for (i = 11; i < 0x28; i++)
    SetIndex0(0, i), WriteData0(0, st->regs[i]);

  SetIndex0(0, 0x29), WriteData0(0, st->regs[0x29]);

  for (i = 0x30; i < 0xa0; i++) {
    index0 = i, WriteData0(0, st->regs[i]);
    index1 = i, WriteData1(0, st->regs[i + 0x100]);
  }
  for (i = 0xb0; i < 0xb7; i++) {
    index0 = i, WriteData0(0, st->regs[i]);
    index1 = i, WriteData1(0, st->regs[i + 0x100]);
  }
  for (i = 0; i < 3; i++) {
    index0 = 0xa4 + i, WriteData0(0, st->regs[0xa4 + i]);
    index0 = 0xa0 + i, WriteData0(0, st->regs[0xa0 + i]);
    index0 = 0xac + i, WriteData0(0, st->regs[0xac + i]);
    index0 = 0xa8 + i, WriteData0(0, st->regs[0xa8 + i]);
    index1 = 0xa4 + i, WriteData1(0, st->regs[0x1a4 + i]);
    index1 = 0xa0 + i, WriteData1(0, st->regs[0x1a0 + i]);
    index1 = 0xac + i, WriteData1(0, st->regs[0x1ac + i]);
    index1 = 0xa8 + i, WriteData1(0, st->regs[0x1a8 + i]);
  }
  for (index1 = 0x00; index1 < 0x08; index1++)
    WriteData1(0, st->regs[0x100 | index1]);
  for (index1 = 0x09; index1 < 0x0e; index1++)
    WriteData1(0, st->regs[0x100 | index1]);

  opn.SetIntrMask(!!(st->is & 1));
  opn.Intr(!!(st->is & 2));
  index0 = st->i0;
  index1 = st->i1;
  data1 = st->d1;

#ifndef USE_OPN
  if (opnamode)
    memcpy(opn.GetADPCMBuffer(), s + sizeof(Status), 0x40000);
#endif
  UpdateTimer();
  return true;
}

// ---------------------------------------------------------------------------
//  カウンタを同期
//
void IOCALL OPNInterface::Sync(uint32_t, uint32_t) {
  if (chip) {
    basetime = piccolo->GetCurrentTime();
    basetick = scheduler->GetTime();
  }
}

// ---------------------------------------------------------------------------
//  device description
//
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
