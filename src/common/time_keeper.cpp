// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: timekeep.cpp,v 1.1 2002/04/07 05:40:11 cisc Exp $

#include "common/time_keeper.h"

#include <windows.h>

#include <assert.h>
#include <mmsystem.h>

class TimeKeeperImplQPC final : public TimeKeeper {
 public:
  ~TimeKeeperImplQPC() final {}

  static TimeKeeper* create() {
    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency(&freq)) {
      int64_t clocks_per_unit = (freq.QuadPart + kUnit * 500) / (kUnit * 1000);
      LARGE_INTEGER count;
      QueryPerformanceCounter(&count);
      return new TimeKeeperImplQPC(clocks_per_unit, count.QuadPart);
    }
    return nullptr;
  }

  SchedTime TimeKeeper::GetTime() final {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    int64_t diff = count.QuadPart - base_;
    time_ += static_cast<SchedTime>(diff / freq_);
    base_ = count.QuadPart - (diff % freq_);
    return time_;
  }

 private:
  TimeKeeperImplQPC(int64_t freq, uint64_t base)
      : freq_(freq), base_(base) {}

  int64_t freq_ = 0;
  int64_t base_ = 0;
};

class ScopedPrecisionKeeper {
 public:
  ScopedPrecisionKeeper() {
    // 精度を上げるためのおまじない…らしい
    timeBeginPeriod(1);
  }
  ~ScopedPrecisionKeeper() {
    timeEndPeriod(1);
  }
};

class TimeKeeperImplWin final : public TimeKeeper {
 public:
  ~TimeKeeperImplWin() final {}

  static TimeKeeper* create() {
    return new TimeKeeperImplWin(timeGetTime());
  }

  SchedTime TimeKeeper::GetTime() final {
    int32_t t = timeGetTime();
    int32_t diff = t - base_;
    time_ += diff / kUnit;
    base_ = t - diff % kUnit;
    return time_;
  }

 private:
  explicit TimeKeeperImplWin(int32_t base) : base_(base) {}

  ScopedPrecisionKeeper keeper_;
  int32_t base_;
};

// static
TimeKeeper* TimeKeeper::create() {
  if (TimeKeeper* impl = TimeKeeperImplQPC::create())
    return impl;
  return TimeKeeperImplWin::create();
}
