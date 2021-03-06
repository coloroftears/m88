// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: main.cpp,v 1.9 2001/02/21 11:58:55 cisc Exp $

#include <windows.h>

#include <objbase.h>
#include <stdio.h>
#include <memory>

#include "win32/ui.h"
#include "common/file.h"

// ---------------------------------------------------------------------------

char m88dir[MAX_PATH];
char m88ini[MAX_PATH];

// ---------------------------------------------------------------------------
//  InitPathInfo
//
static void InitPathInfo() {
  char buf[MAX_PATH];
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char fname[_MAX_FNAME];
  char ext[_MAX_EXT];

  GetModuleFileName(0, buf, MAX_PATH);
  _splitpath(buf, drive, dir, fname, ext);
  sprintf(m88ini, "%s%s%s.ini", drive, dir, fname);
  sprintf(m88dir, "%s%s", drive, dir);
}

// ---------------------------------------------------------------------------
//  WinMain
//
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR cmdline, int nwinmode) {
  if (FAILED(CoInitialize(nullptr)))
    return -1;

  InitPathInfo();

  INITCOMMONCONTROLSEX init_ctrls;
  init_ctrls.dwSize = sizeof(init_ctrls);
  init_ctrls.dwICC = ICC_BAR_CLASSES;
  InitCommonControlsEx(&init_ctrls);

  int r = -1;
  {
    std::unique_ptr<m88win::WinUI> ui(new m88win::WinUI(hinst));
    if (ui && ui->InitWindow(nwinmode))
      r = ui->Main(cmdline);
  }

  CoUninitialize();
  return r;
}
