// ---------------------------------------------------------------------------
//  Scheduling class
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: schedule.cpp,v 1.16 2002/04/07 05:40:08 cisc Exp $

#include "common/scheduler.h"

#include <assert.h>

// ---------------------------------------------------------------------------

Scheduler::Scheduler(SchedulerDelegate* delegate) : delegate_(delegate) {}

Scheduler::~Scheduler() {}

// ---------------------------------------------------------------------------

bool Scheduler::Init() {
  evlast = -1;

  time = 0;
  return events != 0;
}

// ---------------------------------------------------------------------------
//  時間イベントを追加
//
Scheduler::Event* IFCALL Scheduler::AddEvent(SchedTimeDelta count,
                                             IDevice* inst,
                                             IDevice::TimeFunc func,
                                             int arg,
                                             bool repeat) {
  assert(inst && func);
  assert(count > 0);

  int i = 0;
  // 空いてる Event を探す
  for (; i <= evlast; i++)
    if (!events[i].inst)
      break;
  if (i >= maxevents)
    return 0;
  if (i > evlast)
    evlast = i;

  Event& ev = events[i];
  ev.count = GetTime() + count;
  ev.inst = inst, ev.func = func, ev.arg = arg;
  ev.time = repeat ? count : 0;

  // 最短イベント発生時刻を更新する？
  if ((etime - ev.count) > 0) {
    delegate_->Shorten(etime - ev.count);
    etime = ev.count;
  }
  return &ev;
}

// ---------------------------------------------------------------------------
//  時間イベントの属性変更
//
void IFCALL Scheduler::SetEvent(Event* ev,
                                int count,
                                IDevice* inst,
                                IDevice::TimeFunc func,
                                int arg,
                                bool repeat) {
  assert(inst && func);
  assert(count > 0);

  ev->count = GetTime() + count;
  ev->inst = inst, ev->func = func, ev->arg = arg;
  ev->time = repeat ? count : 0;

  // 最短イベント発生時刻を更新する？
  if ((etime - ev->count) > 0) {
    delegate_->Shorten(etime - ev->count);
    etime = ev->count;
  }
}

// ---------------------------------------------------------------------------
//  時間イベントを削除
//
bool IFCALL Scheduler::DelEvent(IDevice* inst) {
  Event* ev = &events[evlast];
  for (int i = evlast; i >= 0; i--, ev--) {
    if (ev->inst == inst) {
      ev->inst = 0;
      if (evlast == i)
        evlast--;
    }
  }
  return true;
}

bool IFCALL Scheduler::DelEvent(Event* ev) {
  if (ev) {
    ev->inst = 0;
    if (ev - events == evlast)
      evlast--;
  }
  return true;
}

// ---------------------------------------------------------------------------
//  時間を進める
//
SchedTimeDelta Scheduler::Proceed(SchedTimeDelta ticks) {
  SchedTimeDelta t;
  for (t = ticks; t > 0;) {
    int i;
    SchedTimeDelta ptime = t;
    for (i = 0; i <= evlast; i++) {
      Event& ev = events[i];
      if (ev.inst) {
        SchedTimeDelta l = ev.count - time;
        if (l < ptime)
          ptime = l;
      }
    }

    etime = time + ptime;

    SchedTimeDelta xtime = delegate_->Execute(ptime);
    etime = time += xtime;
    t -= xtime;

    // イベントを駆動
    for (i = evlast; i >= 0; i--) {
      Event& ev = events[i];

      if (ev.inst && (ev.count - time <= 0)) {
        IDevice* inst = ev.inst;
        if (ev.time)
          ev.count += ev.time;
        else {
          ev.inst = 0;
          if (evlast == i)
            evlast--;
        }

        (inst->*ev.func)(ev.arg);
      }
    }
  }
  return ticks - t;
}
