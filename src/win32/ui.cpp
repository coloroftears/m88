// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//	Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	User Interface for Win32
// ---------------------------------------------------------------------------
//	$Id: ui.cpp,v 1.62 2003/09/28 14:35:35 cisc Exp $

#include "headers.h"
#include <shellapi.h>
#include <mbstring.h>
#include "resource.h"
#include "ui.h"
#include "about.h"
#include "misc.h"
#include "file.h"
#include "messages.h"
#include "error.h"
#include "88config.h"
#include "status.h"
#include "pc88/opnif.h"
#include "pc88/diskmgr.h"
#include "pc88/tapemgr.h"
#include "filetest.h"
#include "winvars.h"
#include "winexapi.h"

#define LOGNAME "ui"
#include "diag.h"

extern char m88dir[MAX_PATH];
extern char m88ini[MAX_PATH];

using namespace PC8801;

// TODO: Remove this
WinUI* g_ui;

// ---------------------------------------------------------------------------
//	WinUI 
//	�����E�j��
//
WinUI::WinUI(HINSTANCE hinstance)
: hinst(hinstance), diskmgr(0), tapemgr(0)
{
	g_ui = this;
	timerid = 0;
	point.x = point.y = 0;
//	resizewindow = 0;
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

WinUI::~WinUI()
{
}

// ---------------------------------------------------------------------------
//	WinUI::InitM88
//	M88 �̏�����
//
bool WinUI::InitM88(const char* cmdline)
{
	active = false;
	tapetitle[0] = 0;
	
	//	�ݒ��݂���
	LOG1("%d\tLoadConfig\n", timeGetTime());
	PC8801::LoadConfig(&config, m88ini, true);
	
	//	���݂� path �ۑ�
	char path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, path);

	//	�f�o�C�X�̏�����
	PC8801::LoadConfigDirectory(&config, m88ini, "BIOSPath", true);
	
	LOG1("%d\tdiskmanager\n", timeGetTime());
	if (!diskmgr)
		diskmgr = new DiskManager;
	if (!diskmgr || !diskmgr->Init())
		return false;
	if (!tapemgr)
		tapemgr = new TapeManager;
	if (!tapemgr)
		return false;

	LOG1("%d\tkeyboard if\n", timeGetTime());
	if (!keyif.Init(hwnd))
		return false;
	LOG1("%d\tcore\n", timeGetTime());
	if (!core.Init(this, hwnd, &draw, diskmgr, &keyif, &winconfig, tapemgr))
		return false;

	
	//	debug �p�N���X������
	LOG1("%d\tmonitors\n", timeGetTime());
	opnmon.Init(core.GetOPN1(), core.GetSound());
	memmon.Init(&core);
	codemon.Init(&core);
	basmon.Init(&core);
	regmon.Init(&core);
	loadmon.Init();
	iomon.Init(&core);
	core.GetSound()->SetSoundMonitor(&opnmon);

	//	���s�t�@�C�����σ`�F�b�N
	LOG1("%d\tself test\n", timeGetTime());
	if (!SanityCheck())
		return false;

	//	�G�~�����[�V�����J�n
	LOG1("%d\temulation begin\n", timeGetTime());
	core.Wait(false);
	active = true;
	fullscreen = false;

	//	�ݒ蔽�f
	LOG1("%d\tapply cmdline\n", timeGetTime());
	SetCurrentDirectory(path);
	ApplyCommandLine(cmdline);
	LOG1("%d\tapply config\n", timeGetTime());
	ApplyConfig();
	
	//	���Z�b�g
	LOG1("%d\treset\n", timeGetTime());
	core.Reset();

	// ���Ƃ����Ⴒ���Ⴕ������
	LOG1("%d\tetc\n", timeGetTime());
	if (!diskinfo[0].filename[0])
		PC8801::LoadConfigDirectory(&config, m88ini, "Directory", false);
	ChangeDisplayType(true);
	ResizeWindow(640, 400);
	
	LOG1("%d\tend initm88\n", timeGetTime());
	return true;
}

// ---------------------------------------------------------------------------
//	WinUI::CleanupM88
//	M88 �̌�Еt��
//
void WinUI::CleanupM88()
{
	PC8801::Config cfg = config;
	PC8801::SaveConfig(&cfg, m88ini, true);
	core.Cleanup();
	delete diskmgr; diskmgr = 0;
	delete tapemgr; tapemgr = 0;
}

// ---------------------------------------------------------------------------
//	WinUI::InitWindow
//	M88 �̑��쐬
//
bool WinUI::InitWindow(int nwinmode)
{
	WNDCLASS wcl;
	const char* szwinname = "M88p2 WinUI";
 
	wcl.hInstance = hinst;
	wcl.lpszClassName = szwinname;
	wcl.lpfnWndProc = WNDPROC((void*)(WinProcGate));
	wcl.style = 0;
	wcl.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON_M88));
	wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = 0;
//	wcl.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wcl.hbrBackground = (HBRUSH) GetStockObject(NULL_BRUSH);
	wcl.lpszMenuName = MAKEINTRESOURCE(IDR_MENU_M88);

	accel = LoadAccelerators(hinst, MAKEINTRESOURCE(IDR_ACC_M88UI));

	if (!RegisterClass(&wcl)) 
		return false;

	wstyle = WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX;

	hwnd = CreateWindowEx(
		WS_EX_ACCEPTFILES,// | WS_EX_LAYERED,
		szwinname,
		"M88",
		wstyle,
		CW_USEDEFAULT,		// x
		CW_USEDEFAULT,		// y
		640,				// w
		400,				// h
		NULL,
		NULL,
		hinst,
		NULL
		);

//	SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0xc0, LWA_ALPHA);
	
	if (!draw.Init0(hwnd))
		return false;

	clipmode = 0;
	guimodebymouse = false;

	return true;
}

// ---------------------------------------------------------------------------
//	WinUI::Main
//	���b�Z�[�W���[�v
//
int WinUI::Main(const char* cmdline)
{
	hmenudbg = 0;
	if (InitM88(cmdline))
	{
		timerid = ::SetTimer(hwnd, 1, 1000, 0);
		::SetTimer(hwnd, 2, 100, 0);
		statusdisplay.Init(hwnd);

		ShowWindow(hwnd, SW_SHOWDEFAULT);
		UpdateWindow(hwnd);
//		core.ActivateMouse(true);
	}
	else
	{
		ReportError();
		PostQuitMessage(1);
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (winconfig.ProcMsg(msg))
		{
			if (!winconfig.IsOpen())
				SetGUIFlag(false);
			continue;
		}

		if (!TranslateAccelerator(msg.hwnd, accel, &msg))
		{
			if ((msg.message == WM_SYSKEYDOWN || msg.message == WM_SYSKEYUP)
				&& !(config.flags & Config::suppressmenu))
				TranslateMessage(&msg);
		}
		DispatchMessage(&msg);
	}

	OPNIF* opn = core.GetOPN1();
	if (opn)
		opn->Reset();
	CleanupM88();
	statusdisplay.Cleanup();
	return msg.wParam;
}

// ---------------------------------------------------------------------------
//	WinUI::WinProc
//	���b�Z�[�W�n���h��
//
#define PROC_MSG(msg, func)		case (msg): ret = (func)(hwnd, wp, lp); break

