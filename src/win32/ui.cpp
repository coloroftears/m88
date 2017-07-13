// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  User Interface for Win32
// ---------------------------------------------------------------------------
//  $Id: ui.cpp,v 1.62 2003/09/28 14:35:35 cisc Exp $

#include "win32/ui.h"

#include <mbstring.h>
#include <shellapi.h>

#include <algorithm>
#include <memory>

#include "common/clamp.h"
#include "common/error.h"
#include "common/file.h"
#include "common/status.h"
#include "common/toast.h"
#include "pc88/disk_manager.h"
#include "pc88/opn_interface.h"
#include "pc88/tape_manager.h"
#include "win32/88config.h"
#include "win32/about.h"
#include "win32/file_finder.h"
#include "win32/filetest.h"
#include "win32/messages.h"
#include "win32/resource.h"
#include "win32/status_impl.h"
#include "win32/winexapi.h"

#define LOGNAME "ui"
#include "common/diag.h"

extern char m88dir[MAX_PATH];
extern char m88ini[MAX_PATH];

namespace m88win {

// TODO: Remove this
WinUI* g_ui;

// ---------------------------------------------------------------------------
//  WinUI
//  生成・破棄
//
WinUI::WinUI(HINSTANCE hinstance) : hinst(hinstance) {
  g_ui = this;
  timerid = 0;
  point.x = point.y = 0;
  //  resizewindow = 0;
  displaychangedtime = GetTickCount();
  report = true;

  diskinfo[0].hmenu = 0;
  diskinfo[0].filename[0] = 0;
  diskinfo[0].idchgdisk = IDM_CHANGEDISK_1;
  diskinfo[1].hmenu = 0;
  diskinfo[1].filename[0] = 0;
  diskinfo[1].idchgdisk = IDM_CHANGEDISK_2;
  capturemouse = true;
  resetwindowsize = 0;
  hmenuss[0] = 0;
  hmenuss[1] = 0;
  currentsnapshot = 0;
  snapshotchanged = true;

  clipmode = 0;
}

WinUI::~WinUI() {}

// ---------------------------------------------------------------------------
//  WinUI::InitM88
//  M88 の初期化
//
bool WinUI::InitM88(const char* cmdline) {
  active = false;
  tapetitle[0] = 0;

  //  設定よみこみ
  Log("%d\tLoadConfig\n", timeGetTime());
  LoadConfig(&config, m88ini, true);

  //  現在の path 保存
  char path[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, path);

  //  デバイスの初期化
  LoadConfigDirectory(&config, m88ini, "BIOSPath", true);

  Log("%d\tdiskmanager\n", timeGetTime());
  if (!diskmgr)
    diskmgr.reset(new pc88core::DiskManager);
  if (!diskmgr || !diskmgr->Init())
    return false;
  if (!tapemgr)
    tapemgr.reset(new pc88core::TapeManager);
  if (!tapemgr)
    return false;

  Log("%d\tkeyboard if\n", timeGetTime());
  if (!keyif.Init(hwnd))
    return false;
  Log("%d\tcore\n", timeGetTime());
  if (!core.Init(this, hwnd, &draw, diskmgr.get(), &keyif, &winconfig,
                 tapemgr.get()))
    return false;

  // Initialize monitor classes.
  Log("%d\tmonitors\n", timeGetTime());
  basmon.Init(&core);
  codemon.Init(&core);
  iomon.Init(&core);
  loadmon.Init();
  memmon.Init(&core);
  opnmon.Init(core.GetOPN1(), core.GetSound());
  regmon.Init(&core);

  core.GetSound()->SetSoundMonitor(&opnmon);

  //  実行ファイル改変チェック
  Log("%d\tself test\n", timeGetTime());
  if (!SanityCheck())
    return false;

  //  エミュレーション開始
  Log("%d\temulation begin\n", timeGetTime());
  core.Wait(false);
  active = true;
  fullscreen = false;

  //  設定反映
  Log("%d\tapply cmdline\n", timeGetTime());
  SetCurrentDirectory(path);
  ApplyCommandLine(cmdline);
  Log("%d\tapply config\n", timeGetTime());
  ApplyConfig();

  //  リセット
  Log("%d\treset\n", timeGetTime());
  core.Reset();

  // あとごちゃごちゃしたもの
  Log("%d\tetc\n", timeGetTime());
  if (!diskinfo[0].filename[0])
    LoadConfigDirectory(&config, m88ini, "Directory", false);
  ChangeDisplayType(true);
  ResizeWindow(640, 400);

  Log("%d\tend initm88\n", timeGetTime());
  return true;
}

// ---------------------------------------------------------------------------
//  WinUI::CleanupM88
//  M88 の後片付け
//
void WinUI::CleanupM88() {
  pc88core::Config cfg = config;
  SaveConfig(&cfg, m88ini, true);
  core.Cleanup();
}

// ---------------------------------------------------------------------------
//  WinUI::InitWindow
//  M88 の窓作成
//
bool WinUI::InitWindow(int nwinmode) {
  WNDCLASS wcl;
  const char* szwinname = "M88p2 WinUI";

  wcl.hInstance = hinst;
  wcl.lpszClassName = szwinname;
  wcl.lpfnWndProc = WNDPROC((void*)(WinProcGate));
  wcl.style = 0;
  wcl.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON_M88));
  wcl.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcl.cbClsExtra = 0;
  wcl.cbWndExtra = 0;
  //  wcl.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
  wcl.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
  wcl.lpszMenuName = MAKEINTRESOURCE(IDR_MENU_M88);

  accel = LoadAccelerators(hinst, MAKEINTRESOURCE(IDR_ACC_M88UI));

  if (!RegisterClass(&wcl))
    return false;

  wstyle = WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX;

  hwnd = CreateWindowEx(WS_EX_ACCEPTFILES,  // | WS_EX_LAYERED,
                        szwinname, "M88", wstyle,
                        CW_USEDEFAULT,  // x
                        CW_USEDEFAULT,  // y
                        640,            // w
                        400,            // h
                        nullptr, nullptr, hinst, nullptr);

  //  SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0xc0, LWA_ALPHA);

  if (!draw.Init0(hwnd))
    return false;

  clipmode = 0;
  guimodebymouse = false;

  return true;
}

// ---------------------------------------------------------------------------
//  WinUI::Main
//  メッセージループ
//
int WinUI::Main(const char* cmdline) {
  hmenudbg = 0;
  if (InitM88(cmdline)) {
    timerid = ::SetTimer(hwnd, 1, 1000, 0);
    ::SetTimer(hwnd, 2, 100, 0);
    statusdisplay.impl()->Init(hwnd);

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    //      core.ActivateMouse(true);
  } else {
    ReportError();
    PostQuitMessage(1);
  }

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    if (winconfig.ProcMsg(msg)) {
      if (!winconfig.IsOpen())
        SetGUIFlag(false);
      continue;
    }

    if (!TranslateAccelerator(msg.hwnd, accel, &msg)) {
      if ((msg.message == WM_SYSKEYDOWN || msg.message == WM_SYSKEYUP) &&
          !(config.flags & Config::kSuppressMenu))
        TranslateMessage(&msg);
    }
    DispatchMessage(&msg);
  }

  pc88core::OPNInterface* opn = core.GetOPN1();
  if (opn)
    opn->Reset();
  CleanupM88();
  statusdisplay.impl()->Cleanup();
  return msg.wParam;
}

// ---------------------------------------------------------------------------
//  WinUI::WinProc
//  メッセージハンドラ
//
#define PROC_MSG(msg, func)     \
  case (msg):                   \
    ret = (func)(hwnd, wp, lp); \
    break

