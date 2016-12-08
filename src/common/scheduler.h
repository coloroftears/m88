// ---------------------------------------------------------------------------
//  Scheduling class
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: schedule.h,v 1.12 2002/04/07 05:40:08 cisc Exp $

#pragma once

#include "common/device.h"
#include "interface/ifcommon.h"

// ---------------------------------------------------------------------------

struct SchedulerEvent {
  SchedTime count;  // 時間残り
  IDevice* inst;
  IDevice::TimeFunc func;
  int arg;
  SchedTimeDelta time;  // 時間
};

class SchedulerDelegate {
 public:
  virtual ~SchedulerDelegate() {}

  // Execute |ticks| time, and return executed time.
  virtual SchedTimeDelta Execute(SchedTimeDelta ticks) = 0;
  // TODO: Fill description.
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

  // Overrides IScheduler.
  Event* IFCALL AddEvent(SchedTimeDelta count,
                         IDevice* dev,
                         IDevice::TimeFunc func,
                         int arg = 0,
                         bool repeat = false) override;
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

  int evlast = 0;       // 有効なイベントの番号の最大値
  SchedTime time = 0;   // Scheduler 内の現在時刻
  SchedTime etime = 0;  // Execute の終了予定時刻
  Event events[kMaxEvents];
};

// ---------------------------------------------------------------------------

inline SchedTime IFCALL Scheduler::GetTime() {
  return time + delegate_->GetTicks();
}
