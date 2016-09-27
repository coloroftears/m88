// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//	��ʕ`��֌W
// ---------------------------------------------------------------------------
//	$Id: windraw.h,v 1.19 2002/04/07 05:40:11 cisc Exp $

#if !defined(win32_windraw_h)
#define win32_windraw_h

#include "types.h"
#include "draw.h"
#include "critsect.h"

// ---------------------------------------------------------------------------

class WinDrawSub  
{
public:
	WinDrawSub() : status(0) {}
	virtual ~WinDrawSub() {}

	virtual bool Init(HWND hwnd, uint w, uint h, GUID* display) = 0;
	virtual bool Resize(uint width, uint height) { return false; }
	virtual bool Cleanup() = 0;
	virtual void SetPalette(PALETTEENTRY* pal, int index, int nentries) {}
	virtual void QueryNewPalette() {}
	virtual void DrawScreen(const RECT& rect, bool refresh) = 0;

	virtual bool Lock(uint8** pimage, int* pbpl) { return false; }
	virtual bool Unlock() { return true; }

	virtual void SetGUIMode(bool gui) { }
	virtual uint GetStatus() { return status; }
	virtual void Flip() {}
	virtual bool SetFlipMode(bool flip) { return false; }
	virtual void WindowMoved(int cx, int cy) { return; }

protected:
	uint status;
};

// ---------------------------------------------------------------------------

class WinDraw : public Draw
{
public:
	WinDraw();
	~WinDraw();
	bool Init0(HWND hwindow);
	
// - Draw Common Interface
	bool Init(uint w, uint h, uint bpp);
	bool Cleanup();

	bool Lock(uint8** pimage, int* pbpl);
	bool Unlock();
	
	void Resize(uint width, uint height);
	void SetPalette(uint index, uint nents, const Palette* pal);
	void DrawScreen(const Region& region);

	uint GetStatus();
	void Flip();
	bool SetFlipMode(bool f);
	
// - Unique Interface
	int GetDrawCount() { int ret = drawcount; drawcount = 0; return ret; }
	void RequestPaint();
	void QueryNewPalette(bool background);
	void Activate(bool f) { active = f; }
	
	void SetPriorityLow(bool low);
	void SetGUIFlag(bool flag);
	bool ChangeDisplayMode(bool fullscreen, bool force480 = true);
	void Refresh() { refresh = 1; }
	void WindowMoved(int cx, int cy);

	int CaptureScreen(uint8* dest);

private:
	enum DisplayType { None, GDI, DDWin, DDFull };
	void PaintWindow();

	static BOOL WINAPI DDEnumCallback(GUID FAR* guid, LPSTR desc, LPSTR name, LPVOID context, HMONITOR hm);
	
	uint ThreadMain();
	static uint __stdcall ThreadEntry(LPVOID arg);
	
	uint idthread;
	HANDLE hthread;
	volatile bool shouldterminate;	// �X���b�h�I���v��

	DisplayType drawtype;

	int palcngbegin;				// �p���b�g�ύX�G���g���̍ŏ�
	int palcngend;					// �p���b�g�ύX�G���g���̍Ō�
	int palrgnbegin;				// �g�p���p���b�g�̍ŏ�
	int palrgnend;					// �g�p���p���b�g�̍Ō�
	volatile bool drawing;			// ��ʂ�����������
	bool drawall;					// ��ʑS�̂�����������
	bool active;
	bool haspalette;				// �p���b�g�������Ă���

	int  refresh;
	RECT drawarea;					// ����������̈�
	int drawcount;
	int guicount;

	int width;
	int height;

	HWND hwnd;
	HANDLE hevredraw;
	WinDrawSub* draw;

	CriticalSection csdraw;
	bool locked;
	bool flipmode;

	HMONITOR hmonitor;				// �T������ hmonitor
	GUID gmonitor;					// hmonitor �ɑΉ����� GUID

	PALETTEENTRY palette[0x100];
};

#endif // !defined(win32_windraw_h)
