// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  User Interface for Win32
// ---------------------------------------------------------------------------
//  $Id: ui.h,v 1.33 2002/05/31 09:45:21 cisc Exp $

#ifndef win_ui_h
#define win_ui_h

#include "win32/types.h"
#include "win32/wincore.h"
#include "win32/windraw.h"
#include "win32/winkeyif.h"
#include "win32/88config.h"
#include "win32/wincfg.h"
#include "win32/newdisk.h"
#include "win32/monitors/soundmon.h"
#include "win32/monitors/memmon.h"
#include "win32/monitors/codemon.h"
#include "win32/monitors/basmon.h"
#include "win32/monitors/regmon.h"
#include "win32/monitors/loadmon.h"
#include "win32/monitors/iomon.h"

// ---------------------------------------------------------------------------

class WinUI {
 public:
  WinUI(HINSTANCE hinst);
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
  struct ExtNode {
    ExtNode* next;
    IWinUIExtention* ext;
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

  int AllocControlID();
  void FreeControlID(int);

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
  bool background;
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
  PC8801::WinKeyIF keyif;
  PC8801::Config config;
  PC8801::WinConfig winconfig;
  WinNewDisk newdisk;
  OPNMonitor opnmon;
  PC8801::MemoryMonitor memmon;
  PC8801::CodeMonitor codemon;
  PC8801::BasicMonitor basmon;
  PC8801::IOMonitor iomon;
  Z80RegMonitor regmon;
  LoadMonitor loadmon;
  DiskManager* diskmgr;
  TapeManager* tapemgr;

 private:
  // メッセージ関数
  uint32_t M88ChangeDisplay(HWND, WPARAM, LPARAM);
  uint32_t M88ChangeVolume(HWND, WPARAM, LPARAM);
  uint32_t M88ApplyConfig(HWND, WPARAM, LPARAM);
  uint32_t M88SendKeyState(HWND, WPARAM, LPARAM);
  uint32_t M88ClipCursor(HWND, WPARAM, LPARAM);
  uint32_t WmDropFiles(HWND, WPARAM, LPARAM);
  uint32_t WmDisplayChange(HWND, WPARAM, LPARAM);
  uint32_t WmKeyDown(HWND, WPARAM, LPARAM);
  uint32_t WmKeyUp(HWND, WPARAM, LPARAM);
  uint32_t WmSysKeyDown(HWND, WPARAM, LPARAM);
  uint32_t WmSysKeyUp(HWND, WPARAM, LPARAM);
  uint32_t WmInitMenu(HWND, WPARAM, LPARAM);
  uint32_t WmQueryNewPalette(HWND, WPARAM, LPARAM);
  uint32_t WmPaletteChanged(HWND, WPARAM, LPARAM);
  uint32_t WmActivate(HWND, WPARAM, LPARAM);
  uint32_t WmTimer(HWND, WPARAM, LPARAM);
  uint32_t WmClose(HWND, WPARAM, LPARAM);
  uint32_t WmCreate(HWND, WPARAM, LPARAM);
  uint32_t WmDestroy(HWND, WPARAM, LPARAM);
  uint32_t WmPaint(HWND, WPARAM, LPARAM);
  uint32_t WmCommand(HWND, WPARAM, LPARAM);
  uint32_t WmSize(HWND, WPARAM, LPARAM);
  uint32_t WmDrawItem(HWND, WPARAM, LPARAM);
  uint32_t WmEnterMenuLoop(HWND, WPARAM, LPARAM);
  uint32_t WmExitMenuLoop(HWND, WPARAM, LPARAM);
  uint32_t WmLButtonDown(HWND, WPARAM, LPARAM);
  uint32_t WmLButtonUp(HWND, WPARAM, LPARAM);
  uint32_t WmRButtonDown(HWND, WPARAM, LPARAM);
  uint32_t WmRButtonUp(HWND, WPARAM, LPARAM);
  uint32_t WmEnterSizeMove(HWND, WPARAM, LPARAM);
  uint32_t WmExitSizeMove(HWND, WPARAM, LPARAM);
  uint32_t WmMove(HWND, WPARAM, LPARAM);
  uint32_t WmMouseMove(HWND, WPARAM, LPARAM);
  uint32_t WmSetCursor(HWND, WPARAM, LPARAM);
};

#endif  // WIN_UI_H