LRESULT WinUI::WinProc(HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp) {
  uint32_t ret;
  keyif.Disable(true);

  switch (umsg) {
    PROC_MSG(WM_COMMAND, OnCommand);
    PROC_MSG(WM_PAINT, OnPaint);
    PROC_MSG(WM_CREATE, OnCreate);
    PROC_MSG(WM_DESTROY, OnDestroy);
    PROC_MSG(WM_CLOSE, OnClose);
    PROC_MSG(WM_TIMER, OnTimer);
    PROC_MSG(WM_ACTIVATE, OnActivate);
    PROC_MSG(WM_PALETTECHANGED, OnPaletteChanged);
    PROC_MSG(WM_QUERYNEWPALETTE, OnQueryNewPalette);
    PROC_MSG(WM_INITMENU, OnInitMenu);
    PROC_MSG(WM_KEYUP, OnKeyUp);
    PROC_MSG(WM_KEYDOWN, OnKeyDown);
    PROC_MSG(WM_SYSKEYUP, OnSysKeyUp);
    PROC_MSG(WM_SYSKEYDOWN, OnSysKeyDown);
    PROC_MSG(WM_SIZE, OnSize);
    PROC_MSG(WM_MOVE, OnMove);
    PROC_MSG(WM_DRAWITEM, OnDrawItem);
    PROC_MSG(WM_ENTERMENULOOP, OnEnterMenuLoop);
    PROC_MSG(WM_EXITMENULOOP, OnExitMenuLoop);
    PROC_MSG(WM_DISPLAYCHANGE, OnDisplayChange);
    PROC_MSG(WM_DROPFILES, OnDropFiles);
    PROC_MSG(WM_LBUTTONDOWN, OnLButtonDown);
    PROC_MSG(WM_LBUTTONUP, OnLButtonUp);
    PROC_MSG(WM_RBUTTONDOWN, OnRButtonDown);
    PROC_MSG(WM_RBUTTONUP, OnRButtonUp);
    PROC_MSG(WM_ENTERSIZEMOVE, OnEnterSizeMove);
    PROC_MSG(WM_EXITSIZEMOVE, OnExitSizeMove);
    PROC_MSG(WM_M88_SENDKEYSTATE, OnM88SendKeyState);
    PROC_MSG(WM_M88_APPLYCONFIG, OnM88ApplyConfig);
    PROC_MSG(WM_M88_CHANGEDISPLAY, OnM88ChangeDisplay);
    PROC_MSG(WM_M88_CHANGEVOLUME, OnM88ChangeVolume);
    PROC_MSG(WM_M88_CLIPCURSOR, OnM88ClipCursor);
    PROC_MSG(WM_MOUSEMOVE, OnMouseMove);
    PROC_MSG(WM_SETCURSOR, OnSetCursor);

    default:
      ret = DefWindowProc(hwnd, umsg, wp, lp);
  }
  keyif.Disable(false);
  return ret;
}

LRESULT CALLBACK WinUI::WinProcGate(HWND hwnd,
                                    UINT umsg,
                                    WPARAM wparam,
                                    LPARAM lparam) {
  return g_ui->WinProc(hwnd, umsg, wparam, lparam);
}

// ---------------------------------------------------------------------------
//  WinUI::M88SendKeyState
//
inline uint32_t WinUI::OnM88SendKeyState(HWND hwnd,
                                         WPARAM wparam,
                                         LPARAM lparam) {
  uint8_t* dest = reinterpret_cast<uint8_t*>(wparam);
  GetKeyboardState(dest);
  SetEvent((HANDLE)lparam);
  return 0;
}

inline uint32_t WinUI::OnKeyDown(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  if ((uint32_t)wparam == VK_F12 && !(config.flags & Config::kDisableF12Reset))
    ;
  else
    keyif.KeyDown((uint32_t)wparam, (uint32_t)lparam);

  return 0;
}

inline uint32_t WinUI::OnKeyUp(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  if ((uint32_t)wparam == VK_F12 && !(config.flags & Config::kDisableF12Reset))
    Reset();
  else
    keyif.KeyUp((uint32_t)wparam, (uint32_t)lparam);

  return 0;
}

inline uint32_t WinUI::OnSysKeyDown(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  if (config.flags & Config::kSuppressMenu) {
    keyif.KeyDown((uint32_t)wparam, (uint32_t)lparam);
    return 0;
  }
  return DefWindowProc(hwnd, WM_SYSKEYDOWN, wparam, lparam);
}

inline uint32_t WinUI::OnSysKeyUp(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  if (config.flags & Config::kSuppressMenu) {
    keyif.KeyUp((uint32_t)wparam, (uint32_t)lparam);
    return 0;
  }
  return DefWindowProc(hwnd, WM_SYSKEYUP, wparam, lparam);
}

