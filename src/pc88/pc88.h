// ---------------------------------------------------------------------------
//  PC-8801 emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: pc88.h,v 1.45 2003/04/22 13:16:34 cisc Exp $

#pragma once

#include <memory>

#include "common/device.h"
#include "common/draw.h"
#include "common/scheduler.h"
#include "common/sequencer.h"

// ---------------------------------------------------------------------------
//  使用する Z80 エンジンの種類を決める
//  標準では C++ 版の Z80 エンジンは Release 版ではコンパイルしない設定に
//  なっているので注意！
//
#ifdef USE_Z80_X86
#include "devices/z80_x86.h"
#else
#include "devices/z80c.h"
#endif

// ---------------------------------------------------------------------------
//  仮宣言
//
class DiskManager;
class TapeManager;

namespace PC8801 {
class Base;
class Beep;
class CDIF;
class CRTC;
class Calendar;
class Config;
class DiskIO;
class FDC;
class INTC;
class JoyPad;
class KanjiROM;
class Memory;
class OPNIF;
class PD8257;
class SIO;
class Screen;
class SubSystem;
}

// ---------------------------------------------------------------------------
//  PC8801 クラス
//
class PC88 : public SchedulerDelegate,
             public SequencerDelegate,
             public ICPUTime {
 public:
#if defined(USE_Z80_X86)
  using Z80 = Z80_x86;
#else
  using Z80 = Z80C;
#endif

 public:
  PC88();
  ~PC88();

  bool Init(Draw* draw, DiskManager* diskmgr, TapeManager* tape);

  void Reset();

  // Overrides SequencerDelegate.
  SchedTimeDelta Proceed(SchedTimeDelta us,
                         SchedClock clock,
                         uint32_t eff) final;
  void TimeSync() final;
  void UpdateScreen(bool refresh = false) final;
  SchedTimeDelta GetFramePeriod() const final;

  // Override ICPUTime.
  uint32_t IFCALL GetCPUTick() final { return cpu1.GetCount(); }
  uint32_t IFCALL GetCPUSpeed() final { return clock_; }

  void ApplyConfig(PC8801::Config*);
  void SetVolume(PC8801::Config*);

  uint32_t GetEffectiveSpeed() { return eclock_; }

  bool IsCDSupported();

  PC8801::Memory* GetMem1() { return mem1; }
  PC8801::SubSystem* GetMem2() { return subsys; }
  PC8801::OPNIF* GetOPN1() { return opn1; }
  PC8801::OPNIF* GetOPN2() { return opn2; }
  Z80* GetCPU1() { return &cpu1; }
  Z80* GetCPU2() { return &cpu2; }
  PC8801::PD8257* GetDMAC() { return dmac; }
  PC8801::Beep* GetBEEP() { return beep; }

  Scheduler* GetScheduler() const { return sched_.get(); }

 public:
  enum SpecialPort {
    pint0 = 0x100,
    pint1,
    pint2,
    pint3,
    pint4,
    pint5,
    pint6,
    pint7,
    pres,     // reset
    pirq,     // IRQ
    piack,    // interrupt acknowledgement
    vrtc,     // vertical retrace
    popnio,   // OPN の入出力ポート 1
    popnio2,  // OPN の入出力ポート 2 (連番)
    psioin,   // SIO 関係
    psioreq,
    ptimesync,
    portend
  };

  enum SpecialPort2 {
    pres2 = 0x100,
    pirq2,
    piac2,
    pfdstat,  // FD の動作状況 (b0-1 = LAMP, b2-3 = MODE, b4=SEEK)
    portend2
  };

 private:
  void VSync();

  // Overrides SchedulerDelegate.
  SchedTimeDelta Execute(SchedTimeDelta ticks) final;
  void Shorten(SchedTimeDelta ticks) final;
  SchedTimeDelta GetTicks() final;

  bool ConnectDevices();
  bool ConnectDevices2();

 private:
  enum CPUMode {
    ms11 = 0,
    ms21 = 1,          // bit 0
    stopwhenidle = 4,  // bit 2
  };

  std::unique_ptr<Scheduler> sched_;

  Draw::Region region;

  SchedClock clock_ = 1;
  int cpumode;
  int dexc;
  int eclock_;

  uint32_t cfgflags;
  uint32_t cfgflag2;
  bool updated;

  PC8801::Memory* mem1;
  PC8801::KanjiROM* knj1;
  PC8801::KanjiROM* knj2;
  PC8801::Screen* scrn;
  PC8801::INTC* intc;
  PC8801::CRTC* crtc;
  PC8801::Base* base;
  PC8801::FDC* fdc;
  PC8801::SubSystem* subsys;
  PC8801::SIO* siotape;
  PC8801::SIO* siomidi;
  PC8801::OPNIF* opn1;
  PC8801::OPNIF* opn2;
  PC8801::Calendar* caln;
  PC8801::Beep* beep;
  PC8801::PD8257* dmac;

 protected:
  Draw* draw;
  DiskManager* diskmgr;
  TapeManager* tapemgr;
  PC8801::JoyPad* joypad;

  MemoryManager mm1, mm2;
  IOBus bus1, bus2;
  DeviceList devlist;

 private:
  Z80 cpu1;
  Z80 cpu2;

  friend class PC8801::Base;
};

inline bool PC88::IsCDSupported() {
  return devlist.Find(DEV_ID('c', 'd', 'i', 'f')) != 0;
}
