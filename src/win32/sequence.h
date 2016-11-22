// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: sequence.h,v 1.1 2002/04/07 05:40:10 cisc Exp $

#if !defined(win32_sequence_h)
#define win32_sequence_h

// ---------------------------------------------------------------------------

#include "win32/types.h"
#include "win32/critsect.h"
#include "win32/timekeep.h"

class PC88;

// ---------------------------------------------------------------------------
//  Sequencer
//
//  VM �i�s�Ɖ�ʍX�V�̃^�C�~���O�𒲐���
//  VM ���ԂƎ����Ԃ̓������Ƃ�N���X
//
class Sequencer {
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
  void SetRefreshTiming(uint32_t rti);

 private:
  void Execute(long clock, long length, long ec);
  void ExecuteAsynchronus();

  uint32_t ThreadMain();
  static uint32_t CALLBACK ThreadEntry(LPVOID arg);

  PC88* vm;

  TimeKeeper keeper;

  CriticalSection cs;
  HANDLE hthread;
  uint32_t idthread;

  int clock;  // 1�b�͉�tick?
  int speed;  //
  int execcount;
  int effclock;
  int time;

  uint32_t skippedframe;
  uint32_t refreshcount;
  uint32_t refreshtiming;
  bool drawnextframe;

  volatile bool shouldterminate;
  volatile bool active;
};

inline void Sequencer::SetClock(int clk) {
  clock = clk;
}

inline void Sequencer::SetSpeed(int spd) {
  speed = spd;
}

inline void Sequencer::SetRefreshTiming(uint32_t rti) {
  refreshtiming = rti;
}

#endif  // !defined(win32_sequence_h)
