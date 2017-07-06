// ---------------------------------------------------------------------------
//  FM sound generator common timer module
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//  $Id: fmtimer.h,v 1.2 2003/04/22 13:12:53 cisc Exp $

#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------

namespace fmgen {

class StatusSink;
class TimerASink;

class Timer {
 public:
  virtual ~Timer() {}
  void Reset() {
    timera_count_ = 0;
    timerb_count_ = 0;
  }
  bool Count(int32_t us);
  int32_t GetNextEvent();

 protected:
  virtual void SetStatus(uint32_t bit) = 0;
  virtual void ResetStatus(uint32_t bit) = 0;

  void SetTimerBase(uint32_t clock);
  void SetTimerA(uint32_t addr, uint32_t data);
  void SetTimerB(uint32_t data);
  void SetTimerControl(uint32_t data);

  uint8_t status;
  uint8_t regtc;

  void set_status_sink(StatusSink* sink) { status_sink_ = sink; }
  void set_timera_sink(TimerASink* sink) { timera_sink_ = sink; }

 private:
  virtual void TimerA() {}

  StatusSink* status_sink_ = nullptr;
  TimerASink* timera_sink_ = nullptr;

  uint8_t regta_[2];

  int32_t timera_ = 0;
  int32_t timera_count_ = 0;

  int32_t timerb_ = 0;
  int32_t timerb_count_ = 0;

  int32_t timer_step_ = 0;
};
}  // namespace fmgen
