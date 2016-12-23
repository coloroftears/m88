// ---------------------------------------------------------------------------
//  FM sound generator common timer module
//  Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//  $Id: fmtimer.h,v 1.2 2003/04/22 13:12:53 cisc Exp $

#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------

namespace fmgen {
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

 private:
  virtual void TimerA() {}
  uint8_t regta_[2];

  int32_t timera_;
  int32_t timera_count_;

  int32_t timerb_;
  int32_t timerb_count_;

  int32_t timer_step_;
};
}  // namespace fmgen
