// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: sequence.cpp,v 1.3 2003/05/12 22:26:35 cisc Exp $

#include "common/sequencer.h"

#include "common/time_keeper.h"

#include <algorithm>
#include <chrono>
#include <utility>

#define LOGNAME "sequence"
#include "common/diag.h"

Sequencer::Sequencer() : should_terminate_(false), is_active_(false) {
  keeper_.reset(TimeKeeper::Get());
}

Sequencer::~Sequencer() {
  // Cleanup();
}

bool Sequencer::Init(SequencerDelegate* delegate) {
  delegate_ = delegate;

  should_terminate_ = false;
  is_active_ = false;

  execcount_ = 0;
  clock_ = 1;

  draw_next_frame_ = false;
  skipped_frames_ = 0;
  refresh_timing_ = 1;
  refresh_count_ = 0;

  vm_thread_ = std::thread(&Sequencer::ThreadMain, this);
  return vm_thread_.joinable();
}

bool Sequencer::Cleanup() {
  if (!vm_thread_.joinable())
    return true;
  should_terminate_ = true;
  cv_.notify_one();
  vm_thread_.join();
  return true;
}

// Core (Emulator) Thread
void Sequencer::ThreadMain() {
  time_ = keeper_->GetTime();

  while (!should_terminate_) {
    if (!is_active_) {
      std::unique_lock<std::mutex> lock(mtx_);
      cv_.wait_for(lock, std::chrono::milliseconds(20));
      time_ = keeper_->GetTime();
      continue;
    }

    if (clock_ > 0) {
      ExecuteAsynchronous();
    } else {
      ExecuteBurst();
    }
  }
}

// CPU main loop
//  clock   CPU clock (0.1MHz)
//  length  Execution duration (1tick = 10us)
//  eff     Effective clock (0.1MHz)
inline void Sequencer::Execute(SchedClock clock,
                               SchedTimeDelta length,
                               SchedClock effective_clock) {
  execcount_ += clock * delegate_->Proceed(length, clock, effective_clock);
}

// Execute asynchronous to VSYNC signal
void Sequencer::ExecuteAsynchronous() {
  std::unique_lock<std::mutex> lock(mtx_);

  SchedTimeDelta texec = delegate_->GetFramePeriod();
  delegate_->TimeSync();
  Execute(clock_, texec, clock_);

  SchedTimeDelta tcpu = keeper_->GetTime() - time_;
  if (tcpu < texec) {
    if (draw_next_frame_ && ++refresh_count_ >= refresh_timing_) {
      delegate_->UpdateScreen();
      skipped_frames_ = 0;
      refresh_count_ = 0;
    }

    SchedTimeDelta tdraw = keeper_->GetTime() - time_;

    if (tdraw > texec) {
      draw_next_frame_ = false;
    } else {
      int it = Scheduler::MSFromSchedTimeDelta(texec - tdraw);
      if (it > 0)
        cv_.wait_for(lock, std::chrono::milliseconds(it));
      draw_next_frame_ = true;
    }
    time_ += texec;
    return;
  }

  time_ += texec;
  if (++skipped_frames_ >= 20) {
    delegate_->UpdateScreen();
    skipped_frames_ = 0;
    time_ = keeper_->GetTime();
  }
}

void Sequencer::ExecuteBurst() {
  time_ = keeper_->GetTime();
  delegate_->TimeSync();
  SchedTimeDelta ticks = 0;
  int eclk = 0;
  do {
    if (clock_)
      Execute(-clock_, 500, effective_clock_);
    else
      Execute(effective_clock_, 500 * speed_ / 100, effective_clock_);
    eclk += 5;
    ticks = keeper_->GetTime() - time_;
  } while (ticks < 1000);
  delegate_->UpdateScreen();

  effective_clock_ = std::min(
      (std::min(1000, eclk) * effective_clock_ * 100 / ticks) + 1, 10000);
}

// Returns CPU executed clocks since last call.
int32_t Sequencer::GetExecCount() {
  // Uncomment below when precise value is required.
  // std::lock_guard<std::mutex> lock(mtx_);
  uint32_t i = execcount_;
  execcount_ = 0;
  return i;
}

// Activate CPU emulation.
void Sequencer::Activate(bool active) {
  is_active_ = active;
}
