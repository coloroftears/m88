// ---------------------------------------------------------------------------
//  Scheduling class
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: schedule.cpp,v 1.16 2002/04/07 05:40:08 cisc Exp $

#include "common/scheduler.h"

#include <assert.h>

SchedulerEvent::SchedulerEvent(IDevice* dev,
                               IDevice::TimeFunc func,
                               int arg,
                               SchedTime time,
                               SchedTimeDelta interval)
    : dev_(dev), func_(func), arg_(arg), time_(time), interval_(interval) {}

Scheduler::Scheduler(SchedulerDelegate* delegate) : delegate_(delegate) {}

Scheduler::~Scheduler() {}

bool Scheduler::Init() {
  time_ = 0;
  return true;
}

SchedulerEvent* IFCALL Scheduler::AddEvent(SchedTimeDelta count,
                                           IDevice* dev,
                                           IDevice::TimeFunc func,
                                           int arg,
                                           bool repeat) {
  assert(dev && func);
  assert(count > 0);

  SchedTime earliest_event = queue_.empty() ? 0 : queue_.top()->time();

  SchedulerEvent* ev;
  if (pool_index_ > 0) {
    ev = new (pool_[--pool_index_])
         SchedulerEvent(dev, func, arg, GetTime() + count, repeat ? count : 0);
  } else {
    ev = new SchedulerEvent(dev, func, arg, GetTime() + count, repeat ?
                            count : 0);
  }
  queue_.push(ev);

  // 最短イベント発生時刻を更新する？
  if ((earliest_event - ev->time()) > 0)
    delegate_->Shorten(earliest_event - ev->time());

  return ev;
}

void IFCALL Scheduler::SetEvent(SchedulerEvent* ev,
                                int count,
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
  for (auto it = queue_.begin(); it != queue_.end(); ++it) {
    if ((*it)->dev() == dev)
      (*it)->set_deleted();
  }
  return true;
}

bool IFCALL Scheduler::DelEvent(SchedulerEvent* ev) {
  assert(ev);
  ev->set_deleted();
  return true;
}

void Scheduler::DrainEvents() {
  while (!queue_.empty()) {
    SchedulerEvent* ev = queue_.top();
    if (ev->time() > time_)
      return;

    queue_.pop();
    assert(ev);
    if (!ev->deleted()) {
      ev->RunCallback();
      if (ev->interval()) {
        ev->UpdateTime();
        queue_.push(ev);
        continue;
      }
    }

    if (pool_index_ < kMaxEvents) {
      pool_[pool_index_++] = ev;
      continue;
    }
    delete ev;
  }
}

//  時間を進める
// 1 tick = 10us
SchedTimeDelta Scheduler::Proceed(SchedTimeDelta ticks) {
  SchedTimeDelta remaining_ticks = ticks;
  while (remaining_ticks > 0) {
    SchedTimeDelta execution_ticks = remaining_ticks;
    if (!queue_.empty())
      execution_ticks = std::min(execution_ticks, queue_.top()->time() - time_);
    SchedTimeDelta executed_ticks = delegate_->Execute(execution_ticks);
    time_ += executed_ticks;
    remaining_ticks -= executed_ticks;
    DrainEvents();
  }
  return ticks - remaining_ticks;
}
