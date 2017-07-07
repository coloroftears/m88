// ---------------------------------------------------------------------------
//  Scheduling class
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: schedule.cpp,v 1.16 2002/04/07 05:40:08 cisc Exp $

#include "common/scheduler.h"

#include <assert.h>
#include <algorithm>

SchedulerEvent::SchedulerEvent(IDevice* dev,
                               IDevice::TimeFunc func,
                               int arg,
                               SchedTime time,
                               SchedTimeDelta interval)
    : dev_(dev), func_(func), arg_(arg), time_(time), interval_(interval) {}

Scheduler::Scheduler(SchedulerDelegate* delegate) : delegate_(delegate) {}

Scheduler::~Scheduler() {}

bool Scheduler::Init() {
  time_ticks_ = 0;
  return true;
}

SchedulerEvent* IFCALL Scheduler::AddEvent(SchedTimeDelta count,
                                           IDevice* dev,
                                           IDevice::TimeFunc func,
                                           int arg,
                                           bool repeat) {
  assert(dev && func);
  assert(count > 0);

  SchedulerEvent* ev;
  if (pool_index_ > 0) {
    // std::lock_guard<std::mutex> lock(mtx_);
    ev = new (pool_[--pool_index_])
        SchedulerEvent(dev, func, arg, GetTime() + count, repeat ? count : 0);
  } else {
    ev = new SchedulerEvent(dev, func, arg, GetTime() + count,
                            repeat ? count : 0);
  }
  // std::lock_guard<std::mutex> lock(mtx_);
  queue_.push(ev);
  return ev;
}

void IFCALL Scheduler::SetEvent(SchedulerEvent* ev,
                                SchedTimeDelta count,
                                IDevice* dev,
                                IDevice::TimeFunc func,
                                int arg,
                                bool repeat) {
  assert(dev && func);
  assert(count > 0);

  DelEvent(ev);
  AddEvent(count, dev, func, arg, repeat);

  // This interface assumes that original pointer ev is preserved and reusable,
  // which is false.  Therefore not recommended.
}

bool IFCALL Scheduler::DelEvent(IDevice* dev) {
  // std::lock_guard<std::mutex> lock(mtx_);
  for (auto it = queue_.begin(); it != queue_.end(); ++it) {
    if ((*it)->dev_ == dev)
      (*it)->set_deleted();
  }
  return true;
}

bool IFCALL Scheduler::DelEvent(SchedulerEvent* ev) {
  // std::lock_guard<std::mutex> lock(mtx_);
  assert(ev);
  ev->set_deleted();
  return true;
}

void Scheduler::DrainEvents() {
  while (!queue_.empty()) {
    SchedulerEvent* ev = queue_.top();
    if (ev->time() > time_ticks_)
      return;

    queue_.pop();
    assert(ev);
    if (!ev->is_deleted()) {
      ev->RunCallback();
      if (ev->interval()) {
        ev->UpdateTime();
        queue_.push(ev);
        continue;
      }
    }

    if (pool_index_ < kPoolSize) {
      pool_[pool_index_++] = ev;
      continue;
    }
    delete ev;
  }
}

// 1 tick = 10us
SchedTimeDelta Scheduler::Proceed(SchedTimeDelta ticks) {
  SchedTimeDelta remaining_ticks = ticks;
  while (remaining_ticks > 0) {
    SchedTimeDelta execution_ticks = remaining_ticks;
    if (!queue_.empty())
      execution_ticks =
          std::min(execution_ticks, queue_.top()->time() - time_ticks_);
    SchedTimeDelta executed_ticks = delegate_->Execute(execution_ticks);
    time_ticks_ += executed_ticks;
    remaining_ticks -= executed_ticks;
    DrainEvents();
  }
  return ticks - remaining_ticks;
}