LRESULT WinUI::WinProc(HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp)
{
	uint ret;
	keyif.Disable(true);

	switch (umsg)
	{
	PROC_MSG(WM_COMMAND,			WmCommand);
	PROC_MSG(WM_PAINT,				WmPaint);
	PROC_MSG(WM_CREATE,				WmCreate);
	PROC_MSG(WM_DESTROY,			WmDestroy);
	PROC_MSG(WM_CLOSE,				WmClose);
	PROC_MSG(WM_TIMER,				WmTimer);
	PROC_MSG(WM_ACTIVATE,			WmActivate);
	PROC_MSG(WM_PALETTECHANGED,		WmPaletteChanged);
	PROC_MSG(WM_QUERYNEWPALETTE,	WmQueryNewPalette);
	PROC_MSG(WM_INITMENU,			WmInitMenu);
	PROC_MSG(WM_KEYUP,				WmKeyUp);
	PROC_MSG(WM_KEYDOWN,			WmKeyDown);
	PROC_MSG(WM_SYSKEYUP,			WmSysKeyUp);
	PROC_MSG(WM_SYSKEYDOWN,			WmSysKeyDown);
	PROC_MSG(WM_SIZE,				WmSize);
	PROC_MSG(WM_MOVE,				WmMove);
	PROC_MSG(WM_DRAWITEM,			WmDrawItem);
	PROC_MSG(WM_ENTERMENULOOP,		WmEnterMenuLoop);
	PROC_MSG(WM_EXITMENULOOP,		WmExitMenuLoop);
	PROC_MSG(WM_DISPLAYCHANGE,		WmDisplayChange);
	PROC_MSG(WM_DROPFILES,			WmDropFiles);
	PROC_MSG(WM_LBUTTONDOWN,		WmLButtonDown);
	PROC_MSG(WM_LBUTTONUP,			WmLButtonUp);
	PROC_MSG(WM_RBUTTONDOWN,		WmRButtonDown);
	PROC_MSG(WM_RBUTTONUP,			WmRButtonUp);
	PROC_MSG(WM_ENTERSIZEMOVE,		WmEnterSizeMove);
	PROC_MSG(WM_EXITSIZEMOVE,		WmExitSizeMove);
	PROC_MSG(WM_M88_SENDKEYSTATE,	M88SendKeyState);
	PROC_MSG(WM_M88_APPLYCONFIG,	M88ApplyConfig);
	PROC_MSG(WM_M88_CHANGEDISPLAY,	M88ChangeDisplay);
	PROC_MSG(WM_M88_CHANGEVOLUME,	M88ChangeVolume);
	PROC_MSG(WM_M88_CLIPCURSOR,		M88ClipCursor);
	PROC_MSG(WM_MOUSEMOVE,			WmMouseMove);
	PROC_MSG(WM_SETCURSOR,			WmSetCursor);
	
	default:
		ret = DefWindowProc(hwnd, umsg, wp, lp);
	}
	keyif.Disable(false);
	return ret;
}

LRESULT CALLBACK WinUI::WinProcGate(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	return g_ui->WinProc(hwnd, umsg, wparam, lparam);
}

// ---------------------------------------------------------------------------
//	WinUI::M88SendKeyState
//
inline uint WinUI::M88SendKeyState(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	uint8* dest = reinterpret_cast<uint8*>(wparam);
	GetKeyboardState(dest);
	SetEvent((HANDLE) lparam);
	return 0;
}

inline uint WinUI::WmKeyDown(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	if ((uint) wparam == VK_F12 && !(config.flags & Config::disablef12reset))
		;
	else
		keyif.KeyDown((uint) wparam, (uint32) lparam);
	
	return 0;
}

inline uint WinUI::WmKeyUp(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	if ((uint) wparam == VK_F12 && !(config.flags & Config::disablef12reset))
		Reset();
	else
		keyif.KeyUp((uint) wparam, (uint32) lparam);
	
	return 0;
}

inline uint WinUI::WmSysKeyDown(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	if (config.flags & Config::suppressmenu)
	{
		keyif.KeyDown((uint) wparam, (uint32) lparam);
		return 0;
	}
	return DefWindowProc(hwnd, WM_SYSKEYDOWN, wparam, lparam);
}


inline uint WinUI::WmSysKeyUp(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	if (config.flags & Config::suppressmenu)
	{
		keyif.KeyUp((uint) wparam, (uint32) lparam);
		return 0;
	}
	return DefWindowProc(hwnd, WM_SYSKEYUP, wparam, lparam);
}

// ---------------------------------------------------------------------------
//	WinUI::WmActivate
//	WM_ACTIVATE �n���h��
//
uint WinUI::WmActivate(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	bool prevbg = background;
	background = LOWORD(wparam) == WA_INACTIVE;
	
	if (!HIWORD(wparam))
	{
		draw.RequestPaint();
	}
	
	keyif.Activate(!background);
	draw.QueryNewPalette(background);
	if (prevbg != background)
	{
//		core.ActivateMouse(!background);
		M88ClipCursor(hwnd, background ? CLIPCURSOR_RELEASE : -CLIPCURSOR_RELEASE, 0);
		draw.SetGUIFlag(background);
	}
	snapshotchanged = true;
	return 0;
}

// ---------------------------------------------------------------------------
//	WinUI::WmQueryNewPalette
//	WM_QUERYNEWPALETTE �n���h��
//
uint WinUI::WmQueryNewPalette(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	draw.QueryNewPalette(background);
	return 1;
}

