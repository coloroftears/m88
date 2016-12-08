// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1997, 2001.
// ---------------------------------------------------------------------------
//  $Id: wincore.h,v 1.34 2003/05/15 13:15:36 cisc Exp $

#pragma once

// ---------------------------------------------------------------------------

#include <stdint.h>

#include <vector>

#include "common/critical_section.h"
#include "common/sequencer.h"
#include "interface/if_win.h"
#include "pc88/config.h"
#include "pc88/pc88.h"
#include "win32/winjoy.h"
#include "win32/winsound.h"

namespace PC8801 {
class WinKeyIF;
class ExtendModule;
}

class WinUI;

// ---------------------------------------------------------------------------

class WinCore : public PC88, public ISystem, public ILockCore {
 public:
  WinCore();
  ~WinCore();

  bool Init(WinUI* ui,
            HWND hwnd,
            Draw* draw,
            DiskManager* diskmgr,
            PC8801::WinKeyIF* keyb,
            IConfigPropBase* cpb,
            TapeManager* tapemgr);
  bool Cleanup();

  // Overrides PC88.
  void Reset();
  void ApplyConfig(PC8801::Config* config);
  bool SaveSnapshot(const char* filename);
  bool LoadSnapshot(const char* filename, const char* diskname = 0);

  PC8801::WinSound* GetSound() { return &sound; }

  int32_t GetExecCount() { return seq.GetExecCount(); }
  void Wait(bool dowait) { seq.Activate(!dowait); }

  // Overrides ISystem.
  void* IFCALL QueryIF(REFIID iid) final;

  // Overrides ILockCore.
  void IFCALL Lock() final { seq.Lock(); }
  void IFCALL Unlock() final { seq.Unlock(); }

 private:
  //  Snapshot ヘッダー
  enum {
    ssmajor = 1,
    ssminor = 1,
  };

  struct SnapshotHeader {
    char id[16];
    uint8_t major, minor;

    int8_t disk[2];
    int datasize;
    PC8801::Config::BASICMode basicmode;
    int16_t clock;
    uint16_t erambanks;
    uint16_t cpumode;
    uint16_t mainsubratio;
    uint32_t flags;
    uint32_t flag2;
  };

  class LockObj {
    WinCore* b;

   public:
    explicit LockObj(WinCore* _b) : b(_b) { b->Lock(); }
    ~LockObj() { b->Unlock(); }
  };

 private:
  bool ConnectDevices(PC8801::WinKeyIF* keyb);
  bool ConnectExternalDevices();

  WinUI* ui;
  IConfigPropBase* cfgprop;

  Sequencer seq;
  WinPadIF padif;

  using ExtendModules = std::vector<PC8801::ExtendModule*>;
  ExtendModules extmodules;

  PC8801::WinSound sound;
  PC8801::Config config;
};
