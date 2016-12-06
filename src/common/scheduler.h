// ---------------------------------------------------------------------------
//  Scheduling class
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: schedule.h,v 1.12 2002/04/07 05:40:08 cisc Exp $

#pragma once

#include "common/device.h"

// ---------------------------------------------------------------------------

struct SchedulerEvent {
  SchedTime count;  // 時間残り
  IDevice* inst;
  IDevice::TimeFunc func;
  int arg;
  SchedTimeDelta time;  // 時間
};

class Scheduler : public IScheduler, public ITime {
 public:
  using Event = SchedulerEvent;
  enum {
    maxevents = 16,
  };

 public:
  Scheduler();
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
  virtual SchedTimeDelta Execute(SchedTimeDelta ticks) = 0;
  virtual void Shorten(int ticks) = 0;
  virtual SchedTimeDelta GetTicks() = 0;

 private:
  int evlast;  // 有効なイベントの番号の最大値
  SchedTime time;    // Scheduler 内の現在時刻
  SchedTime etime;   // Execute の終了予定時刻
  Event events[maxevents];
};

// ---------------------------------------------------------------------------

inline SchedTime IFCALL Scheduler::GetTime() {
  return time + GetTicks();
}