// ---------------------------------------------------------------------------
//  WinUI::OnActivate
//  WM_ACTIVATE ハンドラ
//
uint32_t WinUI::OnActivate(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  bool prev_bg = background_;
  background_ = LOWORD(wparam) == WA_INACTIVE;

  if (!HIWORD(wparam)) {
    draw.RequestPaint();
  }

  keyif.Activate(!background_);
  draw.QueryNewPalette(background_);
  if (prev_bg != background_) {
    // core.ActivateMouse(!background_);
    OnM88ClipCursor(hwnd,
                    background_ ? CLIPCURSOR_RELEASE : -CLIPCURSOR_RELEASE, 0);
    draw.SetGUIFlag(background_);
  }
  snapshotchanged = true;
  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::OnQueryNewPalette
//  WM_QUERYNEWPALETTE ハンドラ
//
uint32_t WinUI::OnQueryNewPalette(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  draw.QueryNewPalette(background_);
  return 1;
}

// ---------------------------------------------------------------------------
//  WinUI::OnPaletteChanged
//  WM_PALETTECHANGED ハンドラ
//
uint32_t WinUI::OnPaletteChanged(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  if ((HWND)wparam != hwnd) {
    draw.QueryNewPalette(background_);
    return 1;
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::OnCommand
//  WM_COMMAND ハンドラ
//
uint32_t WinUI::OnCommand(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  uint32_t wid = LOWORD(wparam);
  switch (wid) {
    case IDM_EXIT:
      PostMessage(hwnd, WM_CLOSE, 0, 0);
      break;

    case IDM_RESET:
      Reset();
      break;

    case IDM_N88V1:
      config.basicmode = Config::N88V1;
      Reset();
      break;

    case IDM_N88V1H:
      config.basicmode = Config::N88V1H;
      Reset();
      break;

    case IDM_N88V2:
      config.basicmode = Config::N88V2;
      Reset();
      break;

    case IDM_N88V2CD:
      config.basicmode = Config::N88V2CD;
      Reset();
      break;

    case IDM_NMODE:
      config.basicmode = Config::N80;
      Reset();
      break;

    case IDM_N80MODE:
      config.basicmode = Config::N802;
      Reset();
      break;

    case IDM_N80V2MODE:
      config.basicmode = Config::N80V2;
      Reset();
      break;

    case IDM_DRIVE_1:
    case IDM_CHANGEIMAGE_1:
      ChangeDiskImage(hwnd, 0);
      break;

    case IDM_DRIVE_2:
    case IDM_CHANGEIMAGE_2:
      ChangeDiskImage(hwnd, 1);
      break;

    case IDM_BOTHDRIVE:
      diskmgr->Unmount(1);
      OpenDiskImage(1, 0, 0, 0, false);
      ChangeDiskImage(hwnd, 0);
      break;

    case IDM_ABOUTM88:
      SetGUIFlag(true);
      M88About().Show(hinst, hwnd);
      SetGUIFlag(false);
      break;

    case IDM_CONFIG:
      SetGUIFlag(true);
      winconfig.Show(hinst, hwnd, &config);
      break;

    case IDM_TOGGLEDISPLAY:
      ToggleDisplayMode();
      break;

    case IDM_CAPTURE:
      CaptureScreen();
      break;

    case IDM_DEBUG_TEXT:
      config.flags ^= pc88core::Config::kSpecialPalette;
      ApplyConfig();
      break;

    case IDM_DEBUG_GVRAM0:
      config.flag2 ^= pc88core::Config::kMask0;
      ApplyConfig();
      break;

    case IDM_DEBUG_GVRAM1:
      config.flag2 ^= pc88core::Config::kMask1;
      ApplyConfig();
      break;

    case IDM_DEBUG_GVRAM2:
      config.flag2 ^= pc88core::Config::kMask2;
      ApplyConfig();
      break;

    case IDM_STATUSBAR:
      config.flags ^= pc88core::Config::kShowStatusBar;
      ShowStatusWindow();
      break;

    case IDM_FDC_STATUS:
      config.flags ^= pc88core::Config::kShowFDCStatus;
      ApplyConfig();
      break;

    case IDM_BASMON:
      basmon.Show(hinst, hwnd, !basmon.IsOpen());
      break;

    case IDM_CODEMON:
      codemon.Show(hinst, hwnd, !codemon.IsOpen());
      break;

    case IDM_IOMON:
      iomon.Show(hinst, hwnd, !iomon.IsOpen());
      break;

    case IDM_LOADMON:
      loadmon.Show(hinst, hwnd, !loadmon.IsOpen());
      break;

    case IDM_MEMMON:
      memmon.Show(hinst, hwnd, !memmon.IsOpen());
      break;

    case IDM_SOUNDMON:
      opnmon.Show(hinst, hwnd, !opnmon.IsOpen());
      break;

    case IDM_WATCHREGISTER:
      config.flags &= ~pc88core::Config::kWatchRegister;
      regmon.Show(hinst, hwnd, !regmon.IsOpen());
      break;

    case IDM_TAPE:
      OpenTape();
      break;

    case IDM_RECORDPCM:
      if (!core.GetSound()->IsDumping()) {
        char buf[16];
        SYSTEMTIME t;

        GetLocalTime(&t);
        wsprintf(buf, "%.2d%.2d%.2d%.2d.wav", t.wDay, t.wHour, t.wMinute,
                 t.wSecond);
        core.GetSound()->DumpBegin(buf);
      } else {
        core.GetSound()->DumpEnd();
      }
      break;

    case IDM_SNAPSHOT_SAVE:
      SaveSnapshot(currentsnapshot);
      break;
    case IDM_SNAPSHOT_SAVE_0:
    case IDM_SNAPSHOT_SAVE_1:
    case IDM_SNAPSHOT_SAVE_2:
    case IDM_SNAPSHOT_SAVE_3:
    case IDM_SNAPSHOT_SAVE_4:
    case IDM_SNAPSHOT_SAVE_5:
    case IDM_SNAPSHOT_SAVE_6:
    case IDM_SNAPSHOT_SAVE_7:
    case IDM_SNAPSHOT_SAVE_8:
    case IDM_SNAPSHOT_SAVE_9:
      SaveSnapshot(wid - IDM_SNAPSHOT_SAVE_0);
      break;

    case IDM_SNAPSHOT_LOAD:
      LoadSnapshot(currentsnapshot);
      break;
    case IDM_SNAPSHOT_LOAD_0:
    case IDM_SNAPSHOT_LOAD_1:
    case IDM_SNAPSHOT_LOAD_2:
    case IDM_SNAPSHOT_LOAD_3:
    case IDM_SNAPSHOT_LOAD_4:
    case IDM_SNAPSHOT_LOAD_5:
    case IDM_SNAPSHOT_LOAD_6:
    case IDM_SNAPSHOT_LOAD_7:
    case IDM_SNAPSHOT_LOAD_8:
    case IDM_SNAPSHOT_LOAD_9:
      LoadSnapshot(wid - IDM_SNAPSHOT_LOAD_0);
      break;

    default:
      if (IDM_CHANGEDISK_1 <= wid && wid < IDM_CHANGEDISK_1 + 64) {
        SelectDisk(0, wid - IDM_CHANGEDISK_1, false);
        break;
      }
      if (IDM_CHANGEDISK_2 <= wid && wid < IDM_CHANGEDISK_2 + 64) {
        SelectDisk(1, wid - IDM_CHANGEDISK_2, false);
        break;
      }
      break;
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::OnPaint
//  WM_PAINT ハンドラ
//
uint32_t WinUI::OnPaint(HWND hwnd, WPARAM wp, LPARAM lp) {
  draw.RequestPaint();
  return DefWindowProc(hwnd, WM_PAINT, wp, lp);
}

// ---------------------------------------------------------------------------
//  WinUI::OnCreate
//  WM_CREATE ハンドラ
//
uint32_t WinUI::OnCreate(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  CREATESTRUCT* cs = (CREATESTRUCT*)wparam;

  RECT rect;
  rect.left = 0;
  rect.right = 640;
  rect.top = 0;
  rect.bottom = 400;

  AdjustWindowRectEx(&rect, wstyle, TRUE, 0);
  SetWindowPos(hwnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
               SWP_NOMOVE | SWP_NOZORDER);

  (*EnableIME)(hwnd, false);
  //  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

  GetWindowRect(hwnd, &rect);
  point.x = rect.left;
  point.y = rect.top;

  Log("WmCreate\n");
  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::OnDestroy
//  WM_DESTROY ハンドラ
//
uint32_t WinUI::OnDestroy(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  PostQuitMessage(0);
  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::OnClose
//  WM_CLOSE ハンドラ
//
uint32_t WinUI::OnClose(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  // 確認
  if (config.flags & Config::kAskBeforeReset) {
    SetGUIFlag(true);
    int res = MessageBox(hwnd, "M88 を終了します", "M88",
                         MB_ICONEXCLAMATION | MB_OKCANCEL | MB_DEFBUTTON2);
    SetGUIFlag(false);
    if (res != IDOK)
      return 0;
  }

  // タイマー破棄
  KillTimer(hwnd, timerid);
  timerid = 0;

  // 拡張メニューを破壊する
  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.cbSize = sizeof(MENUITEMINFO);
  mii.fMask = MIIM_SUBMENU;
  mii.hSubMenu = 0;

  SetMenuItemInfo(GetMenu(hwnd), IDM_DRIVE_1, false, &mii);
  SetMenuItemInfo(GetMenu(hwnd), IDM_DRIVE_2, false, &mii);
  SetMenuItemInfo(GetMenu(hwnd), IDM_SNAPSHOT_LOAD, false, &mii);
  SetMenuItemInfo(GetMenu(hwnd), IDM_SNAPSHOT_SAVE, false, &mii);

  int i;
  for (i = 0; i < 2; i++) {
    if (diskinfo[i].hmenu)
      DestroyMenu(diskinfo[i].hmenu), diskinfo[i].hmenu = 0;
  }
  for (i = 0; i < 2; i++) {
    if (hmenuss[i])
      DestroyMenu(hmenuss[i]), hmenuss[i] = 0;
  }

  // 窓を閉じる
  DestroyWindow(hwnd);
  active = false;

  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::OnTimer
//  WM_TIMER ハンドラ
//
uint32_t WinUI::OnTimer(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  Log("WmTimer:%d(%d)\n", wparam, timerid);
  if (wparam == timerid) {
    // 実効周波数,表示フレーム数を取得
    int fcount = draw.GetDrawCount();
    int32_t icount = core.GetExecCount();

    // レポートする場合はタイトルバーを更新
    if (report) {
      if (active) {
        char buf[64];
        uint32_t freq = icount / 10000;
        wsprintf(buf, "M88 - %d fps.  %d.%.2d MHz", fcount, freq / 100,
                 freq % 100);
        SetWindowText(hwnd, buf);
      } else
        SetWindowText(hwnd, "M88");
    }

    if (resetwindowsize > 0) {
      resetwindowsize--;
      ResizeWindow(640, 400);
    }
    return 0;
  }
  if (wparam == 2) {
    KillTimer(hwnd, 2);
    InvalidateRect(hwnd, 0, false);
    return 0;
  }
  if (wparam == statusdisplay.impl()->GetTimerID()) {
    statusdisplay.impl()->Update();
    return 0;
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::OnInitMenu
//  WM_INITMENU ハンドラ
//
uint32_t WinUI::OnInitMenu(HWND hwnd, WPARAM wp, LPARAM lp) {
  HMENU hmenu = (HMENU)wp;
#ifndef DEBUG_MONITOR
  EnableMenuItem(hmenu, IDM_LOGSTART, MF_GRAYED);
  EnableMenuItem(hmenu, IDM_LOGEND, MF_GRAYED);
#endif
  CheckMenuItem(hmenu, IDM_N88V1, (config.basicmode == Config::N88V1)
                                      ? MF_CHECKED
                                      : MF_UNCHECKED);
  CheckMenuItem(hmenu, IDM_N88V1H, (config.basicmode == Config::N88V1H)
                                       ? MF_CHECKED
                                       : MF_UNCHECKED);
  CheckMenuItem(hmenu, IDM_N88V2, (config.basicmode == Config::N88V2)
                                      ? MF_CHECKED
                                      : MF_UNCHECKED);
  CheckMenuItem(hmenu, IDM_NMODE,
                (config.basicmode == Config::N80) ? MF_CHECKED : MF_UNCHECKED);
  CheckMenuItem(hmenu, IDM_N80MODE,
                (config.basicmode == Config::N802) ? MF_CHECKED : MF_UNCHECKED);
  CheckMenuItem(hmenu, IDM_N80V2MODE, (config.basicmode == Config::N80V2)
                                          ? MF_CHECKED
                                          : MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_N88V2CD, (config.basicmode == Config::N88V2CD)
                                        ? MF_CHECKED
                                        : MF_UNCHECKED);
  EnableMenuItem(hmenu, IDM_N88V2CD,
                 core.IsCDSupported() ? MF_ENABLED : MF_GRAYED);

  CheckMenuItem(hmenu, IDM_STATUSBAR, (config.flags & Config::kShowStatusBar)
                                          ? MF_CHECKED
                                          : MF_UNCHECKED);
  EnableMenuItem(hmenu, IDM_STATUSBAR, fullscreen ? MF_GRAYED : MF_ENABLED);
  CheckMenuItem(hmenu, IDM_FDC_STATUS, (config.flags & Config::kShowFDCStatus)
                                           ? MF_CHECKED
                                           : MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_BASMON, basmon.IsOpen() ? MF_CHECKED : MF_UNCHECKED);
  CheckMenuItem(hmenu, IDM_CODEMON,
                codemon.IsOpen() ? MF_CHECKED : MF_UNCHECKED);
  CheckMenuItem(hmenu, IDM_IOMON, iomon.IsOpen() ? MF_CHECKED : MF_UNCHECKED);
  CheckMenuItem(hmenu, IDM_LOADMON,
                loadmon.IsOpen() ? MF_CHECKED : MF_UNCHECKED);
  CheckMenuItem(hmenu, IDM_MEMMON, memmon.IsOpen() ? MF_CHECKED : MF_UNCHECKED);
  CheckMenuItem(hmenu, IDM_SOUNDMON,
                opnmon.IsOpen() ? MF_CHECKED : MF_UNCHECKED);
  CheckMenuItem(
      hmenu, IDM_WATCHREGISTER,
      (config.dipsw() != 1 && regmon.IsOpen()) ? MF_CHECKED : MF_UNCHECKED);

  CheckMenuItem(hmenu, IDM_RECORDPCM,
                core.GetSound()->IsDumping() ? MF_CHECKED : MF_UNCHECKED);

  EnableMenuItem(hmenu, IDM_DUMPCPU1,
                 core.GetCPU1()->GetDumpState() == -1 ? MF_GRAYED : MF_ENABLED);
  CheckMenuItem(hmenu, IDM_DUMPCPU1, core.GetCPU1()->GetDumpState() == 1
                                         ? MF_CHECKED
                                         : MF_UNCHECKED);
  EnableMenuItem(hmenu, IDM_DUMPCPU2,
                 core.GetCPU2()->GetDumpState() == -1 ? MF_GRAYED : MF_ENABLED);
  CheckMenuItem(hmenu, IDM_DUMPCPU2, core.GetCPU2()->GetDumpState() == 1
                                         ? MF_CHECKED
                                         : MF_UNCHECKED);

  if (hmenudbg) {
    CheckMenuItem(
        hmenudbg, IDM_DEBUG_TEXT,
        (config.flags & Config::kSpecialPalette) ? MF_CHECKED : MF_UNCHECKED);
    int mask = (config.flag2 / Config::kMask0) & 7;
    CheckMenuItem(hmenudbg, IDM_DEBUG_GVRAM0,
                  (mask & 1) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmenudbg, IDM_DEBUG_GVRAM1,
                  (mask & 2) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmenudbg, IDM_DEBUG_GVRAM2,
                  (mask & 4) ? MF_CHECKED : MF_UNCHECKED);
  }

  MakeSnapshotMenu();
  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::OnSize
//  WM_SIZE
//
uint32_t WinUI::OnSize(HWND hwnd, WPARAM wp, LPARAM lp) {
  HWND hwndstatus = statusdisplay.impl()->GetHWnd();
  if (hwndstatus)
    PostMessage(hwndstatus, WM_SIZE, wp, lp);
  active = wp != SIZE_MINIMIZED;
  draw.Activate(active);
  return DefWindowProc(hwnd, WM_SIZE, wp, lp);
}

// ---------------------------------------------------------------------------
//  WinUI::ReportError
//  エラー表示
//
void WinUI::ReportError() {
  const char* errtext = Error::GetErrorText();
  if (errtext) {
    ::MessageBox(hwnd, errtext, "M88", MB_ICONERROR | MB_OK);
  }
}

// ---------------------------------------------------------------------------
//  WinUI::ReportError
//
uint32_t WinUI::OnM88ApplyConfig(HWND, WPARAM newconfig, LPARAM) {
  if (newconfig) {
    // 乱暴ですな。
    if (memcmp(&config, (pc88core::Config*)newconfig, sizeof(pc88core::Config))) {
      config = *((pc88core::Config*)newconfig);
      ApplyConfig();
    }
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::ApplyConfig
//
void WinUI::ApplyConfig() {
  config.mainsubratio =
      (config.clock() >= 60 || (config.flags & Config::kFullspeed)) ? 2 : 1;
  if (config.dipsw() != 1) {
    config.flags &= ~Config::kSpecialPalette;
    config.flag2 &= ~(Config::kMask0 | Config::kMask1 | Config::kMask2);
  }

  core.ApplyConfig(&config);
  keyif.ApplyConfig(&config);
  draw.SetPriorityLow((config.flags & Config::kDrawPriorityLow) != 0);

  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.cbSize = sizeof(MENUITEMINFO);
  mii.fMask = MIIM_TYPE;
  mii.fType = MFT_STRING;
  mii.dwTypeData =
      (config.flags & Config::kDisableF12Reset) ? "&Reset" : "&Reset\tF12";
  SetMenuItemInfo(GetMenu(hwnd), IDM_RESET, false, &mii);
  ShowStatusWindow();

  if (config.dipsw() == 1) {
    if (!hmenudbg) {
      hmenudbg = LoadMenu(hinst, MAKEINTRESOURCE(IDR_MENU_DEBUG));

      mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
      mii.fType = MFT_STRING;
      mii.dwTypeData = "Control &Plane";
      mii.hSubMenu = hmenudbg;
      SetMenuItemInfo(GetMenu(hwnd), IDM_WATCHREGISTER, false, &mii);
    }
  } else {
    mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
    mii.fType = MFT_STRING;
    mii.dwTypeData = "Show &Register";
    mii.hSubMenu = 0;
    SetMenuItemInfo(GetMenu(hwnd), IDM_WATCHREGISTER, false, &mii);
    hmenudbg = 0;
  }
}

// ---------------------------------------------------------------------------
//  WinUI::Reset
//
void WinUI::Reset() {
  if (config.flags & Config::kAskBeforeReset) {
    SetGUIFlag(true);
    int res = MessageBox(hwnd, "リセットしますか？", "M88",
                         MB_ICONQUESTION | MB_OKCANCEL | MB_DEFBUTTON2);
    SetGUIFlag(false);
    if (res != IDOK)
      return;
  }
  keyif.ApplyConfig(&config);
  core.ApplyConfig(&config);
  core.Reset();
}

// ---------------------------------------------------------------------------
//  ChangeDiskImage
//  ディスク入れ替え
//
void WinUI::ChangeDiskImage(HWND hwnd, int drive) {
  HANDLE hthread = GetCurrentThread();
  int prev = GetThreadPriority(hthread);
  SetThreadPriority(hthread, THREAD_PRIORITY_ABOVE_NORMAL);

  SetGUIFlag(true);

  if (!diskmgr->Unmount(drive)) {
    MessageBox(hwnd,
               "DiskManger::Unmount failed\nディスクの取り外しに失敗しました.",
               "M88", MB_ICONERROR | MB_OK);
  }

  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.FlagsEx = config.flag2 & Config::kShowPlacesBar ? 0 : OFN_EX_NOPLACESBAR;

  char filename[MAX_PATH];
  filename[0] = 0;

  ofn.hwndOwner = hwnd;
  ofn.lpstrFilter =
      "8801 disk image (*.d88)\0*.d88\0"
      "All Files (*.*)\0*.*\0";
  ofn.lpstrFile = filename;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_CREATEPROMPT | OFN_SHAREAWARE;
  ofn.lpstrDefExt = "d88";
  ofn.lpstrTitle = "Open disk image";

  (*EnableIME)(hwnd, true);
  bool isopen = !!GetOpenFileName(&ofn);
  (*EnableIME)(hwnd, false);

  if (isopen) {
    // 指定されたファイルは存在するか？
    bool createnew = false;
    if (!diskmgr->IsImageOpen(filename)) {
      FileIO file;
      if (!file.Open(filename, FileIO::readonly)) {
        if (file.GetError() == FileIO::file_not_found) {
          // ファイルが存在しない
          createnew = true;
          if (!newdisk.Show(hinst, hwnd))
            return;
        } else {
          // 何らかの理由でアクセスできない
          return;
        }
      }
    }

    OpenDiskImage(drive, filename, ofn.Flags & OFN_READONLY, 0, createnew);

    if (createnew && diskmgr->GetNumDisks(drive) == 0) {
      diskmgr->AddDisk(drive, newdisk.GetTitle(), newdisk.GetType());
      OpenDiskImage(drive, filename, 0, 0, false);
      if (newdisk.DoFormat())
        diskmgr->FormatDisk(drive);
    }
    if (drive == 0 && !diskinfo[1].filename[0] && diskmgr->GetNumDisks(0) > 1) {
      OpenDiskImage(1, filename, ofn.Flags & OFN_READONLY, 1, false);
    }
  } else
    OpenDiskImage(drive, 0, 0, 0, false);

  SetGUIFlag(false);
  SetThreadPriority(hthread, prev);
  snapshotchanged = true;
}

// ---------------------------------------------------------------------------
//  適当にディスクイメージを開く
//
void WinUI::OpenDiskImage(const char* path) {
  // ディスクイメージをマウントする
  OpenDiskImage(0, path, 0, 0, false);
  if (diskmgr->GetNumDisks(0) > 1) {
    OpenDiskImage(1, path, 0, 1, false);
  } else {
    diskmgr->Unmount(1);
    OpenDiskImage(1, 0, 0, 0, false);
  }
}

// ---------------------------------------------------------------------------
//  ファイルネームの部分を取り出す
//
static void GetFileNameTitle(char* title, const char* name) {
  if (name) {
    uint8_t* ptr;
    ptr = _mbsrchr((uint8_t*)name, '\\');
    _mbscpy((uint8_t*)title, ptr ? ptr + 1 : (uint8_t*)(name));
    ptr = _mbschr((uint8_t*)title, '.');
    if (ptr)
      *ptr = 0;
  }
}

// ---------------------------------------------------------------------------
//  OpenDiskImage
//  ディスクイメージを開く
//
bool WinUI::OpenDiskImage(int drive,
                          const char* name,
                          bool readonly,
                          int id,
                          bool create) {
  DiskInfo& dinfo = diskinfo[drive];

  bool result = false;
  if (name) {
    strcpy(dinfo.filename, name);
    result = diskmgr->Mount(drive, dinfo.filename, readonly, id, create);
    dinfo.readonly = readonly;
    dinfo.currentdisk = diskmgr->GetCurrentDisk(0);
  }
  if (!result)
    dinfo.filename[0] = 0;

  CreateDiskMenu(drive);
  return true;
}

// ---------------------------------------------------------------------------
//  SelectDisk
//  ディスクセット
//
bool WinUI::SelectDisk(uint32_t drive, int id, bool menuonly) {
  DiskInfo& dinfo = diskinfo[drive];
  if (drive >= 2 || id >= 64)
    return false;

  CheckMenuItem(
      dinfo.hmenu,
      dinfo.idchgdisk + (dinfo.currentdisk < 0 ? 63 : dinfo.currentdisk),
      MF_BYCOMMAND | MF_UNCHECKED);

  bool result = true;
  if (!menuonly)
    result = diskmgr->Mount(drive, dinfo.filename, dinfo.readonly, id, false);

  int current = result ? diskmgr->GetCurrentDisk(drive) : -1;

  CheckMenuItem(dinfo.hmenu, dinfo.idchgdisk + (current < 0 ? 63 : current),
                MF_BYCOMMAND | MF_CHECKED);
  dinfo.currentdisk = current;
  return true;
}

// ---------------------------------------------------------------------------
//  CreateDiskMenu
//  マルチディスクイメージ用メニューの作成
//
bool WinUI::CreateDiskMenu(uint32_t drive) {
  char buf[MAX_PATH + 16];

  DiskInfo& dinfo = diskinfo[drive];
  HMENU hmenuprev = dinfo.hmenu;
  dinfo.currentdisk = -1;

  int ndisks = std::min(diskmgr->GetNumDisks(drive), 60U);
  if (ndisks) {
    // メニュー作成
    dinfo.hmenu = CreatePopupMenu();
    if (!dinfo.hmenu)
      return false;

    for (int i = 0; i < ndisks; i++) {
      const char* title = diskmgr->GetImageTitle(drive, i);

      if (!title)
        break;
      if (!title[0])
        title = "(untitled)";

      wsprintf(buf, i < 9 ? "&%d %.16s" : "%d %.16s", i + 1, title);
      AppendMenu(dinfo.hmenu,
                 MF_STRING | (i && !(i % 20) ? MF_MENUBARBREAK : 0),
                 dinfo.idchgdisk + i, buf);
    }

    AppendMenu(dinfo.hmenu, MF_SEPARATOR, 0, 0);
    AppendMenu(dinfo.hmenu, MF_STRING, dinfo.idchgdisk + 63, "&N No disk");
    AppendMenu(dinfo.hmenu, MF_STRING, dinfo.idchgdisk + 64, "&0 Change disk");
    SetMenuDefaultItem(dinfo.hmenu, dinfo.idchgdisk + 64, FALSE);
  }

  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.cbSize = sizeof(MENUITEMINFO);
  mii.fType = MFT_STRING;
  mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
  mii.dwTypeData = buf;

  if (!ndisks) {
    wsprintf(buf, "Drive &%d...", drive + 1);
    mii.hSubMenu = 0;
  } else {
    char title[MAX_PATH] = "";
    GetFileNameTitle(title, diskinfo[drive].filename);

    wsprintf(buf, "Drive &%d - %s", drive + 1, title);
    mii.hSubMenu = dinfo.hmenu;
  }
  SetMenuItemInfo(GetMenu(hwnd), drive ? IDM_DRIVE_2 : IDM_DRIVE_1, false,
                  &mii);
  if (hmenuprev)
    DestroyMenu(hmenuprev);

  if (ndisks)
    SelectDisk(drive, dinfo.currentdisk, true);

  return true;
}

// ---------------------------------------------------------------------------
//  OpenTape
//  ディスク入れ替え
//
void WinUI::OpenTape() {
  HANDLE hthread = GetCurrentThread();
  int prev = GetThreadPriority(hthread);
  SetThreadPriority(hthread, THREAD_PRIORITY_ABOVE_NORMAL);

  SetGUIFlag(true);

  tapemgr->Close();

  OPENFILENAME ofn;
  memset(&ofn, 0, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.FlagsEx = config.flag2 & Config::kShowPlacesBar ? 0 : OFN_EX_NOPLACESBAR;

  char filename[MAX_PATH];
  filename[0] = 0;

  ofn.hwndOwner = hwnd;
  ofn.lpstrFilter =
      "T88 tape image (*.t88)\0*.t88\0"
      "All Files (*.*)\0*.*\0";
  ofn.lpstrFile = filename;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_CREATEPROMPT | OFN_SHAREAWARE;
  ofn.lpstrDefExt = "t88";
  ofn.lpstrTitle = "Open tape image";

  (*EnableIME)(hwnd, true);
  bool isopen = !!GetOpenFileName(&ofn);
  (*EnableIME)(hwnd, false);

  char buf[MAX_PATH + 32];
  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.cbSize = sizeof(MENUITEMINFO);
  mii.fMask = MIIM_TYPE;
  mii.fType = MFT_STRING;

  if (isopen && tapemgr->Open(filename)) {
    GetFileNameTitle(tapetitle, filename);
    wsprintf(buf, "&Tape - %s...", tapetitle);
    mii.dwTypeData = buf;
  } else {
    mii.dwTypeData = "&Tape...";
  }
  SetMenuItemInfo(GetMenu(hwnd), IDM_TAPE, false, &mii);

  SetGUIFlag(false);
  SetThreadPriority(hthread, prev);
  snapshotchanged = true;
}

// ---------------------------------------------------------------------------
//  WinUI::ResizeWindow
//  ウィンドウの大きさを変える
//
void WinUI::ResizeWindow(uint32_t width, uint32_t height) {
  RECT rect;
  rect.left = 0;
  rect.right = width;
  rect.top = 0;
  rect.bottom = height + statusdisplay.impl()->GetHeight();

  AdjustWindowRectEx(&rect, wstyle, TRUE, 0);
  SetWindowPos(hwnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  PostMessage(hwnd, WM_SIZE, SIZE_RESTORED, MAKELONG(width, height));
}

// ---------------------------------------------------------------------------
//  ステータスバー表示切り替え
//
void WinUI::ShowStatusWindow() {
  if (!fullscreen) {
    if (config.flags & pc88core::Config::kShowStatusBar)
      statusdisplay.impl()->Enable(
          (config.flags & pc88core::Config::kShowFDCStatus) != 0);
    else
      statusdisplay.impl()->Disable();
    ResizeWindow(640, 400);
  }
}

// ---------------------------------------------------------------------------
//  WinUI::OnDrawItem
//  WM_DRAWITEM ハンドラ
//
uint32_t WinUI::OnDrawItem(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  if ((UINT)wparam == 1)
    statusdisplay.impl()->DrawItem((DRAWITEMSTRUCT*)lparam);
  return TRUE;
}

// ---------------------------------------------------------------------------
//  WinUI::ToggleDisplayMode
//  全画面・ウィンドウ表示切替  (ALT+ENTER)
//
void WinUI::ToggleDisplayMode() {
  uint32_t tick = GetTickCount();
  if ((tick - displaychangedtime) < 1000)
    return;

  displaychangedtime = GetTickCount();
  fullscreen = !fullscreen;

  if (fullscreen)
    statusdisplay.impl()->Disable();
  ChangeDisplayType(fullscreen);
}

// ---------------------------------------------------------------------------
//  WinUI::ChangeDisplayType
//  表示メソッド変更
//
void WinUI::ChangeDisplayType(bool savepos) {
  if (winconfig.IsOpen()) {
    winconfig.Close();
    SetGUIFlag(false);
  }
  if (savepos) {
    RECT rect;
    GetWindowRect(hwnd, &rect);
    point.x = rect.left;
    point.y = rect.top;
  }
  PostMessage(hwnd, WM_M88_CHANGEDISPLAY, 0, 0);
}

// ---------------------------------------------------------------------------
//  WinUI::M88ChangeDisplay
//  表示メソッドの変更
//
uint32_t WinUI::OnM88ChangeDisplay(HWND hwnd, WPARAM, LPARAM) {
  // 画面ドライバの切替え
  // ドライバが false を返した場合 GDI ドライバが使用されることになる
  if (!draw.ChangeDisplayMode(fullscreen,
                              (config.flags & pc88core::Config::kForce480) != 0))
    fullscreen = false;

  // ウィンドウスタイル関係の変更
  wstyle = GetWindowLong(hwnd, GWL_STYLE);
  int exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);

  if (!fullscreen) {
    wstyle = (wstyle & ~WS_POPUP) | (WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU);
    exstyle &= ~WS_EX_TOPMOST;

    //      SetCapture(hwnd);
    SetWindowLong(hwnd, GWL_STYLE, wstyle);
    SetWindowLong(hwnd, GWL_EXSTYLE, exstyle);
    ResizeWindow(640, 400);
    SetWindowPos(hwnd, HWND_NOTOPMOST, point.x, point.y, 0, 0, SWP_NOSIZE);
    ShowStatusWindow();
    report = true;
    guimodebymouse = false;
  } else {
    if (guimodebymouse)
      SetGUIFlag(false);

    //  ReleaseCapture();
    wstyle = (wstyle & ~(WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU)) | WS_POPUP;
    exstyle |= WS_EX_TOPMOST;
    SetWindowLong(hwnd, GWL_STYLE, wstyle);
    SetWindowLong(hwnd, GWL_EXSTYLE, exstyle);
    SetWindowText(hwnd, "M88");

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

    report = false;
  }
  SetGUIFlag(true);
  SetGUIFlag(false);
  SetGUIFlag(true);
  SetGUIFlag(false);
  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::OnEnterMenuLoop
//  WM_ENTERMENULOOP ハンドラ
//
uint32_t WinUI::OnEnterMenuLoop(HWND, WPARAM wp, LPARAM) {
  if (!wp) {
    keyif.Activate(false);
    SetGUIFlag(true);
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::OnExitMenuLoop
//  WM_EXITMENULOOP ハンドラ
//
uint32_t WinUI::OnExitMenuLoop(HWND, WPARAM wp, LPARAM) {
  if (!wp) {
    keyif.Activate(true);
    SetGUIFlag(false);
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  ボリューム変更
//
uint32_t WinUI::OnM88ChangeVolume(HWND, WPARAM c, LPARAM) {
  if (c)
    core.SetVolume((pc88core::Config*)c);
  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::OnDisplayChange
//  WM_DISPLAYCHANGE ハンドラ
//
uint32_t WinUI::OnDisplayChange(HWND hwnd, WPARAM wp, LPARAM lp) {
  resetwindowsize = fullscreen ? 0 : 5;
  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::DropFiles
//  WM_DROPFILES ハンドラ
//  午後Ｔ氏の実装をもとに作成しました．
//
uint32_t WinUI::OnDropFiles(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  HDROP hdrop = (HDROP)wparam;

  // 受け取ったファイルの数を確認
  int nfiles = DragQueryFile(hdrop, ~0, 0, 0);
  if (nfiles != 1) {
    Toast::Show(50, 3000, "ドロップできるのはファイル１つだけです.");
    DragFinish(hdrop);
    return 0;
  }

  // ファイルネームを取得
  char path[MAX_PATH];
  DragQueryFile(hdrop, 0, path, 512);
  DragFinish(hdrop);

  OpenDiskImage(path);

  // 正常にマウントできたらリセット
  if (diskinfo[0].filename[0])
    Reset();

  return 0;
}

// ---------------------------------------------------------------------------
//  WinUI::CaptureScreen
//
void WinUI::CaptureScreen() {
  int bmpsize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) +
                15 * sizeof(RGBQUAD) + 320 * 400;
  std::unique_ptr<uint8_t[]> bmp(new uint8_t[bmpsize]);
  if (!bmp)
    return;

  bmpsize = draw.CaptureScreen(bmp.get());
  if (bmpsize) {
    bool save;
    char filename[MAX_PATH];

    if (config.flag2 & Config::kGenScrnshotName) {
      SYSTEMTIME time;
      GetLocalTime(&time);
      wsprintf(filename, "%.2d%.2d%.2d%.2d%.2d.bmp", time.wDay, time.wHour,
               time.wMinute, time.wSecond, time.wMilliseconds / 10);
      save = true;
      Toast::Show(80, 1500, "画面イメージを %s に保存しました", filename);
    } else {
      filename[0] = 0;

      OPENFILENAME ofn;
      memset(&ofn, 0, sizeof(OPENFILENAME));
      ofn.lStructSize = sizeof(OPENFILENAME);
      ofn.hwndOwner = hwnd;
      ofn.lpstrFilter = "bitmap image [4bpp] (*.bmp)\0*.bmp\0";
      ofn.lpstrFile = filename;
      ofn.nMaxFile = MAX_PATH;
      ofn.Flags = OFN_CREATEPROMPT | OFN_NOREADONLYRETURN;
      ofn.lpstrDefExt = "bmp";
      ofn.lpstrTitle = "Save captured image";
      ofn.FlagsEx =
          config.flag2 & Config::kShowPlacesBar ? 0 : OFN_EX_NOPLACESBAR;

      SetGUIFlag(true);
      (*EnableIME)(hwnd, true);
      save = !!GetSaveFileName(&ofn);
      (*EnableIME)(hwnd, false);
      SetGUIFlag(false);
    }
    if (save) {
      FileIO file;
      if (file.Open(filename, FileIO::create))
        file.Write(bmp.get(), bmpsize);
    }
  }
}

// ---------------------------------------------------------------------------
//  マウス状態の取得
//
uint32_t WinUI::OnLButtonDown(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  if (capturemouse) {
    mousebutton |= 1;
    return 0;
  }
  return DefWindowProc(hwnd, WM_LBUTTONDOWN, wparam, lparam);
}

uint32_t WinUI::OnLButtonUp(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  if (capturemouse) {
    mousebutton &= ~1;
    return 0;
  }
  return DefWindowProc(hwnd, WM_LBUTTONDOWN, wparam, lparam);
}

uint32_t WinUI::OnRButtonDown(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  if (capturemouse) {
    mousebutton |= 2;
    return 0;
  }
  return DefWindowProc(hwnd, WM_LBUTTONDOWN, wparam, lparam);
}

uint32_t WinUI::OnRButtonUp(HWND hwnd, WPARAM wparam, LPARAM lparam) {
  if (capturemouse) {
    mousebutton &= ~2;
    return 0;
  }
  return DefWindowProc(hwnd, WM_LBUTTONDOWN, wparam, lparam);
}

// ---------------------------------------------------------------------------
//  ウィンドウ移動モードに入る・出る
//
uint32_t WinUI::OnEnterSizeMove(HWND hwnd, WPARAM, LPARAM) {
  //  core.ActivateMouse(false);
  return 0;
}

uint32_t WinUI::OnExitSizeMove(HWND hwnd, WPARAM, LPARAM) {
  //  core.ActivateMouse(true);
  return 0;
}

// ---------------------------------------------------------------------------
//  WM_MOVE
//
uint32_t WinUI::OnMove(HWND hwnd, WPARAM, LPARAM) {
  POINT srcpoint;
  srcpoint.x = 0, srcpoint.y = 0;
  ClientToScreen(hwnd, &srcpoint);

  draw.WindowMoved(srcpoint.x, srcpoint.y);
  return 0;
}

// ---------------------------------------------------------------------------
//  WM_M88_CLIPCURSOR
//
uint32_t WinUI::OnM88ClipCursor(HWND hwnd, WPARAM op, LPARAM) {
  if (static_cast<int>(op) > 0)
    clipmode |= op;
  else
    clipmode &= ~(-op);

  if (clipmode && !(clipmode & CLIPCURSOR_RELEASE)) {
    RECT rect;
    POINT center;
    GetWindowRect(hwnd, &rect);

    if (clipmode & CLIPCURSOR_MOUSE) {
      center.x = (rect.left + rect.right) / 2;
      center.y = (rect.top + rect.bottom) / 2;
      rect.left = center.x - 180;
      rect.right = center.x + 180;
      rect.top = center.y - 180;
      rect.bottom = center.y + 180;
    }
    Log("rect: %d %d %d %d\n", rect.left, rect.top, rect.right, rect.bottom);
    ClipCursor(&rect);
  } else {
    ClipCursor(0);
  }
  return 1;
}

uint32_t WinUI::OnMouseMove(HWND hwnd, WPARAM wp, LPARAM lp) {
#if 0
  if (fullscreen) {
    POINTS p;
    p = MAKEPOINTS(lp);
    uint32_t menu = p.y < 8;
    if (!guimodebymouse) {
      if (menu) {
        guimodebymouse = true;
        SetGUIFlag(true);
      }
    } else {
      if (!menu) {
        guimodebymouse = false;
        SetGUIFlag(false);
      }
    }
  }
#endif
  return 0;
}

uint32_t WinUI::OnSetCursor(HWND hwnd, WPARAM wp, LPARAM lp) {
  if (fullscreen) {
    uint32_t menu = LOWORD(lp) == HTMENU;
    if (!guimodebymouse) {
      if (menu) {
        guimodebymouse = true;
        SetGUIFlag(true);
      }
    } else {
      if (!menu) {
        guimodebymouse = false;
        SetGUIFlag(false);
      }
    }
  }
  // Toast::Show(0, 0, "%d", LOWORD(lp));
  return DefWindowProc(hwnd, WM_SETCURSOR, wp, lp);
}

// ---------------------------------------------------------------------------
//  GUI 操作モードに入る
//
void WinUI::SetGUIFlag(bool gui) {
  if (gui && !background_) {
    if (!background_)
      ::DrawMenuBar(hwnd);
  }
  //  core.SetGUIFlag(gui);
  draw.SetGUIFlag(gui);
}

// ---------------------------------------------------------------------------
//  スナップショットの名前を作る
//
void WinUI::GetSnapshotName(char* name, int n) {
  char buf[MAX_PATH];
  char* title;

  if (diskmgr->GetNumDisks(0)) {
    GetFileNameTitle(buf, diskinfo[0].filename);
    title = buf;
  } else if (tapemgr->IsOpen()) {
    title = tapetitle;
  } else {
    title = "snapshot";
  }

  if (n >= 0)
    wsprintf(name, "%s_%d.s88", title, n);
  else
    wsprintf(name, "%s_?.s88", title);
}

// ---------------------------------------------------------------------------
//  スナップショットの書込み
//
void WinUI::SaveSnapshot(int n) {
  char name[MAX_PATH];
  GetSnapshotName(name, n);
  if (core.SaveSnapshot(name))
    Toast::Show(80, 3000, "%s に保存しました", name);
  else
    Toast::Show(80, 3000, "%s に保存できません", name);
  currentsnapshot = n;
  snapshotchanged = true;
}

// ---------------------------------------------------------------------------
//  スナップショットの読み込み
//
void WinUI::LoadSnapshot(int n) {
  char name[MAX_PATH];
  GetSnapshotName(name, n);
  bool r;
  if (diskinfo[0].filename && diskmgr->GetNumDisks(0) >= 2) {
    OpenDiskImage(1, diskinfo[0].filename, diskinfo[0].readonly, 1, false);
    r = core.LoadSnapshot(name, diskinfo[0].filename);
  } else {
    r = core.LoadSnapshot(name, 0);
  }

  if (r)
    Toast::Show(80, 2500, "%s から復元しました", name);
  else
    Toast::Show(80, 2500, "%s から復元できません", name);
  for (uint32_t i = 0; i < 2; i++)
    CreateDiskMenu(i);
  currentsnapshot = n;
  snapshotchanged = true;
}

// ---------------------------------------------------------------------------
//  スナップショット用のメニューを作成
//
bool WinUI::MakeSnapshotMenu() {
  if (snapshotchanged) {
    int i;
    snapshotchanged = false;

    // メニューを元に戻す
    MENUITEMINFO mii;
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_SUBMENU;
    mii.hSubMenu = 0;
    SetMenuItemInfo(GetMenu(hwnd), IDM_SNAPSHOT_LOAD, false, &mii);
    SetMenuItemInfo(GetMenu(hwnd), IDM_SNAPSHOT_SAVE, false, &mii);

    // メニュー作成
    for (i = 0; i < 2; i++) {
      if (hmenuss[i])
        DestroyMenu(hmenuss[i]);
      hmenuss[i] = CreatePopupMenu();
      if (!hmenuss[i])
        return false;
    }

    // スナップショットを検索
    int entries = 0;
    FileFinder ff;
    char buf[MAX_PATH];

    GetSnapshotName(buf, -1);
    int p = strlen(buf) - 5;
    ff.FindFile(buf);
    while (ff.FindNext()) {
      int n = ff.GetFileName()[p] - '0';
      if (0 <= n && n <= 9)
        entries |= 1 << n;
    }

    // メニュー作成
    for (i = 0; i < 10; i++) {
      wsprintf(buf, "&%d", i);
      AppendMenu(hmenuss[0], MF_STRING, IDM_SNAPSHOT_SAVE_0 + i, buf);

      if (entries & (1 << i)) {
        CheckMenuItem(hmenuss[0], IDM_SNAPSHOT_SAVE_0 + i,
                      MF_BYCOMMAND | MF_CHECKED);
        AppendMenu(hmenuss[1], MF_STRING, IDM_SNAPSHOT_LOAD_0 + i, buf);
        if (i == currentsnapshot)
          SetMenuDefaultItem(hmenuss[1], IDM_SNAPSHOT_LOAD_0 + i, FALSE);
      }
    }
    SetMenuDefaultItem(hmenuss[0], IDM_SNAPSHOT_SAVE_0 + currentsnapshot,
                       FALSE);

    mii.hSubMenu = hmenuss[0];
    SetMenuItemInfo(GetMenu(hwnd), IDM_SNAPSHOT_SAVE, false, &mii);
    if (entries) {
      mii.hSubMenu = hmenuss[1];
      SetMenuItemInfo(GetMenu(hwnd), IDM_SNAPSHOT_LOAD, false, &mii);
    }
  }
  return true;
}

// ---------------------------------------------------------------------------
//  コマンドライン走査
//
//  書式:
//  M88 [-flags] diskimagepath
//
//  -bN     basic mode (hex)
//  -fA,B   flags (16進)    (A=flagsの中身, B=反映させるビット)
//  -gA,B   flag2 (16進)    (A=flag2の中身, B=反映させるビット)
//  -cCLK   clock (MHz) (10進)
//  -F      フルスクリーン起動
//
//  N, A, B の値は src/pc88/config.h を参照．
//
//  現バージョンでは設定ダイアログでは設定できない組み合わせも
//  受け付けてしまうので要注意
//  例えばマウスを使用しながらキーによるメニュー抑制なんかはヤバイかも．
//
//  例: M88 -b31 -c8 -f10,10 popfulmail.d88
//      V2 モード，8MHz，OPNA モードで popfulmail.d88 を起動する
//
//  説明が分かりづらいのは百も承知です(^^;
//  詳しくは下を参照するか，開発室にでも質問してください．
//
//  他のパラメータも変更できるようにしたい場合も，一言頂ければ対応します．
//
void WinUI::ApplyCommandLine(const char* cmdline) {
  bool change = false;
  while (*cmdline) {
    while (*cmdline == ' ')
      cmdline++;

    if (*cmdline == '-') {
      cmdline += 2;
      switch (tolower(cmdline[-1])) {
        char* endptr;
        int32_t newflags, activate;

        // BASIC モードを設定  -bモード番号
        case 'b':
          config.basicmode = Config::BASICMode(strtoul(cmdline, &endptr, 16));
          cmdline = endptr;
          change = true;
          break;

        // clock を設定  -cクロック
        case 'c':
          config.set_clock(Limit(strtoul(cmdline, &endptr, 10), 100, 1) * 10);
          cmdline = endptr;
          change = true;
          break;

        // フルスクリーン起動
        case 'F':
          fullscreen = true;
          break;

        // flags の値を設定  -g値,有効ビット
        case 'f':
          newflags = strtoul(cmdline, &endptr, 16);
          activate = ~0;
          if (*endptr == ',') {
            activate = strtoul(endptr + 1, &endptr, 16);
          }
          cmdline = endptr;
          config.flags = (config.flags & ~activate) | (newflags & activate);
          change = true;
          break;

        // flag2 の値を設定  -g値,有効ビット
        case 'g':
          newflags = strtoul(cmdline, &endptr, 16);
          activate = ~0;
          if (*endptr == ',') {
            activate = strtoul(endptr + 1, &endptr, 16);
          }
          cmdline = endptr;
          config.flag2 = (config.flag2 & ~activate) | (newflags & activate);
          change = true;
          break;
      }

      while (*cmdline && *cmdline != ' ')
        cmdline++;
      continue;
    }

    // 残りは多分ファイル名
    OpenDiskImage(cmdline);
    break;
  }
}
}  // namespace m88win