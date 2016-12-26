// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  User Interface for Win32
// ---------------------------------------------------------------------------
//  $Id: ui.h,v 1.33 2002/05/31 09:45:21 cisc Exp $

#pragma once

#include <stdint.h>

#include <memory>

#include "interface/if_win.h"
#include "win32/88config.h"
#include "win32/monitors/basmon.h"
#include "win32/monitors/codemon.h"
#include "win32/monitors/iomon.h"
#include "win32/monitors/loadmon.h"
#include "win32/monitors/memmon.h"
#include "win32/monitors/regmon.h"
#include "win32/monitors/soundmon.h"
#include "win32/newdisk.h"
#include "win32/wincfg.h"
#include "win32/wincore.h"
#include "win32/windraw.h"
#include "win32/winkeyif.h"

// ---------------------------------------------------------------------------

class WinUI {
 public:
  explicit WinUI(HINSTANCE hinst);
  ~WinUI();

  bool InitWindow(int nwinmode);
  int Main(const char* cmdline);
  uint32_t GetMouseButton() { return mousebutton; }
  HWND GetHWnd() { return hwnd; }

 private:
  struct DiskInfo {
    HMENU hmenu;
    int currentdisk;
    int idchgdisk;
    bool readonly;
    char filename[MAX_PATH];
  };

 private:
  bool InitM88(const char* cmdline);
  void CleanupM88();
  LRESULT WinProc(HWND, UINT, WPARAM, LPARAM);
  static LRESULT CALLBACK WinProcGate(HWND, UINT, WPARAM, LPARAM);
  void ReportError();
  void Reset();

  void ApplyConfig();
  void ApplyCommandLine(const char* cmdline);

  void ToggleDisplayMode();
  void ChangeDisplayType(bool savepos);

  void ChangeDiskImage(HWND hwnd, int drive);
  bool OpenDiskImage(int drive,
                     const char* filename,
                     bool readonly,
                     int id,
                     bool create);
  void OpenDiskImage(const char* filename);
  bool SelectDisk(uint32_t drive, int id, bool menuonly);
  bool CreateDiskMenu(uint32_t drive);

  void OpenTape();

  void ShowStatusWindow();
  void ResizeWindow(uint32_t width, uint32_t height);
  void SetGUIFlag(bool);

  void SaveSnapshot(int n);
  void LoadSnapshot(int n);
  void GetSnapshotName(char*, int);
  bool MakeSnapshotMenu();

  void CaptureScreen();

  // ウインドウ関係
  HWND hwnd;
  HINSTANCE hinst;
  HACCEL accel;
  HMENU hmenudbg;

  // 状態表示用
  UINT timerid;
  bool report;
  volatile bool active;

  // ウインドウの状態
  bool background_ = true;
  bool fullscreen;
  uint32_t displaychangedtime;
  uint32_t resetwindowsize;
  DWORD wstyle;
  POINT point;
  int clipmode;
  bool guimodebymouse;

  // disk
  DiskInfo diskinfo[2];
  char tapetitle[MAX_PATH];

  // snapshot 関係
  HMENU hmenuss[2];
  int currentsnapshot;
  bool snapshotchanged;

  // PC88
  bool capturemouse;
  uint32_t mousebutton;

  WinCore core;
  WinDraw draw;
  pc88::WinKeyIF keyif;
  pc88::Config config;
  pc88::WinConfig winconfig;
  WinNewDisk newdisk;
  OPNMonitor opnmon;
  pc88::MemoryMonitor memmon;
  pc88::CodeMonitor codemon;
  pc88::BasicMonitor basmon;
  pc88::IOMonitor iomon;
  Z80RegMonitor regmon;
  LoadMonitor loadmon;

  std::unique_ptr<DiskManager> diskmgr;
  std::unique_ptr<TapeManager> tapemgr;

 private:
  // メッセージ関数
  uint32_t OnM88ChangeDisplay(HWND, WPARAM, LPARAM);
  uint32_t OnM88ChangeVolume(HWND, WPARAM, LPARAM);
  uint32_t OnM88ApplyConfig(HWND, WPARAM, LPARAM);
  uint32_t OnM88SendKeyState(HWND, WPARAM, LPARAM);
  uint32_t OnM88ClipCursor(HWND, WPARAM, LPARAM);
  uint32_t OnDropFiles(HWND, WPARAM, LPARAM);
  uint32_t OnDisplayChange(HWND, WPARAM, LPARAM);
  uint32_t OnKeyDown(HWND, WPARAM, LPARAM);
  uint32_t OnKeyUp(HWND, WPARAM, LPARAM);
  uint32_t OnSysKeyDown(HWND, WPARAM, LPARAM);
  uint32_t OnSysKeyUp(HWND, WPARAM, LPARAM);
  uint32_t OnInitMenu(HWND, WPARAM, LPARAM);
  uint32_t OnQueryNewPalette(HWND, WPARAM, LPARAM);
  uint32_t OnPaletteChanged(HWND, WPARAM, LPARAM);
  uint32_t OnActivate(HWND, WPARAM, LPARAM);
  uint32_t OnTimer(HWND, WPARAM, LPARAM);
  uint32_t OnClose(HWND, WPARAM, LPARAM);
  uint32_t OnCreate(HWND, WPARAM, LPARAM);
  uint32_t OnDestroy(HWND, WPARAM, LPARAM);
  uint32_t OnPaint(HWND, WPARAM, LPARAM);
  uint32_t OnCommand(HWND, WPARAM, LPARAM);
  uint32_t OnSize(HWND, WPARAM, LPARAM);
  uint32_t OnDrawItem(HWND, WPARAM, LPARAM);
  uint32_t OnEnterMenuLoop(HWND, WPARAM, LPARAM);
  uint32_t OnExitMenuLoop(HWND, WPARAM, LPARAM);
  uint32_t OnLButtonDown(HWND, WPARAM, LPARAM);
  uint32_t OnLButtonUp(HWND, WPARAM, LPARAM);
  uint32_t OnRButtonDown(HWND, WPARAM, LPARAM);
  uint32_t OnRButtonUp(HWND, WPARAM, LPARAM);
  uint32_t OnEnterSizeMove(HWND, WPARAM, LPARAM);
  uint32_t OnExitSizeMove(HWND, WPARAM, LPARAM);
  uint32_t OnMove(HWND, WPARAM, LPARAM);
  uint32_t OnMouseMove(HWND, WPARAM, LPARAM);
  uint32_t OnSetCursor(HWND, WPARAM, LPARAM);
};
