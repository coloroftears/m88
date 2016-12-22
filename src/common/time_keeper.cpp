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
    if (QueryPerformanceFrequency(&freq))
      return new TimeKeeperImplQPC(freq.QuadPart);
    return nullptr;
  }

  SchedTime TimeKeeper::GetTime() final {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    int64_t diff = count.QuadPart - base_;
    base_ = count.QuadPart;

    SchedTimeDelta delta = static_cast<SchedTimeDelta>(
        static_cast<double>(diff) * (kUnit * 1000) / freq_);
    time_ += delta;
    return time_;
  }

 private:
  TimeKeeperImplQPC(int64_t freq) : freq_(freq) {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    base_ = count.QuadPart;
  }

  int64_t freq_;
  int64_t base_ = 0;
};

class ScopedPrecisionKeeper final {
 public:
  ScopedPrecisionKeeper() { timeBeginPeriod(1); }
  ~ScopedPrecisionKeeper() { timeEndPeriod(1); }
};

// Warning: This class is not working as expected.
class TimeKeeperImplWin final : public TimeKeeper {
 public:
  ~TimeKeeperImplWin() final {}

  static TimeKeeper* create() { return new TimeKeeperImplWin(timeGetTime()); }

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
