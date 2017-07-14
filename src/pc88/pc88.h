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
  uint32_t IFCALL GetCPUTick() final { return main_cpu_.GetCount(); }
  uint32_t IFCALL GetCPUSpeed() final { return clock_; }

  void ApplyConfig(pc88core::Config*);
  void SetVolume(pc88core::Config*);

  uint32_t GetEffectiveSpeed() { return eclock_; }

  bool IsCDSupported();

  pc88core::Memory* GetMem1() { return mem1; }
  pc88core::SubSystem* GetMem2() { return subsys; }
  pc88core::OPNInterface* GetOPN1() { return opn1; }
  pc88core::OPNInterface* GetOPN2() { return opn2; }
  Z80* GetCPU1() { return &main_cpu_; }
  Z80* GetCPU2() { return &sub_cpu_; }
  pc88core::PD8257* GetDMAC() { return dmac; }
  pc88core::Beep* GetBEEP() { return beep; }

  Scheduler* GetScheduler() const { return sched_.get(); }

 public:
  enum SpecialPort {
    kPortInt0 = 0x100,
    kPortInt1,
    kPortInt2,
    kPortInt3,
    kPortInt4,
    kPortInt5,
    kPortInt6,
    kPortInt7,
    kPortReset,   // reset
    kPortIRQ,     // IRQ
    kPortIntAck,  // interrupt acknowledgement
    kVRTC,        // vertical retrace
    kPortOPNIO,   // OPN の入出力ポート 1
    kPortOPNIO2,  // OPN の入出力ポート 2 (連番)
    kPortSIOIn,   // SIO 関係
    kPortSIOReq,
    kPortTimeSync,
    kPortEnd
  };

  enum SpecialPort2 {
    kPortReset2 = 0x100,
    kPortIRQ2,
    kPortIntAck2,
    kPortFDStat,  // FD の動作状況 (b0-1 = LAMP, b2-3 = MODE, b4=SEEK)
    kPortEnd2
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

  MemoryManager main_mm_;
  MemoryManager sub_mm_;
  IOBus main_bus_;
  IOBus sub_bus_;
  DeviceList devlist_;

 private:
  Z80 main_cpu_;
  Z80 sub_cpu_;

  friend class pc88core::Base;
};

inline bool PC88::IsCDSupported() {
  return devlist_.Find(DEV_ID('c', 'd', 'i', 'f')) != 0;
}

}  // namespace pc88core
