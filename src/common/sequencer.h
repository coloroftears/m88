// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: sequence.h,v 1.1 2002/04/07 05:40:10 cisc Exp $

#pragma once

// ---------------------------------------------------------------------------

#include "common/scheduler.h"

#include <stdint.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class TimeKeeper;

// Interface to be implemented by Sequencer class user.
class SequencerDelegate {
 public:
  SequencerDelegate() {}
  virtual ~SequencerDelegate() {}

  virtual SchedTimeDelta Proceed(SchedTimeDelta ticks,
                                 SchedClock clock,
                                 SchedClock effective_clock) = 0;
  virtual void TimeSync() = 0;
  virtual void UpdateScreen(bool refresh = false) = 0;
  virtual SchedTimeDelta GetFramePeriod() const = 0;
};

// ---------------------------------------------------------------------------
//  Sequencer
//
//  Coordinate progress of CPU emulation and display updates.
//  Also responsible for synchronizing emulation time to real time.
//
class Sequencer final {
 public:
  Sequencer();
  ~Sequencer();

  bool Init(SequencerDelegate* delegate);
  bool Cleanup();

  int32_t GetExecCount();
  void Activate(bool active);

  void Lock() { mtx_.lock(); }
  void Unlock() { mtx_.unlock(); }

  void SetClock(SchedClock clock) { clock_ = clock; }
  void SetSpeed(int speed) { speed_ = speed; }
  // Set redraw rate (1 / rti).
  void SetRefreshTiming(uint32_t rti) { refresh_timing_ = rti; }

 private:
  void Execute(SchedClock clock,
               SchedTimeDelta length,
               SchedClock effective_clock);
  void ExecuteAsynchronous();
  void ExecuteBurst();
  void ThreadMain();

  std::unique_ptr<TimeKeeper> keeper_;
  SequencerDelegate* delegate_ = nullptr;

  std::thread vm_thread_;
  std::mutex mtx_;
  std::condition_variable cv_;

  // CPU clocks in 1 tick. If negative, run in burst mode.
  SchedClock clock_ = 1;
  SchedClock effective_clock_ = 1;
  // Speed ratio to specified clock (in %, 100 = 1.0x).
  int speed_ = 100;
  // CPU executed clocks since last GetExecCount() call.
  uint32_t execcount_ = 0;
  SchedTime time_ = 0;

  // Bookkeeping statistics for drawing frames.
  uint32_t skipped_frames_ = 0;
  uint32_t refresh_count_ = 1;
  uint32_t refresh_timing_ = 0;
  bool draw_next_frame_ = false;

  std::atomic<bool> should_terminate_;
  std::atomic<bool> is_active_;
};
