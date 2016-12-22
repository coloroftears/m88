// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: sequence.cpp,v 1.3 2003/05/12 22:26:35 cisc Exp $

#include "common/sequencer.h"

#include <process.h>

#include <algorithm>

#define LOGNAME "sequence"
#include "common/diag.h"

Sequencer::Sequencer() {
  keeper_.reset(TimeKeeper::create());
}

Sequencer::~Sequencer() {
  Cleanup();
}

bool Sequencer::Init(SequencerDelegate* delegate) {
  delegate_ = delegate;

  should_terminate_ = false;
  is_active_ = false;

  clock_ = 1;
  effective_clock_ = 1;
  speed_ = 100;
  exec_count_ = 0;
  time_ = 0;

  draw_next_frame_ = false;
  skipped_frames_ = 0;
  refresh_count_ = 0;
  refresh_timing_ = 1;

  if (!hthread_) {
    hthread_ = (HANDLE)_beginthreadex(
        nullptr, 0, ThreadEntry, reinterpret_cast<void*>(this), 0, &idthread_);
  }
  return !!hthread_;
}

bool Sequencer::Cleanup() {
  if (hthread_) {
    should_terminate_ = true;
    if (WAIT_TIMEOUT == WaitForSingleObject(hthread_, 3000)) {
      TerminateThread(hthread_, 0);
    }
    CloseHandle(hthread_);
    hthread_ = 0;
  }
  return true;
}

// ---------------------------------------------------------------------------
//  Core (Emulator) Thread
//
uint32_t Sequencer::ThreadMain() {
  time_ = keeper_->GetTime();
  effective_clock_ = 100;

  while (!should_terminate_) {
    if (is_active_) {
      if (clock_ <= 0)
        ExecuteBurst();
      else
        ExecuteAsynchronus();
      continue;
    }
    Sleep(20);
    time_ = keeper_->GetTime();
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  Entry point for emulation thread
//
// static
uint32_t CALLBACK Sequencer::ThreadEntry(void* arg) {
  return reinterpret_cast<Sequencer*>(arg)->ThreadMain();
}

// ---------------------------------------------------------------------------
//  CPU Main loop
//  clock   CPU clock (0.1MHz)
//  length  Execution duration (0.01ms)
//  eff     Effective clock (0.1MHz)
//
inline void Sequencer::Execute(SchedClock clk,
                               SchedTimeDelta length,
                               SchedClock eff) {
  CriticalSection::Lock lock(cs_);
  exec_count_ += clk * delegate_->Proceed(length, clk, eff);
}

// ---------------------------------------------------------------------------
//  Execute asynchronous to VSYNC signal
//
void Sequencer::ExecuteAsynchronus() {
  SchedTimeDelta texec = delegate_->GetFramePeriod();
  SchedTimeDelta twork = texec * 100 / speed_;
  delegate_->TimeSync();
  Execute(clock_, texec, clock_ * speed_ / 100);

  SchedTimeDelta tcpu = keeper_->GetTime() - time_;
  if (tcpu < twork) {
    if (draw_next_frame_ && ++refresh_count_ >= refresh_timing_) {
      delegate_->UpdateScreen();
      skipped_frames_ = 0;
      refresh_count_ = 0;
    }

    SchedTimeDelta tdraw = keeper_->GetTime() - time_;

    if (tdraw > twork) {
      draw_next_frame_ = false;
    } else {
      int it = TimeKeeper::ToMilliSeconds(twork - tdraw);
      if (it > 0)
        Sleep(it);
      draw_next_frame_ = true;
    }
    time_ += twork;
    return;
  }

  time_ += twork;
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

// ---------------------------------------------------------------------------
//  Returns CPU executed clocks since last call.
//
int32_t Sequencer::GetExecCount() {
  // Uncomment below when precise value is required.
  // CriticalSection::Lock lock(cs_);
  int32_t i = exec_count_;
  exec_count_ = 0;
  return i;
}

// ---------------------------------------------------------------------------
//  Activate CPU emulation.
//
void Sequencer::Activate(bool active) {
  CriticalSection::Lock lock(cs_);
  is_active_ = active;
}
