// ---------------------------------------------------------------------------
//  Scheduling class
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: schedule.h,v 1.12 2002/04/07 05:40:08 cisc Exp $

#pragma once

#include "common/device.h"

#include <queue>
#include <utility>
#include <vector>

template <class T>
class EventComparator final {
 public:
  bool operator()(const T a, const T b) const { return a->time() > b->time(); }
};

template <class T>
class EventQueue final
    : public std::priority_queue<T, std::vector<T>, EventComparator<T>> {
 public:
  EventQueue() : std::priority_queue<T, std::vector<T>, EventComparator<T>>() {}
  ~EventQueue() {}

  using parent =
      typename std::priority_queue<T, std::vector<T>, EventComparator<T>>;
  using iterator = typename std::vector<T>::iterator;

  iterator begin() { return parent::c.begin(); }
  iterator end() { return parent::c.end(); }
  const T& back() const { return parent::c.back(); }

  size_t size() const { return parent::c.size(); }
};

class SchedulerEvent final {
 public:
  SchedulerEvent() {}
  SchedulerEvent(IDevice* dev,
                 IDevice::TimeFunc func,
                 int arg,
                 SchedTime time,
                 SchedTimeDelta interval);

  SchedTimeDelta interval() const { return interval_; }
  SchedTime time() const { return time_; }

  const IDevice* dev() const { return dev_; }
  bool is_deleted() const { return deleted_; }

 private:
  friend class Scheduler;
  void set_deleted() { deleted_ = true; }

  void RunCallback() { (dev_->*func_)(arg_); }
  void UpdateTime() { time_ += interval_; }

  IDevice* dev_ = nullptr;
  IDevice::TimeFunc func_ = nullptr;
  int arg_ = 0;

  // Timestamp to fire this event.
  SchedTime time_ = 0;
  // Recurring timer.
  SchedTimeDelta interval_ = 0;

  bool deleted_ = false;
};

class SchedulerDelegate {
 public:
  virtual ~SchedulerDelegate() {}

  // Execute |ticks| time, and return executed time.
  virtual SchedTimeDelta Execute(SchedTimeDelta ticks) = 0;
  // TODO: deprecated
  virtual void Shorten(SchedTimeDelta ticks) = 0;
  // Get current VM time during Execute().
  virtual SchedTimeDelta GetTicks() = 0;
};

class Scheduler final : public IScheduler, public ITime {
 public:
  explicit Scheduler(SchedulerDelegate* delegate);
  ~Scheduler();

  // 1 SchedTime = 10microseconds
  static constexpr int kPrecisionUs = 10;
  static SchedTimeDelta SchedTimeDeltaFromMS(int ms) {
    return ms / kPrecisionUs;
  }
  static int MSFromSchedTimeDelta(SchedTimeDelta delta) {
    return delta * kPrecisionUs / 1000;
  }
  static int USFromSchedTimeDelta(SchedTimeDelta delta) {
    return delta * kPrecisionUs;
  }

  bool Init();
  SchedTimeDelta Proceed(SchedTimeDelta ticks);

  // Overrides IScheduler.
  SchedulerEvent* IFCALL AddEvent(SchedTimeDelta count,
                                  IDevice* dev,
                                  IDevice::TimeFunc func,
                                  int arg = 0,
                                  bool repeat = false) final;
  // Warning: deprecated, do not use.
  void IFCALL SetEvent(SchedulerEvent* ev,
                       SchedTimeDelta count,
                       IDevice* dev,
                       IDevice::TimeFunc func,
                       int arg = 0,
                       bool repeat = false) final;
  bool IFCALL DelEvent(IDevice* dev) final;
  bool IFCALL DelEvent(SchedulerEvent* ev) final;

  // Overrides ITime
  SchedTime IFCALL GetTime() final;

 private:
  // Drain all expired events in |quque_|.
  void DrainEvents();

  SchedulerDelegate* delegate_;

  // Current time in ticks in Schedule.
  SchedTime time_ticks_ = 0;

  // Guards against queue_.
  // std::mutex mtx_;

  // No lock required, but has to run only in emulation thread.
  EventQueue<SchedulerEvent*> queue_;
  int pool_index_ = 0;
  static constexpr int kPoolSize = 16;
  SchedulerEvent* pool_[kPoolSize];
};

inline SchedTime IFCALL Scheduler::GetTime() {
  return time_ticks_ + delegate_->GetTicks();
}
