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

class StatusImpl {
 public:
  StatusImpl();
  ~StatusImpl();

  bool Init(HWND hwndparent);
  void Cleanup();

  bool Enable(bool sfs = false);
  bool Disable();
  int GetHeight() const { return height; }
  void DrawItem(DRAWITEMSTRUCT* dis);

  void Update();
  uint32_t GetTimerID() { return timerid; }

  HWND GetHWnd() { return hwnd; }

  bool Show(int priority, int duration, const char* msg, va_list args);
  void FDAccess(uint32_t dr, bool hd, bool active);
  void UpdateDisplay();
  void WaitSubSys() { litstat[2] = 9; }

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

  HWND hwnd = 0;
  HWND hwndparent = 0;
  List* list = nullptr;
  uint32_t timerid = 0;
  CriticalSection cs;
  Border border;
  int height = 0;
  int litstat[3];
  int litcurrent[3];
  bool showfdstat = false;
  bool updatemessage = false;

  int currentduration = 0;
  int currentpriority = 0;

  char buf[128];
};
//
//extern StatusDisplay statusdisplay;
