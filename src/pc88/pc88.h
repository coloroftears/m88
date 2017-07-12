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

namespace pc88core {

class Base;
class Beep;
class CRTC;
class Calendar;
class Config;
class DiskIO;
class DiskManager;
class FDC;
class InterruptController;
class JoyPad;
class KanjiROM;
class Memory;
class OPNInterface;
class PD8257;
class SIO;
class Screen;
class SubSystem;
class TapeManager;

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
  SchedTimeDelta Proceed(SchedTimeDelta ticks,
                         SchedClock clock,
                         SchedClock effective_clock) final;
  void TimeSync() final;
  void UpdateScreen(bool refresh = false) final;
  SchedTimeDelta GetFramePeriod() const final;

  // Override ICPUTime.
  uint32_t IFCALL GetCPUTick() final { return cpu1.GetCount(); }
  uint32_t IFCALL GetCPUSpeed() final { return clock_; }

  void ApplyConfig(pc88core::Config*);
  void SetVolume(pc88core::Config*);

  uint32_t GetEffectiveSpeed() { return eclock_; }

  bool IsCDSupported();

  pc88core::Memory* GetMem1() { return mem1; }
  pc88core::SubSystem* GetMem2() { return subsys; }
  pc88core::OPNInterface* GetOPN1() { return opn1; }
  pc88core::OPNInterface* GetOPN2() { return opn2; }
  Z80* GetCPU1() { return &cpu1; }
  Z80* GetCPU2() { return &cpu2; }
  pc88core::PD8257* GetDMAC() { return dmac; }
  pc88core::Beep* GetBEEP() { return beep; }

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

  pc88core::Memory* mem1;
  pc88core::KanjiROM* knj1;
  pc88core::KanjiROM* knj2;
  pc88core::Screen* scrn;
  pc88core::InterruptController* intc;
  pc88core::CRTC* crtc;
  pc88core::Base* base;
  pc88core::FDC* fdc;
  pc88core::SubSystem* subsys;
  pc88core::SIO* siotape;
  pc88core::SIO* siomidi;
  pc88core::OPNInterface* opn1;
  pc88core::OPNInterface* opn2;
  pc88core::Calendar* caln;
  pc88core::Beep* beep;
  pc88core::PD8257* dmac;

 protected:
  Draw* draw;
  DiskManager* diskmgr_;
  TapeManager* tapemgr_;
  pc88core::JoyPad* joypad;

  MemoryManager mm1, mm2;
  IOBus bus1, bus2;
  DeviceList devlist_;

 private:
  Z80 cpu1;
  Z80 cpu2;

  friend class pc88core::Base;
};

inline bool PC88::IsCDSupported() {
  return devlist_.Find(DEV_ID('c', 'd', 'i', 'f')) != 0;
}

}  // namespace pc88core
