// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: sequence.cpp,v 1.3 2003/05/12 22:26:35 cisc Exp $

#include "win32/headers.h"
#include "win32/sequence.h"
#include "pc88/pc88.h"
#include "common/misc.h"

#define LOGNAME "sequence"
#include "win32/diag.h"

// ---------------------------------------------------------------------------
//  �\�z/����
//
Sequencer::Sequencer() : hthread(0), execcount(0), vm(0) {}

Sequencer::~Sequencer() {
  Cleanup();
}

// ---------------------------------------------------------------------------
//  ������
//
bool Sequencer::Init(PC88* _vm) {
  vm = _vm;

  active = false;
  shouldterminate = false;
  execcount = 0;
  clock = 1;
  speed = 100;

  drawnextframe = false;
  skippedframe = 0;
  refreshtiming = 1;
  refreshcount = 0;

  if (!hthread) {
    hthread = (HANDLE)_beginthreadex(
        NULL, 0, ThreadEntry, reinterpret_cast<void*>(this), 0, &idthread);
  }
  return !!hthread;
}

// ---------------------------------------------------------------------------
//  ��n��
//
bool Sequencer::Cleanup() {
  if (hthread) {
    shouldterminate = true;
    if (WAIT_TIMEOUT == WaitForSingleObject(hthread, 3000)) {
      TerminateThread(hthread, 0);
    }
    CloseHandle(hthread);
    hthread = 0;
  }
  return true;
}

// ---------------------------------------------------------------------------
//  Core Thread
//
uint32_t Sequencer::ThreadMain() {
  time = keeper.GetTime();
  effclock = 100;

  while (!shouldterminate) {
    if (active) {
      ExecuteAsynchronus();
    } else {
      Sleep(20);
      time = keeper.GetTime();
    }
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  �T�u�X���b�h�J�n�_
//
uint32_t CALLBACK Sequencer::ThreadEntry(void* arg) {
  return reinterpret_cast<Sequencer*>(arg)->ThreadMain();
}

// ---------------------------------------------------------------------------
//  �b�o�t���C�����[�v
//  clock   �b�o�t�̃N���b�N(0.1MHz)
//  length  ���s���鎞�� (0.01ms)
//  eff     �����N���b�N
//
inline void Sequencer::Execute(long clk, long length, long eff) {
  CriticalSection::Lock lock(cs);
  execcount += clk * vm->Proceed(length, clk, eff);
}

// ---------------------------------------------------------------------------
//  VSYNC �񓯊�
//
void Sequencer::ExecuteAsynchronus() {
  if (clock <= 0) {
    time = keeper.GetTime();
    vm->TimeSync();
    DWORD ms;
    int eclk = 0;
    do {
      if (clock)
        Execute(-clock, 500, effclock);
      else
        Execute(effclock, 500 * speed / 100, effclock);
      eclk += 5;
      ms = keeper.GetTime() - time;
    } while (ms < 1000);
    vm->UpdateScreen();

    effclock = Min((Min(1000, eclk) * effclock * 100 / ms) + 1, 10000);
  } else {
    int texec = vm->GetFramePeriod();
    int twork = texec * 100 / speed;
    vm->TimeSync();
    Execute(clock, texec, clock * speed / 100);

    int32_t tcpu = keeper.GetTime() - time;
    if (tcpu < twork) {
      if (drawnextframe && ++refreshcount >= refreshtiming) {
        vm->UpdateScreen();
        skippedframe = 0;
        refreshcount = 0;
      }

      int32_t tdraw = keeper.GetTime() - time;

      if (tdraw > twork) {
        drawnextframe = false;
      } else {
        int it = (twork - tdraw) / 100;
        if (it > 0)
          Sleep(it);
        drawnextframe = true;
      }
      time += twork;
    } else {
      time += twork;
      if (++skippedframe >= 20) {
        vm->UpdateScreen();
        skippedframe = 0;
        time = keeper.GetTime();
      }
    }
  }
}

// ---------------------------------------------------------------------------
//  ���s�N���b�N�J�E���g�̒l��Ԃ��A�J�E���^�����Z�b�g
//
long Sequencer::GetExecCount() {
  //  CriticalSection::Lock lock(cs); // ���m�Ȓl���K�v�ȂƂ��͗L���ɂ���

  int i = execcount;
  execcount = 0;
  return i;
}

// ---------------------------------------------------------------------------
//  ���s����
//
void Sequencer::Activate(bool a) {
  CriticalSection::Lock lock(cs);
  active = a;
}
