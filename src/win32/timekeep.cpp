// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: timekeep.cpp,v 1.1 2002/04/07 05:40:11 cisc Exp $

#include "win32/headers.h"
#include "win32/timekeep.h"

// ---------------------------------------------------------------------------
//  �\�z/����
//
TimeKeeper::TimeKeeper() {
  assert(unit > 0);

  LARGE_INTEGER li;
  if (QueryPerformanceFrequency(&li)) {
    freq = (li.LowPart + unit * 500) / (unit * 1000);
    QueryPerformanceCounter(&li);
    base = li.LowPart;
  } else {
    freq = 0;
    timeBeginPeriod(1);  // ���x���グ�邽�߂̂��܂��Ȃ��c�炵��
    base = timeGetTime();
  }
  time = 0;
}

TimeKeeper::~TimeKeeper() {
  if (!freq) {
    timeEndPeriod(1);
  }
}

// ---------------------------------------------------------------------------
//  ���Ԃ��擾
//
uint32_t TimeKeeper::GetTime() {
  if (freq) {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    uint32_t dc = li.LowPart - base;
    time += dc / freq;
    base = li.LowPart - dc % freq;
    return time;
  } else {
    uint32_t t = timeGetTime();
    uint32_t dc = t - base;
    time += dc / unit;
    base = t - dc % unit;
    return time;
  }
}
