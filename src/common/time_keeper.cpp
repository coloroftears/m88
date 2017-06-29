// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: timekeep.cpp,v 1.1 2002/04/07 05:40:11 cisc Exp $

#include "common/time_keeper.h"

#include <assert.h>

#if !defined(WIN32)
#include <chrono>

class TimeKeeperChrono final : public TimeKeeper {
public:
  ~TimeKeeperChrono() final {}

  static TimeKeeper* Get() {
    return new TimeKeeperChrono();
  }

  SchedTime GetTime() {
    auto new_base = Clock::now();
    MicroSeconds diff =
        std::chrono::duration_cast<MicroSeconds>(new_base - base_);
    base_ = new_base;
    time_ += diff;

    return static_cast<uint32_t>(time_.count() / Scheduler::kPrecisionUs);
  }

 private:
  using Clock = std::chrono::high_resolution_clock;
  using TimePoint = std::chrono::time_point<Clock>;
  using MicroSeconds = std::chrono::microseconds;
  using MilliSeconds = std::chrono::milliseconds;

  TimeKeeperChrono() : time_(0) {
    base_ = Clock::now();
  }

  TimePoint base_;     // 最後の呼び出しの際の元クロックの値
  MicroSeconds time_;  // 最後の呼び出しに返した値
};

// static
TimeKeeper* TimeKeeper::Get() {
  return TimeKeeperChrono::Get();
}
#else  // WIN32

#include <windows.h>
#include <mmsystem.h>

class TimeKeeperImplQPC final : public TimeKeeper {
 public:
  ~TimeKeeperImplQPC() final {}

  static TimeKeeper* Get() {
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

  SchedTime time_ = 0;

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

  static TimeKeeper* Get() { return new TimeKeeperImplWin(timeGetTime()); }

  SchedTime TimeKeeper::GetTime() final {
    int32_t t = timeGetTime();
    int32_t diff = t - base_;
    time_ += diff / kUnit;
    base_ = t - diff % kUnit;
    return time_;
  }

 private:
  explicit TimeKeeperImplWin(int32_t base) : base_(base) {}

  SchedTime time_ = 0;

  ScopedPrecisionKeeper keeper_;
  int32_t base_;
};

// static
TimeKeeper* TimeKeeper::Get() {
  if (TimeKeeper* impl = TimeKeeperImplQPC::Get())
    return impl;
  return TimeKeeperImplWin::Get();
}
#endif  // !WIN32
