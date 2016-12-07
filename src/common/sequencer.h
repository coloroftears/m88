// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: sequence.h,v 1.1 2002/04/07 05:40:10 cisc Exp $

#pragma once

// ---------------------------------------------------------------------------

#include <stdint.h>

#include "common/critical_section.h"
#include "common/time_keeper.h"
#include "interface/ifcommon.h"

class SequencerDelegate {
 public:
  SequencerDelegate() {}
  virtual ~SequencerDelegate() {}

  virtual SchedTimeDelta Proceed(SchedTimeDelta ticks, SchedClock clock, uint32_t ecl) = 0;
  virtual void TimeSync() = 0;
  virtual void UpdateScreen(bool refresh = false) = 0;
  virtual SchedTimeDelta GetFramePeriod() const = 0;
};

// ---------------------------------------------------------------------------
//  Sequencer
//
//  VM 進行と画面更新のタイミングを調整し
//  VM 時間と実時間の同期をとるクラス
//
class Sequencer {
 public:
  Sequencer();
  ~Sequencer();

  bool Init(SequencerDelegate* delegate);
  bool Cleanup();

  int32_t GetExecCount();
  void Activate(bool active);

  void Lock() { cs.lock(); }
  void Unlock() { cs.unlock(); }

  void SetClock(SchedClock clk);
  void SetSpeed(int spd);
  void SetRefreshTiming(uint32_t rti);

 private:
  void Execute(SchedClock clock, SchedTimeDelta length, int32_t ec);
  void ExecuteAsynchronus();

  uint32_t ThreadMain();
  static uint32_t CALLBACK ThreadEntry(LPVOID arg);

  SequencerDelegate* delegate_ = nullptr;

  TimeKeeper keeper;

  CriticalSection cs;
  HANDLE hthread = 0;
  uint32_t idthread = 0;

  SchedClock clock = 1;  // 1秒は何tick?
  int speed = 100;  //
  int32_t execcount = 0;
  SchedClock effclock = 1;
  SchedTime time = 0;

  uint32_t skippedframe = 0;
  uint32_t refreshcount = 1;
  uint32_t refreshtiming = 0;
  bool drawnextframe = false;

  volatile bool shouldterminate = false;
  volatile bool active = false;
};

inline void Sequencer::SetClock(SchedClock clk) {
  clock = clk;
}

inline void Sequencer::SetSpeed(int spd) {
  speed = spd;
}

inline void Sequencer::SetRefreshTiming(uint32_t rti) {
  refreshtiming = rti;
}
