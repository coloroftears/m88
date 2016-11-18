// ---------------------------------------------------------------------------
//  Scheduling class
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: schedule.h,v 1.12 2002/04/07 05:40:08 cisc Exp $

#ifndef common_schedule_h
#define common_schedule_h

#include "device.h"

// ---------------------------------------------------------------------------

struct SchedulerEvent
{
    int count;          // ���Ԏc��
    IDevice* inst;
    IDevice::TimeFunc func;
    int arg;
    int time;           // ����
};

class Scheduler : public IScheduler, public ITime
{
public:
    typedef SchedulerEvent Event;
    enum
    {
        maxevents = 16,
    };

public:
    Scheduler();
    virtual ~Scheduler();

    bool Init();
    int Proceed(int ticks);

    Event* IFCALL AddEvent(int count, IDevice* dev, IDevice::TimeFunc func, int arg=0, bool repeat=false);
    void IFCALL SetEvent(Event* ev, int count, IDevice* dev, IDevice::TimeFunc func, int arg=0, bool repeat=false);
    bool IFCALL DelEvent(IDevice* dev);
    bool IFCALL DelEvent(Event* ev);

    int IFCALL GetTime();

private:
    virtual int Execute(int ticks) = 0;
    virtual void Shorten(int ticks) = 0;
    virtual int GetTicks() = 0;

private:
    int evlast;             // �L���ȃC�x���g�̔ԍ��̍ő�l
    int time;               // Scheduler ���̌��ݎ���
    int etime;              // Execute �̏I���\�莞��
    Event events[maxevents];
};

// ---------------------------------------------------------------------------

inline int IFCALL Scheduler::GetTime()
{
    return time + GetTicks();
}


#endif // common_schedule_h
