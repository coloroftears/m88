// ---------------------------------------------------------------------------
//  Scheduling class
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: schedule.h,v 1.12 2002/04/07 05:40:08 cisc Exp $

#pragma once

#include <queue>
#include <utility>
#include <vector>

#include "common/device.h"
#include "interface/ifcommon.h"

// ---------------------------------------------------------------------------

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

  void RunCallback() { (dev_->*func_)(arg_); }
  void UpdateTime() { time_ += interval_; }

  SchedTimeDelta interval() const { return interval_; }
  SchedTime time() const { return time_; }

  const IDevice* dev() const { return dev_; }
  bool deleted() const { return deleted_; }
  void set_deleted() { deleted_ = true; }

 private:
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
  // TODO: fill description.
  virtual void Shorten(SchedTimeDelta ticks) = 0;
  // Get current VM time during Execute().
  virtual SchedTimeDelta GetTicks() = 0;
};

class Scheduler : public IScheduler, public ITime {
 public:
  using Event = SchedulerEvent;
  enum {
    kMaxEvents = 16,
  };

 public:
  explicit Scheduler(SchedulerDelegate* delegate);
  virtual ~Scheduler();

  bool Init();
  SchedTimeDelta Proceed(SchedTimeDelta ticks);
  void DrainEvents();

  // Overrides IScheduler.
  Event* IFCALL AddEvent(SchedTimeDelta count,
                         IDevice* dev,
                         IDevice::TimeFunc func,
                         int arg = 0,
                         bool repeat = false) override;
  // Warning: deprecated, do not use.
  void IFCALL SetEvent(Event* ev,
                       int count,
                       IDevice* dev,
                       IDevice::TimeFunc func,
                       int arg = 0,
                       bool repeat = false) override;
  bool IFCALL DelEvent(IDevice* dev) override;
  bool IFCALL DelEvent(Event* ev) override;

  // Overrides ITime
  SchedTime IFCALL GetTime() override;

 private:
  SchedulerDelegate* delegate_ = nullptr;

  SchedTime time_ = 0;  // Scheduler 内の現在時刻

  EventQueue<Event*> queue_;
  int pool_index_ = 0;
  Event* pool_[kMaxEvents];
};

inline SchedTime IFCALL Scheduler::GetTime() {
  return time_ + delegate_->GetTicks();
}
