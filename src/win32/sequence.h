// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: sequence.h,v 1.1 2002/04/07 05:40:10 cisc Exp $

#if !defined(win32_sequence_h)
#define win32_sequence_h

// ---------------------------------------------------------------------------

#include "types.h"
#include "critsect.h"
#include "timekeep.h"

class PC88;

// ---------------------------------------------------------------------------
//  Sequencer
//
//  VM �i�s�Ɖ�ʍX�V�̃^�C�~���O�𒲐���
//  VM ���ԂƎ����Ԃ̓������Ƃ�N���X
//
class Sequencer
{
public:
    Sequencer();
    ~Sequencer();

    bool Init(PC88* vm);
    bool Cleanup();

    long GetExecCount();
    void Activate(bool active);

    void Lock() { cs.lock(); }
    void Unlock() { cs.unlock(); }

    void SetClock(int clk);
    void SetSpeed(int spd);
    void SetRefreshTiming(uint rti);

private:
    void Execute(long clock, long length, long ec);
    void ExecuteAsynchronus();
    
    uint ThreadMain();
    static uint CALLBACK ThreadEntry(LPVOID arg);
    
    PC88* vm;

    TimeKeeper keeper;

    CriticalSection cs;
    HANDLE hthread;
    uint idthread;

    int clock;                  // 1�b�͉�tick?
    int speed;                  // 
    int execcount;
    int effclock;
    int time;

    uint skippedframe;
    uint refreshcount;
    uint refreshtiming;
    bool drawnextframe;
    
    volatile bool shouldterminate;
    volatile bool active;
};

inline void Sequencer::SetClock(int clk)
{
    clock = clk;
}

inline void Sequencer::SetSpeed(int spd)
{
    speed = spd;
}

inline void Sequencer::SetRefreshTiming(uint rti)
{
    refreshtiming = rti;
}


#endif // !defined(win32_sequence_h)
