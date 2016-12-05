// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: status.h,v 1.6 2002/04/07 05:40:10 cisc Exp $

#pragma once

#include <stdarg.h>
#include <stdint.h>

#include "common/critical_section.h"
#include "common/toast.h"

class StatusDisplay {
 public:
  StatusDisplay();
  ~StatusDisplay();

  bool Init(HWND hwndparent);
  void Cleanup();

  bool Enable(bool sfs = false);
  bool Disable();
  int GetHeight() { return height; }
  void DrawItem(DRAWITEMSTRUCT* dis);
  void FDAccess(uint32_t dr, bool hd, bool active);
  void UpdateDisplay();
  void WaitSubSys() { litstat[2] = 9; }

  bool Show(int priority, int duration, const char* msg, va_list args);
  void Update();
  uint32_t GetTimerID() { return timerid; }

  HWND GetHWnd() { return hwnd; }

 private:
  struct List {
    List* next;
    int priority;
    int duration;
    char msg[127];
    bool clear;
  };
  struct Border {
    int horizontal;
    int vertical;
    int split;
  };

  void Clean();

  HWND hwnd;
  HWND hwndparent;
  List* list;
  uint32_t timerid;
  CriticalSection cs;
  Border border;
  int height;
  int litstat[3];
  int litcurrent[3];
  bool showfdstat;
  bool updatemessage;

  int currentduration;
  int currentpriority;

  char buf[128];
};

extern StatusDisplay statusdisplay;