// ---------------------------------------------------------------------------
//	WinUI::WmPaletteChanged
//	WM_PALETTECHANGED �n���h��
//
uint WinUI::WmPaletteChanged(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	if ((HWND) wparam != hwnd)
	{
		draw.QueryNewPalette(background);
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	WinUI::WmCommand
//	WM_COMMAND �n���h��
//
uint WinUI::WmCommand(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	uint wid = LOWORD(wparam);
	switch (wid)
	{
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
		config.flags ^= PC8801::Config::specialpalette;
		ApplyConfig();
		break;

	case IDM_DEBUG_GVRAM0:
		config.flag2 ^= PC8801::Config::mask0;
		ApplyConfig();
		break;

	case IDM_DEBUG_GVRAM1:
		config.flag2 ^= PC8801::Config::mask1;
		ApplyConfig();
		break;

	case IDM_DEBUG_GVRAM2:
		config.flag2 ^= PC8801::Config::mask2;
		ApplyConfig();
		break;
		
	case IDM_STATUSBAR:
		config.flags ^= PC8801::Config::showstatusbar;
		ShowStatusWindow();
		break;

	case IDM_FDC_STATUS:
		config.flags ^= PC8801::Config::showfdcstatus;
		ApplyConfig();
		break;

	case IDM_SOUNDMON:
		opnmon.Show(hinst, hwnd, !opnmon.IsOpen());
		break;

	case IDM_MEMMON:
		memmon.Show(hinst, hwnd, !memmon.IsOpen());
		break;

	case IDM_CODEMON:
		codemon.Show(hinst, hwnd, !codemon.IsOpen());
		break;

	case IDM_BASMON:
		basmon.Show(hinst, hwnd, !basmon.IsOpen());
		break;

	case IDM_LOADMON:
		loadmon.Show(hinst, hwnd, !loadmon.IsOpen());
		break;

	case IDM_IOMON:
		iomon.Show(hinst, hwnd, !iomon.IsOpen());
		break;

	case IDM_WATCHREGISTER:
		config.flags &= ~PC8801::Config::watchregister;
		regmon.Show(hinst, hwnd, !regmon.IsOpen());
		break;

	case IDM_TAPE:
		OpenTape();
		break;

	case IDM_RECORDPCM:
		if (!core.GetSound()->IsDumping())
		{
			char buf[16];
			SYSTEMTIME t;

			GetLocalTime(&t);
			wsprintf(buf, "%.2d%.2d%.2d%.2d.wav", t.wDay, t.wHour, t.wMinute, t.wSecond);
			core.GetSound()->DumpBegin(buf);
		}
		else
		{
			core.GetSound()->DumpEnd();
		}
		break;

	case IDM_SNAPSHOT_SAVE:
		SaveSnapshot(currentsnapshot);
		break;
	case IDM_SNAPSHOT_SAVE_0:	case IDM_SNAPSHOT_SAVE_1:	case IDM_SNAPSHOT_SAVE_2:
	case IDM_SNAPSHOT_SAVE_3:	case IDM_SNAPSHOT_SAVE_4:	case IDM_SNAPSHOT_SAVE_5:
	case IDM_SNAPSHOT_SAVE_6:	case IDM_SNAPSHOT_SAVE_7:	case IDM_SNAPSHOT_SAVE_8:
	case IDM_SNAPSHOT_SAVE_9:
		SaveSnapshot(wid - IDM_SNAPSHOT_SAVE_0);
		break;
		
	case IDM_SNAPSHOT_LOAD:
		LoadSnapshot(currentsnapshot);
		break;
	case IDM_SNAPSHOT_LOAD_0:	case IDM_SNAPSHOT_LOAD_1:	case IDM_SNAPSHOT_LOAD_2:
	case IDM_SNAPSHOT_LOAD_3:	case IDM_SNAPSHOT_LOAD_4:	case IDM_SNAPSHOT_LOAD_5:
	case IDM_SNAPSHOT_LOAD_6:	case IDM_SNAPSHOT_LOAD_7:	case IDM_SNAPSHOT_LOAD_8:
	case IDM_SNAPSHOT_LOAD_9:
		LoadSnapshot(wid - IDM_SNAPSHOT_LOAD_0);
		break;

	default:
		if (IDM_CHANGEDISK_1 <= wid && wid < IDM_CHANGEDISK_1 + 64)
		{
			SelectDisk(0, wid - IDM_CHANGEDISK_1, false);
			break;
		}
		if (IDM_CHANGEDISK_2 <= wid && wid < IDM_CHANGEDISK_2 + 64)
		{
			SelectDisk(1, wid - IDM_CHANGEDISK_2, false);
			break;
		}
		break;
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	WinUI::WmPaint
//	WM_PAINT �n���h��
//
uint WinUI::WmPaint(HWND hwnd, WPARAM wp, LPARAM lp)
{
	draw.RequestPaint();
	return DefWindowProc(hwnd, WM_PAINT, wp, lp);
}

// ---------------------------------------------------------------------------
//	WinUI::WmCreate
//	WM_CREATE �n���h��
//
uint WinUI::WmCreate(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	CREATESTRUCT* cs = (CREATESTRUCT*) wparam;

	RECT rect;
	rect.left = 0;	rect.right =  640;
	rect.top  = 0;  rect.bottom = 400;
	
	AdjustWindowRectEx(&rect, wstyle, TRUE, 0);
	SetWindowPos(hwnd, 0, 0, 0, rect.right-rect.left, rect.bottom-rect.top,
				 SWP_NOMOVE | SWP_NOZORDER);
	
	(*EnableIME)(hwnd, false);
//	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	GetWindowRect(hwnd, &rect);
	point.x = rect.left; point.y = rect.top;

	LOG0("WmCreate\n");
	return 0;
}

// ---------------------------------------------------------------------------
//	WinUI::WmDestroy
//	WM_DESTROY �n���h��
//
uint WinUI::WmDestroy(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	PostQuitMessage(0);
	return 0;
}

// ---------------------------------------------------------------------------
//	WinUI::WmClose
//	WM_CLOSE �n���h��
//
uint WinUI::WmClose(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	// �m�F
	if (config.flags & Config::askbeforereset)
	{
		SetGUIFlag(true);
		int res = MessageBox(hwnd, "M88 ���I�����܂�", "M88", MB_ICONEXCLAMATION | MB_OKCANCEL | MB_DEFBUTTON2);
		SetGUIFlag(false);
		if (res != IDOK)
			return 0;
	}
	
	// �^�C�}�[�j��
	KillTimer(hwnd, timerid);
	timerid = 0;

	// �g�����j���[��j�󂷂�
	MENUITEMINFO mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = WINVAR(MIISIZE);
	mii.fMask = MIIM_SUBMENU;
	mii.hSubMenu = 0;
	
	SetMenuItemInfo(GetMenu(hwnd), IDM_DRIVE_1, false, &mii);
	SetMenuItemInfo(GetMenu(hwnd), IDM_DRIVE_2, false, &mii);
	SetMenuItemInfo(GetMenu(hwnd), IDM_SNAPSHOT_LOAD, false, &mii);
	SetMenuItemInfo(GetMenu(hwnd), IDM_SNAPSHOT_SAVE, false, &mii);
	
	int i;
	for (i=0; i<2; i++)
	{
		if (diskinfo[i].hmenu)
			DestroyMenu(diskinfo[i].hmenu), diskinfo[i].hmenu = 0;
	}
	for (i=0; i<2; i++)
	{
		if (hmenuss[i])
			DestroyMenu(hmenuss[i]), hmenuss[i] = 0;
	}

	// �������
	DestroyWindow(hwnd);
	active = false;
	
	return 0;
}

// ---------------------------------------------------------------------------
//	WinUI::WmTimer
//	WM_TIMER �n���h��
//
uint WinUI::WmTimer(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	LOG2("WmTimer:%d(%d)\n", wparam, timerid);
	if (wparam == timerid)
	{
		// �������g��,�\���t���[�������擾
		int	fcount = draw.GetDrawCount();
		int	icount = core.GetExecCount();
		
		// ���|�[�g����ꍇ�̓^�C�g���o�[���X�V
		if (report)
		{
			if (active)
			{
				char buf[64];
				uint freq = icount / 10000;
				wsprintf(buf, "M88 - %d fps.  %d.%.2d MHz", 
					fcount, freq / 100, freq % 100);
				SetWindowText(hwnd, buf);
			}
			else
				SetWindowText(hwnd, "M88");
		}

		if (resetwindowsize > 0)
		{
			resetwindowsize--;
			ResizeWindow(640, 400);
		}
		return 0;
	}
	if (wparam == 2)
	{
		KillTimer(hwnd, 2);
		InvalidateRect(hwnd, 0, false);
		return 0;
	}
	if (wparam == statusdisplay.GetTimerID())
	{
		statusdisplay.Update();
		return 0;
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	WinUI::WmInitMenu
//	WM_INITMENU �n���h��
//
uint WinUI::WmInitMenu(HWND hwnd, WPARAM wp, LPARAM lp)
{
	HMENU hmenu = (HMENU) wp;
#ifndef DEBUG_MONITOR
	EnableMenuItem(hmenu, IDM_LOGSTART, MF_GRAYED);
	EnableMenuItem(hmenu, IDM_LOGEND, MF_GRAYED);
#endif
	CheckMenuItem(hmenu, IDM_N88V1, (config.basicmode == Config::N88V1) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_N88V1H,(config.basicmode == Config::N88V1H)? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_N88V2, (config.basicmode == Config::N88V2) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_NMODE, (config.basicmode == Config::N80)   ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_N80MODE, (config.basicmode == Config::N802)? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_N80V2MODE, (config.basicmode == Config::N80V2)? MF_CHECKED : MF_UNCHECKED);
	
	CheckMenuItem(hmenu, IDM_N88V2CD, (config.basicmode == Config::N88V2CD) ? MF_CHECKED : MF_UNCHECKED);
	EnableMenuItem(hmenu, IDM_N88V2CD, core.IsCDSupported() ? MF_ENABLED : MF_GRAYED);

	CheckMenuItem(hmenu, IDM_WATCHREGISTER, (config.dipsw != 1 && regmon.IsOpen()) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_STATUSBAR, (config.flags & Config::showstatusbar) ? MF_CHECKED : MF_UNCHECKED);
	EnableMenuItem(hmenu, IDM_STATUSBAR, fullscreen ? MF_GRAYED : MF_ENABLED);
	CheckMenuItem(hmenu, IDM_FDC_STATUS, (config.flags & Config::showfdcstatus) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_SOUNDMON, opnmon.IsOpen() ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_MEMMON, memmon.IsOpen() ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_CODEMON, codemon.IsOpen() ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_BASMON, basmon.IsOpen() ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_LOADMON, loadmon.IsOpen() ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_IOMON, iomon.IsOpen() ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hmenu, IDM_RECORDPCM, core.GetSound()->IsDumping() ? MF_CHECKED : MF_UNCHECKED);
	
	EnableMenuItem(hmenu, IDM_DUMPCPU1, core.GetCPU1()->GetDumpState() == -1 ? MF_GRAYED : MF_ENABLED);
	CheckMenuItem(hmenu, IDM_DUMPCPU1, core.GetCPU1()->GetDumpState() == 1 ? MF_CHECKED : MF_UNCHECKED);
	EnableMenuItem(hmenu, IDM_DUMPCPU2, core.GetCPU2()->GetDumpState() == -1 ? MF_GRAYED : MF_ENABLED);
	CheckMenuItem(hmenu, IDM_DUMPCPU2, core.GetCPU2()->GetDumpState() == 1 ? MF_CHECKED : MF_UNCHECKED);
	
	if (hmenudbg)
	{
		CheckMenuItem(hmenudbg, IDM_DEBUG_TEXT, (config.flags & Config::specialpalette) ? MF_CHECKED : MF_UNCHECKED);
		int mask = (config.flag2 / Config::mask0) & 7;
		CheckMenuItem(hmenudbg, IDM_DEBUG_GVRAM0, (mask & 1) ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenudbg, IDM_DEBUG_GVRAM1, (mask & 2) ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(hmenudbg, IDM_DEBUG_GVRAM2, (mask & 4) ? MF_CHECKED : MF_UNCHECKED);
	}

	MakeSnapshotMenu();
	return 0;
}


// ---------------------------------------------------------------------------
//	WinUI::WmSize
//	WM_SIZE
//
uint WinUI::WmSize(HWND hwnd, WPARAM wp, LPARAM lp)
{
	HWND hwndstatus = statusdisplay.GetHWnd();
	if (hwndstatus)
		PostMessage(hwndstatus, WM_SIZE, wp, lp);
	active = wp != SIZE_MINIMIZED;
	draw.Activate(active);
	return DefWindowProc(hwnd, WM_SIZE, wp, lp);
}

// ---------------------------------------------------------------------------
//	WinUI::ReportError
//	�G���[�\��
//
void WinUI::ReportError()
{
	const char* errtext = Error::GetErrorText();
	if (errtext)
	{
		::MessageBox(hwnd, errtext, "M88", MB_ICONERROR | MB_OK);
	}
}

// ---------------------------------------------------------------------------
//	WinUI::ReportError
//
uint WinUI::M88ApplyConfig(HWND, WPARAM newconfig, LPARAM)
{
	if (newconfig)
	{
		// ���\�ł��ȁB
		if (memcmp(&config, (PC8801::Config*)newconfig, sizeof(PC8801::Config)))
		{
			config = *((PC8801::Config*)newconfig);
			ApplyConfig();
		}
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	WinUI::ApplyConfig
//
void WinUI::ApplyConfig()
{
	config.mainsubratio = 
		(config.clock >= 60 || (config.flags & Config::fullspeed)) ? 2 : 1;
	if (config.dipsw != 1)
	{
		config.flags &= ~Config::specialpalette;
		config.flag2 &= ~(Config::mask0 | Config::mask1 | Config::mask2);
	}
		
	core.ApplyConfig(&config);
	keyif.ApplyConfig(&config);
	draw.SetPriorityLow((config.flags & Config::drawprioritylow) != 0);

	MENUITEMINFO mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = WINVAR(MIISIZE);
	mii.fMask = MIIM_TYPE;
	mii.fType = MFT_STRING;
	mii.dwTypeData = (config.flags & Config::disablef12reset) ? "&Reset" : "&Reset\tF12";
	SetMenuItemInfo(GetMenu(hwnd), IDM_RESET, false, &mii);
	ShowStatusWindow();

	if (config.dipsw == 1)
	{
		if (!hmenudbg)
		{
			hmenudbg = LoadMenu(hinst, MAKEINTRESOURCE(IDR_MENU_DEBUG));

			mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
			mii.fType = MFT_STRING;
			mii.dwTypeData = "Control &Plane";
			mii.hSubMenu = hmenudbg;
			SetMenuItemInfo(GetMenu(hwnd), IDM_WATCHREGISTER, false, &mii);
		}
	}
	else
	{
		mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
		mii.fType = MFT_STRING;
		mii.dwTypeData = "Show &Register";
		mii.hSubMenu = 0;
		SetMenuItemInfo(GetMenu(hwnd), IDM_WATCHREGISTER, false, &mii);
		hmenudbg = 0;
	}
}

// ---------------------------------------------------------------------------
//	WinUI::Reset
//
void WinUI::Reset()
{
	if (config.flags & Config::askbeforereset)
	{
		SetGUIFlag(true);
		int res = MessageBox(hwnd, "���Z�b�g���܂����H", "M88", MB_ICONQUESTION | MB_OKCANCEL | MB_DEFBUTTON2);
		SetGUIFlag(false);
		if (res != IDOK)
			return;
	}
	keyif.ApplyConfig(&config);
	core.ApplyConfig(&config);
	core.Reset();
}


// ---------------------------------------------------------------------------
//	ChangeDiskImage
//	�f�B�X�N����ւ�
//
void WinUI::ChangeDiskImage(HWND hwnd, int drive)
{
	HANDLE hthread = GetCurrentThread();
	int prev = GetThreadPriority(hthread);
	SetThreadPriority(hthread, THREAD_PRIORITY_ABOVE_NORMAL);
	
	SetGUIFlag(true);
	
	if (!diskmgr->Unmount(drive))
	{
		MessageBox(hwnd, 
			"DiskManger::Unmount failed\n�f�B�X�N�̎��O���Ɏ��s���܂���.",
			"M88",
			MB_ICONERROR | MB_OK);
	}

	OFNV5 ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = WINVAR(OFNSIZE);
	ofn.FlagsEx = config.flag2 & Config::showplacesbar ? 0 : OFN_EX_NOPLACESBAR;

	char filename[MAX_PATH];
	filename[0] = 0;
	
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = "8801 disk image (*.d88)\0*.d88\0"
					  "All Files (*.*)\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_CREATEPROMPT | OFN_SHAREAWARE;
	ofn.lpstrDefExt = "d88";
	ofn.lpstrTitle = "Open disk image";
	
	(*EnableIME)(hwnd, true);
	bool isopen = !!GetOpenFileName(&ofn);
	(*EnableIME)(hwnd, false);

	if (isopen)
	{
		// �w�肳�ꂽ�t�@�C���͑��݂��邩�H
		bool createnew = false;
		if (!diskmgr->IsImageOpen(filename))
		{
			FileIO file;
			if (!file.Open(filename, FileIO::readonly))
			{
				if (file.GetError() == FileIO::file_not_found)
				{
					// �t�@�C�������݂��Ȃ�
					createnew = true;
					if (!newdisk.Show(hinst, hwnd))
						return;
				}
				else
				{
					// ���炩�̗��R�ŃA�N�Z�X�ł��Ȃ�
					return;
				}
			}
		}

		OpenDiskImage(drive, filename, ofn.Flags & OFN_READONLY, 0, createnew);

		if (createnew && diskmgr->GetNumDisks(drive) == 0)
		{
			diskmgr->AddDisk(drive, newdisk.GetTitle(), newdisk.GetType());
			OpenDiskImage(drive, filename, 0, 0, false);
			if (newdisk.DoFormat())
				diskmgr->FormatDisk(drive);
		}
		if (drive == 0 && !diskinfo[1].filename[0] && diskmgr->GetNumDisks(0) > 1)
		{
			OpenDiskImage(1, filename, ofn.Flags & OFN_READONLY, 1, false);
		}
	}
	else
		OpenDiskImage(drive, 0, 0, 0, false);
	
	SetGUIFlag(false);
	SetThreadPriority(hthread, prev);
	snapshotchanged = true;
}

// ---------------------------------------------------------------------------
//	�K���Ƀf�B�X�N�C���[�W���J��
//
void WinUI::OpenDiskImage(const char* path)
{
	// �f�B�X�N�C���[�W���}�E���g����
	OpenDiskImage(0, path, 0, 0, false);
	if (diskmgr->GetNumDisks(0) > 1)
	{
		OpenDiskImage(1, path, 0, 1, false);
	}
	else
	{
		diskmgr->Unmount(1);
		OpenDiskImage(1, 0, 0, 0, false);
	}
}

// ---------------------------------------------------------------------------
//	�t�@�C���l�[���̕��������o��
//
static void GetFileNameTitle(char* title, const char* name)
{
	if (name)
	{
		uchar* ptr;
		ptr = _mbsrchr((uchar*) name, '\\');
		_mbscpy((uchar*) title,  ptr ? ptr+1 : (uchar*)(name));
		ptr = _mbschr((uchar*) title, '.');
		if (ptr)
			*ptr = 0;
	}
}

// ---------------------------------------------------------------------------
//	OpenDiskImage
//	�f�B�X�N�C���[�W���J��
//
bool WinUI::OpenDiskImage
(int drive, const char* name, bool readonly, int id, bool create)
{
	DiskInfo& dinfo = diskinfo[drive];

	bool result = false;
	if (name)
	{
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
//	SelectDisk
//	�f�B�X�N�Z�b�g
//
bool WinUI::SelectDisk(uint drive, int id, bool menuonly)
{
	DiskInfo& dinfo = diskinfo[drive];
	if (drive >= 2 || id >= 64)
		return false;

	CheckMenuItem(dinfo.hmenu,
		dinfo.idchgdisk+(dinfo.currentdisk < 0 ? 63 : dinfo.currentdisk),
		MF_BYCOMMAND | MF_UNCHECKED);
	
	bool result = true;
	if (!menuonly)
		result = diskmgr->Mount(drive, dinfo.filename, dinfo.readonly, id, false);
	
	int current = result ? diskmgr->GetCurrentDisk(drive) : -1;
	
	CheckMenuItem(dinfo.hmenu,
				  dinfo.idchgdisk+(current < 0 ? 63 : current),
				  MF_BYCOMMAND | MF_CHECKED);
	dinfo.currentdisk = current;
	return true;
}

// ---------------------------------------------------------------------------
//	CreateDiskMenu
//	�}���`�f�B�X�N�C���[�W�p���j���[�̍쐬
//
bool WinUI::CreateDiskMenu(uint drive)
{
	char buf[MAX_PATH + 16];
	
	DiskInfo& dinfo = diskinfo[drive];
	HMENU hmenuprev = dinfo.hmenu;
	dinfo.currentdisk = -1;

	int ndisks = Min(diskmgr->GetNumDisks(drive), 60);
	if (ndisks)
	{
		// ���j���[�쐬
		dinfo.hmenu = CreatePopupMenu();
		if (!dinfo.hmenu)
			return false;
		
		for (int i=0; i<ndisks; i++)
		{
			const char* title = diskmgr->GetImageTitle(drive, i);
			
			if (!title) break;
			if (!title[0]) title = "(untitled)";
			
			wsprintf(buf, i < 9 ? "&%d %.16s" : "%d %.16s", i+1, title);
			AppendMenu(	dinfo.hmenu,
						MF_STRING | (i && !(i % 20) ? MF_MENUBARBREAK : 0),
						dinfo.idchgdisk + i,
						buf );
		}

		AppendMenu(dinfo.hmenu, MF_SEPARATOR, 0, 0);
		AppendMenu(dinfo.hmenu, MF_STRING, dinfo.idchgdisk + 63, "&N No disk");
		AppendMenu(dinfo.hmenu, MF_STRING, dinfo.idchgdisk + 64, "&0 Change disk");
		SetMenuDefaultItem(dinfo.hmenu, dinfo.idchgdisk + 64, FALSE );
	}

	MENUITEMINFO mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = WINVAR(MIISIZE);
	mii.fType = MFT_STRING;
	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.dwTypeData = buf;

	if (!ndisks)
	{
		wsprintf(buf, "Drive &%d...", drive+1);
		mii.hSubMenu = 0;
	}
	else
	{
		char title[MAX_PATH] = "";
		GetFileNameTitle(title, diskinfo[drive].filename);
	
		wsprintf(buf, "Drive &%d - %s", drive+1, title);
		mii.hSubMenu = dinfo.hmenu;
	}
	SetMenuItemInfo(GetMenu(hwnd), drive ? IDM_DRIVE_2 : IDM_DRIVE_1, false, &mii);
	if (hmenuprev)
		DestroyMenu(hmenuprev);
	
	if (ndisks)
		SelectDisk(drive, dinfo.currentdisk, true);

	return true;
}


// ---------------------------------------------------------------------------
//	OpenTape
//	�f�B�X�N����ւ�
//
void WinUI::OpenTape()
{
	HANDLE hthread = GetCurrentThread();
	int prev = GetThreadPriority(hthread);
	SetThreadPriority(hthread, THREAD_PRIORITY_ABOVE_NORMAL);
	
	SetGUIFlag(true);
	
	tapemgr->Close();

	OFNV5 ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = WINVAR(OFNSIZE);
	ofn.FlagsEx = config.flag2 & Config::showplacesbar ? 0 : OFN_EX_NOPLACESBAR;

	char filename[MAX_PATH];
	filename[0] = 0;
	
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = "T88 tape image (*.t88)\0*.t88\0"
					  "All Files (*.*)\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_CREATEPROMPT | OFN_SHAREAWARE;
	ofn.lpstrDefExt = "t88";
	ofn.lpstrTitle = "Open tape image";
	
	(*EnableIME)(hwnd, true);
	bool isopen = !!GetOpenFileName(&ofn);
	(*EnableIME)(hwnd, false);

	char buf[MAX_PATH+32];
	MENUITEMINFO mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = WINVAR(MIISIZE);
	mii.fMask = MIIM_TYPE;
	mii.fType = MFT_STRING;
		
	if (isopen && tapemgr->Open(filename))
	{
		GetFileNameTitle(tapetitle, filename);
		wsprintf(buf, "&Tape - %s...", tapetitle);
		mii.dwTypeData = buf;
	}
	else
	{
		mii.dwTypeData = "&Tape...";
	}
	SetMenuItemInfo(GetMenu(hwnd), IDM_TAPE, false, &mii);
	
	SetGUIFlag(false);
	SetThreadPriority(hthread, prev);
	snapshotchanged = true;
}

// ---------------------------------------------------------------------------
//	WinUI::ResizeWindow
//	�E�B���h�E�̑傫����ς���
//
void WinUI::ResizeWindow(uint width, uint height)
{
	RECT rect;
	rect.left = 0;	rect.right = width;
	rect.top  = 0;  rect.bottom = height + statusdisplay.GetHeight();
	
	AdjustWindowRectEx(&rect, wstyle, TRUE, 0);
	SetWindowPos(hwnd, 0, 0, 0, rect.right-rect.left, rect.bottom-rect.top,
				 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	PostMessage(hwnd, WM_SIZE, SIZE_RESTORED, MAKELONG(width, height));
}

// ---------------------------------------------------------------------------
//	�X�e�[�^�X�o�[�\���؂�ւ�
//
void WinUI::ShowStatusWindow()
{
	if (!fullscreen)
	{
		if (config.flags & PC8801::Config::showstatusbar)
			statusdisplay.Enable((config.flags & PC8801::Config::showfdcstatus) != 0);
		else
			statusdisplay.Disable();
		ResizeWindow(640, 400);
	}
}

// ---------------------------------------------------------------------------
//	WinUI::WmDrawItem
//	WM_DRAWITEM �n���h��
//
uint WinUI::WmDrawItem(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	if ((UINT) wparam == 1)
		statusdisplay.DrawItem((DRAWITEMSTRUCT*)lparam);
	return TRUE;
}

// ---------------------------------------------------------------------------
//	WinUI::ToggleDisplayMode
//	�S��ʁE�E�B���h�E�\���ؑ�  (ALT+ENTER)
//
void WinUI::ToggleDisplayMode()
{
	uint tick = GetTickCount();
	if ((tick - displaychangedtime) < 1000)
		return;

	displaychangedtime = GetTickCount();
	fullscreen = !fullscreen;

	if (fullscreen)
		statusdisplay.Disable();
	ChangeDisplayType(fullscreen);
}

// ---------------------------------------------------------------------------
//	WinUI::ChangeDisplayType
//	�\�����\�b�h�ύX
//
void WinUI::ChangeDisplayType(bool savepos)
{
	if (winconfig.IsOpen())
	{
		winconfig.Close();
		SetGUIFlag(false);
	}
	if (savepos)
	{
		RECT rect;
		GetWindowRect(hwnd, &rect);
		point.x = rect.left; point.y = rect.top;
	}
	PostMessage(hwnd, WM_M88_CHANGEDISPLAY, 0, 0);
}

// ---------------------------------------------------------------------------
//	WinUI::M88ChangeDisplay
//	�\�����\�b�h�̕ύX
//
uint WinUI::M88ChangeDisplay(HWND hwnd, WPARAM, LPARAM)
{
	// ��ʃh���C�o�̐ؑւ�
	// �h���C�o�� false ��Ԃ����ꍇ GDI �h���C�o���g�p����邱�ƂɂȂ�
	if (!draw.ChangeDisplayMode
				(fullscreen, (config.flags & PC8801::Config::force480) != 0))
		fullscreen = false;

	// �E�B���h�E�X�^�C���֌W�̕ύX
	wstyle = GetWindowLong(hwnd, GWL_STYLE);
	int exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);

	if (!fullscreen)
	{
		wstyle = (wstyle & ~WS_POPUP) | (WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU);
		exstyle &= ~WS_EX_TOPMOST;
		
//		SetCapture(hwnd);
		SetWindowLong(hwnd, GWL_STYLE, wstyle);
		SetWindowLong(hwnd, GWL_EXSTYLE, exstyle);
		ResizeWindow(640, 400);
		SetWindowPos(hwnd, HWND_NOTOPMOST, point.x, point.y, 0, 0, SWP_NOSIZE);
		ShowStatusWindow();
		report = true;
		guimodebymouse = false;
	}
	else
	{
		if (guimodebymouse)
			SetGUIFlag(false);
			
//	ReleaseCapture();
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
//	WinUI::WmEnterMenuLoop
//	WM_ENTERMENULOOP �n���h��
//
uint WinUI::WmEnterMenuLoop(HWND, WPARAM wp, LPARAM)
{
	if (!wp)
	{
		keyif.Activate(false);
		SetGUIFlag(true);
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	WinUI::WmExitMenuLoop
//	WM_EXITMENULOOP �n���h��
//
uint WinUI::WmExitMenuLoop(HWND, WPARAM wp, LPARAM)
{
	if (!wp)
	{
		keyif.Activate(true);
		SetGUIFlag(false);
	}
	return 0;
}

// ---------------------------------------------------------------------------
//	�{�����[���ύX
//
uint WinUI::M88ChangeVolume(HWND, WPARAM c, LPARAM)
{
	if (c)
		core.SetVolume((PC8801::Config*) c);
	return 0;
}

// ---------------------------------------------------------------------------
//	WinUI::WmDisplayChange
//	WM_DISPLAYCHANGE �n���h��
//
uint WinUI::WmDisplayChange(HWND hwnd, WPARAM wp, LPARAM lp)
{
	resetwindowsize = fullscreen ? 0 : 5;
	return 0;
}

// ---------------------------------------------------------------------------
//	WinUI::DropFiles
//	WM_DROPFILES �n���h��
//	�ߌ�s���̎��������Ƃɍ쐬���܂����D
//
uint WinUI::WmDropFiles(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	HDROP hdrop = (HDROP) wparam;

	// �󂯎�����t�@�C���̐����m�F
	int nfiles = DragQueryFile(hdrop, ~0, 0, 0);
	if (nfiles != 1)
	{
		statusdisplay.Show(50, 3000, "�h���b�v�ł���̂̓t�@�C���P�����ł�.");
		DragFinish(hdrop);
		return 0;
	}

	// �t�@�C���l�[�����擾
	char path[MAX_PATH];
	DragQueryFile(hdrop, 0, path, 512);
	DragFinish(hdrop);
	
	OpenDiskImage(path);
	
	// ����Ƀ}�E���g�ł����烊�Z�b�g
	if (diskinfo[0].filename[0])
		Reset();

	return 0;
}

// ---------------------------------------------------------------------------
//	WinUI::CaptureScreen
//
void WinUI::CaptureScreen()
{
	int bmpsize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) 
					  + 15 * sizeof(RGBQUAD) + 320 * 400;
	uint8* bmp = new uint8[bmpsize];
	if (!bmp) return;
	
	bmpsize = draw.CaptureScreen(bmp);
	if (bmpsize)
	{
		bool save;
		char filename[MAX_PATH];
		
		if (config.flag2 & Config::genscrnshotname)
		{
			SYSTEMTIME time;
			GetLocalTime(&time);
			wsprintf(filename, "%.2d%.2d%.2d%.2d%.2d.bmp", 
					 time.wDay, time.wHour, time.wMinute, time.wSecond,
					 time.wMilliseconds / 10);
			save = true;
			statusdisplay.Show(80, 1500, "��ʃC���[�W�� %s �ɕۑ����܂���", filename);
		}
		else
		{
			filename[0] = 0;
			
			OFNV5 ofn;
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = WINVAR(OFNSIZE);
			ofn.hwndOwner = hwnd;
			ofn.lpstrFilter = "bitmap image [4bpp] (*.bmp)\0*.bmp\0";
			ofn.lpstrFile = filename;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_CREATEPROMPT | OFN_NOREADONLYRETURN;
			ofn.lpstrDefExt = "bmp";
			ofn.lpstrTitle = "Save captured image";
			ofn.FlagsEx = config.flag2 & Config::showplacesbar ? 0 : OFN_EX_NOPLACESBAR;
			
			SetGUIFlag(true);
			(*EnableIME)(hwnd, true);
			save = !!GetSaveFileName(&ofn);
			(*EnableIME)(hwnd, false);
			SetGUIFlag(false);
		}
		if (save)
		{
			FileIO file;
			if (file.Open(filename, FileIO::create))
				file.Write(bmp, bmpsize);
		}
	}
	delete[] bmp;
}

// ---------------------------------------------------------------------------
//	�}�E�X��Ԃ̎擾
//
uint WinUI::WmLButtonDown(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	if (capturemouse)
	{
		mousebutton |= 1;
		return 0;
	}
	return DefWindowProc(hwnd, WM_LBUTTONDOWN, wparam, lparam);
}

uint WinUI::WmLButtonUp(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	if (capturemouse)
	{
		mousebutton &= ~1;
		return 0;
	}
	return DefWindowProc(hwnd, WM_LBUTTONDOWN, wparam, lparam);
}

uint WinUI::WmRButtonDown(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	if (capturemouse)
	{
		mousebutton |= 2;
		return 0;
	}
	return DefWindowProc(hwnd, WM_LBUTTONDOWN, wparam, lparam);
}

uint WinUI::WmRButtonUp(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	if (capturemouse)
	{
		mousebutton &= ~2;
		return 0;
	}
	return DefWindowProc(hwnd, WM_LBUTTONDOWN, wparam, lparam);
}

// ---------------------------------------------------------------------------
//	�E�B���h�E�ړ����[�h�ɓ���E�o��
//
uint WinUI::WmEnterSizeMove(HWND hwnd, WPARAM, LPARAM)
{
//	core.ActivateMouse(false);
	return 0;
}

uint WinUI::WmExitSizeMove(HWND hwnd, WPARAM, LPARAM)
{
//	core.ActivateMouse(true);
	return 0;
}

// ---------------------------------------------------------------------------
//	WM_MOVE
//
uint WinUI::WmMove(HWND hwnd, WPARAM, LPARAM)
{
	POINT srcpoint;
	srcpoint.x = 0, srcpoint.y = 0;
	ClientToScreen(hwnd, &srcpoint);

	draw.WindowMoved(srcpoint.x, srcpoint.y);
	return 0;
}

// ---------------------------------------------------------------------------
//	WM_M88_CLIPCURSOR
//
uint WinUI::M88ClipCursor(HWND hwnd, WPARAM op, LPARAM)
{
	if (int(op) > 0)
		clipmode |= op;
	else
		clipmode &= ~(-op);

	if (clipmode && !(clipmode & CLIPCURSOR_RELEASE))
	{
		RECT rect;
		POINT center;
		GetWindowRect(hwnd, &rect);

		if (clipmode & CLIPCURSOR_MOUSE)
		{
			center.x = (rect.left + rect.right) / 2;
			center.y = (rect.top + rect.bottom) / 2;
			rect.left   = center.x - 180;
			rect.right  = center.x + 180;
			rect.top    = center.y - 180;
			rect.bottom = center.y + 180;
		}				
		LOG4("rect: %d %d %d %d\n", rect.left, rect.top, rect.right, rect.bottom);
		ClipCursor(&rect);
	}
	else
	{
		ClipCursor(0);
	}
	return 1;
}

uint WinUI::WmMouseMove(HWND hwnd, WPARAM wp, LPARAM lp)
{
/*	if (fullscreen)
	{
		POINTS p;
		p = MAKEPOINTS(lp);
		uint menu = p.y < 8;
		if (!guimodebymouse)
		{
			if (menu)
			{
				guimodebymouse = true;
				SetGUIFlag(true);
			}
		}
		else
		{
			if (!menu)
			{
				guimodebymouse = false;
				SetGUIFlag(false);
			}
		}
	}
*/	return 0;

}

uint WinUI::WmSetCursor(HWND hwnd, WPARAM wp, LPARAM lp)
{
	if (fullscreen)
	{
		uint menu = LOWORD(lp) == HTMENU;
		if (!guimodebymouse)
		{
			if (menu)
			{
				guimodebymouse = true;
				SetGUIFlag(true);
			}
		}
		else
		{
			if (!menu)
			{
				guimodebymouse = false;
				SetGUIFlag(false);
			}
		}
	}
//	statusdisplay.Show(0, 0, "%d", LOWORD(lp));
	return DefWindowProc(hwnd, WM_SETCURSOR, wp, lp);
}

// ---------------------------------------------------------------------------
//	GUI ���샂�[�h�ɓ���
//
void WinUI::SetGUIFlag(bool gui)
{
	if (gui && !background)
	{
		if (!background)
			::DrawMenuBar(hwnd);
	}
//	core.SetGUIFlag(gui);
	draw.SetGUIFlag(gui);
}

// ---------------------------------------------------------------------------
//	�X�i�b�v�V���b�g�̖��O�����
//
void WinUI::GetSnapshotName(char* name, int n)
{
	char buf[MAX_PATH];
	char* title;

	if (diskmgr->GetNumDisks(0))
	{
		GetFileNameTitle(buf, diskinfo[0].filename);
		title = buf;
	}
	else if (tapemgr->IsOpen())
	{
		title = tapetitle;
	}
	else
	{
		title = "snapshot";
	}
	
	if (n >= 0)
		wsprintf(name, "%s_%d.s88", title, n);
	else
		wsprintf(name, "%s_?.s88", title);
}

// ---------------------------------------------------------------------------
//	�X�i�b�v�V���b�g�̏�����
//
void WinUI::SaveSnapshot(int n)
{
	char name[MAX_PATH];
	GetSnapshotName(name, n);
	if (core.SaveShapshot(name))
		statusdisplay.Show(80, 3000, "%s �ɕۑ����܂���", name);
	else
		statusdisplay.Show(80, 3000, "%s �ɕۑ��ł��܂���", name);
	currentsnapshot = n;
	snapshotchanged = true;
}

// ---------------------------------------------------------------------------
//	�X�i�b�v�V���b�g�̓ǂݍ���
//
void WinUI::LoadSnapshot(int n)
{
	char name[MAX_PATH];
	GetSnapshotName(name, n);
	bool r;
	if (diskinfo[0].filename && diskmgr->GetNumDisks(0) >= 2)
	{
		OpenDiskImage(1, diskinfo[0].filename, diskinfo[0].readonly, 1, false);
		r = core.LoadShapshot(name, diskinfo[0].filename);
	}
	else
	{
		r = core.LoadShapshot(name, 0);
	}

	if (r)
		statusdisplay.Show(80, 2500, "%s ���畜�����܂���", name);
	else
		statusdisplay.Show(80, 2500, "%s ���畜���ł��܂���", name);
	for (uint i=0; i<2; i++)
		CreateDiskMenu(i);
	currentsnapshot = n;
	snapshotchanged = true;
}	

// ---------------------------------------------------------------------------
//	�X�i�b�v�V���b�g�p�̃��j���[���쐬
//
bool WinUI::MakeSnapshotMenu()
{
	if (snapshotchanged)
	{
		int i;
		snapshotchanged = false;

		// ���j���[�����ɖ߂�
		MENUITEMINFO mii;
		memset(&mii, 0, sizeof(mii));
		mii.cbSize = WINVAR(MIISIZE);
		mii.fMask = MIIM_SUBMENU;
		mii.hSubMenu = 0;
		SetMenuItemInfo(GetMenu(hwnd), IDM_SNAPSHOT_LOAD, false, &mii);
		SetMenuItemInfo(GetMenu(hwnd), IDM_SNAPSHOT_SAVE, false, &mii);
		
		// ���j���[�쐬
		for (i=0; i<2; i++)
		{
			if (hmenuss[i])
				DestroyMenu(hmenuss[i]);
			hmenuss[i] = CreatePopupMenu();
			if (!hmenuss[i])
				return false;
		}
		
		// �X�i�b�v�V���b�g������
		int entries = 0;
		FileFinder ff;
		char buf[MAX_PATH];

		GetSnapshotName(buf, -1);
		int p = strlen(buf) - 5;
		ff.FindFile(buf);
		while (ff.FindNext())
		{
			int n = ff.GetFileName()[p] - '0';
			if (0 <= n && n <= 9)
				entries |= 1 << n;
		}

		// ���j���[�쐬
		for (i=0; i<10; i++)
		{
			wsprintf(buf, "&%d", i);
			AppendMenu(hmenuss[0], MF_STRING, IDM_SNAPSHOT_SAVE_0 + i, buf);
			
			if (entries & (1 << i))
			{
				CheckMenuItem(hmenuss[0], IDM_SNAPSHOT_SAVE_0 + i, MF_BYCOMMAND | MF_CHECKED);
				AppendMenu(hmenuss[1], MF_STRING, IDM_SNAPSHOT_LOAD_0 + i, buf);
				if (i == currentsnapshot)
					SetMenuDefaultItem(hmenuss[1], IDM_SNAPSHOT_LOAD_0 + i, FALSE );
			}
		}
		SetMenuDefaultItem(hmenuss[0], IDM_SNAPSHOT_SAVE_0 + currentsnapshot, FALSE );
		
		mii.hSubMenu = hmenuss[0];
		SetMenuItemInfo(GetMenu(hwnd), IDM_SNAPSHOT_SAVE, false, &mii);
		if (entries)
		{
			mii.hSubMenu = hmenuss[1];
			SetMenuItemInfo(GetMenu(hwnd), IDM_SNAPSHOT_LOAD, false, &mii);
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
//	�R�}���h���C������
//
//	����:
//	M88 [-flags] diskimagepath
//
//	-bN		basic mode (hex)
//	-fA,B	flags (16�i)	(A=flags�̒��g, B=���f������r�b�g)
//	-gA,B	flag2 (16�i)	(A=flag2�̒��g, B=���f������r�b�g)
//	-cCLK	clock (MHz) (10�i)
//	-F		�t���X�N���[���N��
//
//	N, A, B �̒l�� src/pc88/config.h ���Q�ƁD
//	
//	���o�[�W�����ł͐ݒ�_�C�A���O�ł͐ݒ�ł��Ȃ��g�ݍ��킹��
//	�󂯕t���Ă��܂��̂ŗv����
//	�Ⴆ�΃}�E�X���g�p���Ȃ���L�[�ɂ�郁�j���[�}���Ȃ񂩂̓��o�C�����D
//
//	��:	M88 -b31 -c8 -f10,10 popfulmail.d88
//		V2 ���[�h�C8MHz�COPNA ���[�h�� popfulmail.d88 ���N������
//	
//	������������Â炢�͕̂S�����m�ł�(^^;
//	�ڂ����͉����Q�Ƃ��邩�C�J�����ɂł����₵�Ă��������D
//
//	���̃p�����[�^���ύX�ł���悤�ɂ������ꍇ���C�ꌾ������ΑΉ����܂��D
//
void WinUI::ApplyCommandLine(const char* cmdline)
{
	bool change = false;
	while (*cmdline)
	{
		while (*cmdline == ' ')
			cmdline++;

		if (*cmdline == '-')
		{
			cmdline += 2;
			switch (tolower(cmdline[-1]))
			{
				char* endptr;
				int32 newflags, activate;

			// BASIC ���[�h��ݒ�  -b���[�h�ԍ�
			case 'b':
				config.basicmode =
					Config::BASICMode(strtoul(cmdline, &endptr, 16));
				cmdline = endptr;
				change = true;
				break;

			// clock ��ݒ�  -c�N���b�N
			case 'c':
				config.clock = Limit(strtoul(cmdline, &endptr, 10), 100, 1) * 10;
				cmdline = endptr;
				change = true;
				break;

			// �t���X�N���[���N��
			case 'F':
				fullscreen = true;
				break;

			// flags �̒l��ݒ�  -g�l,�L���r�b�g
			case 'f':
				newflags = strtoul(cmdline, &endptr, 16);
				activate = ~0;
				if (*endptr == ',')
				{
					activate = strtoul(endptr+1, &endptr, 16);
				}
				cmdline = endptr;
				config.flags = (config.flags & ~activate) | (newflags & activate);
				change = true;
				break;

			// flag2 �̒l��ݒ�  -g�l,�L���r�b�g
			case 'g':
				newflags = strtoul(cmdline, &endptr, 16);
				activate = ~0;
				if (*endptr == ',')
				{
					activate = strtoul(endptr+1, &endptr, 16);
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

		// �c��͑����t�@�C����
		OpenDiskImage(cmdline);
		break;
	}
}

