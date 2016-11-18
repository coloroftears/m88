// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 2000.
// ---------------------------------------------------------------------------
//  $Id: winmon.cpp,v 1.4 2002/04/07 05:40:11 cisc Exp $

#include "headers.h"
#include "resource.h"
#include "winmon.h"
#include "misc.h"

// ---------------------------------------------------------------------------
//  �\�z/����
//
WinMonitor::WinMonitor()
: txtbuf(0), txpbuf(0), timerinterval(0), timer(0)
{
    hwnd = 0;
    hwndstatus = 0;
    line = 0;
    hfont = 0;
    hbitmap = 0;
    fontheight = 12;
    wndrect.right = -1;
}

WinMonitor::~WinMonitor()
{
    DeleteObject(hbitmap);
    DeleteObject(hfont);
    delete[] txtbuf;
}

// ---------------------------------------------------------------------------
//  ������
//
bool WinMonitor::Init(LPCTSTR tmpl)
{
    lptemplate = tmpl;
    nlines = 0;
    txcol = 0xffffff;
    bkcol = 0x400000;
    return true;
}

// ---------------------------------------------------------------------------
//  �_�C�A���O�\��
//
void WinMonitor::Show(HINSTANCE hinstance, HWND hwndparent, bool show)
{
    if (show)
    {
        if (!hwnd)
        {
            assert(!hwndstatus);

            RECT rect = wndrect;
            hinst = hinstance;
            hwnd = CreateDialogParam(hinst, lptemplate, hwndparent, DLGPROC((void*)DlgProcGate), LPARAM(this));
            hwndstatus = 0;

            if (rect.right > 0)
            {
                SetWindowPos(hwnd, 0, 0, 0, 
                        rect.right-rect.left, rect.bottom - rect.top, 
                        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            SetLines(nlines);
            Start();
        }
    }
    else
    {
        if (hwnd)
        {
            Stop();
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }
    }
}

// ---------------------------------------------------------------------------
//  �t�H���g�����̕ύX
//
bool WinMonitor::SetFont(HWND hwnd, int fh)
{
    if (hfont)
        DeleteObject(hfont);

    fontheight = fh;
    hfont = CreateFont(fontheight, 0, 0, 0, 0, 0, 0, 0, 
                    SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, FIXED_PITCH, "�l�r�S�V�b�N");
    if (!hfont)
        return false;

    HDC hdc = GetDC(hwnd);
    HFONT hof = (HFONT) SelectObject(hdc, hfont);
    GetCharWidth(hdc, '0', '0', &fontwidth);
    SelectObject(hdc, hof);
    ReleaseDC(hwnd, hdc);
    return true;
}

// ---------------------------------------------------------------------------
//  ���̑傫���ɏ]���e�L�X�g�o�b�t�@�̃T�C�Y��ύX
//          
void WinMonitor::ResizeWindow(HWND hwnd)
{
    RECT rect;

    GetWindowRect(hwnd, &wndrect);
    GetClientRect(hwnd, &rect);
    
    clientwidth = rect.right;
    clientheight = rect.bottom;
    if (hwndstatus)
    {
        RECT rectstatus;
        GetWindowRect(hwndstatus, &rectstatus);
        clientheight -= rectstatus.bottom - rectstatus.top;
    }
    width = (clientwidth + fontwidth - 1) / fontwidth;
    height = (clientheight + fontheight - 1) / fontheight;

    // ���z TVRAM �̃Z�b�g�A�b�v
    delete[] txtbuf;
    txtbuf = new TXCHAR[width * height * 2];
    txpbuf = txtbuf + width * height;
    ClearText();

    // DDB �̓���ւ�
    HDC hdc = GetDC(hwnd);
    HDC hmemdc = CreateCompatibleDC(hdc);
    
    if (hbitmap)
        DeleteObject(hbitmap);
    hbitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);

    // �w�i�F�œh��Ԃ�
    HBITMAP holdbmp = (HBITMAP) SelectObject(hmemdc, hbitmap);
    HBRUSH hbrush = (HBRUSH) SelectObject(hmemdc, CreateSolidBrush(bkcol));
    PatBlt(hmemdc, 0, 0, rect.right, rect.bottom, PATCOPY);
    DeleteObject(SelectObject(hmemdc, hbrush));
    SelectObject(hmemdc, holdbmp);
    DeleteDC(hmemdc);
    
    ReleaseDC(hwnd, hdc);

    // �X�N���[���o�[�̐ݒ�
    SetLines(nlines);
}

// ---------------------------------------------------------------------------
//  ��ʂ̒��g������
//
void WinMonitor::ClearText()
{
    int nch = width * height;
    for (int i=0; i<nch; i++)
    {
        txtbuf[i].ch = ' ';
        txtbuf[i].txcol = txcol;
        txtbuf[i].bkcol = bkcol;
    }
}

static inline bool IsKan(int c)
{
    return (0x81 <= c && c <= 0x9f) || (0xe0 < c && c < 0xfd);
}

// ---------------------------------------------------------------------------
//  ��ʂ�����
//
void WinMonitor::DrawMain(HDC _hdc, bool update)
{
    hdc = _hdc;
    
    Locate(0, 0);

    assert(height>=0);
    memcpy(txpbuf, txtbuf, sizeof(TXCHAR) * width * height);
    UpdateText();

    char* buf = new char[width];
    int c=0;
    int y=0;
    for (int l=0; l<height; l++)
    {
        if (memcmp(txpbuf + c, txtbuf + c, width * sizeof(TXCHAR)) || update)
        {
            // 1�s�`��
            for (int x=0; x<width; )
            {
                // ���������ŉ����������邩�c�H
                int n;
                
                for (n=x; n<width; n++)
                {
                    if (   txtbuf[c+x].txcol != txtbuf[c+n].txcol 
                        || txtbuf[c+x].bkcol != txtbuf[c+n].bkcol)
                        break;
                    buf[n-x] = txtbuf[c+n].ch;
                }

                // ����
                SetTextColor(hdc, txtbuf[c+x].txcol);
                SetBkColor(hdc, txtbuf[c+x].bkcol);
                TextOut(hdc, x * fontwidth, y, buf, n - x);
                x = n;
            }
        }
        c += width, y += fontheight;
    }
    delete[] buf;
}

// ---------------------------------------------------------------------------
//  1�s��ɃX�N���[��
//
void WinMonitor::ScrollUp()
{
    if (height > 2)
    {
        memmove(txtbuf, txtbuf + width, width * (height - 2) * sizeof(TXCHAR));
        memset(txtbuf+width*(height-1), 0, width * sizeof(TXCHAR)); 

        HDC hdc = GetDC(hwnd);
        HDC hmemdc = CreateCompatibleDC(hdc);
        
        HBITMAP holdbmp = (HBITMAP) SelectObject(hmemdc, hbitmap);
        
        RECT rect;
        GetClientRect(hwnd, &rect);
        BitBlt(hmemdc, 0, 0, rect.right, rect.bottom-fontheight,
               hmemdc, 0, fontheight, SRCCOPY);
        
        SelectObject(hmemdc, holdbmp);
        DeleteDC(hmemdc);

        ReleaseDC(hwnd, hdc);
    }
}

// ---------------------------------------------------------------------------
//  1�s���ɃX�N���[��
//
void WinMonitor::ScrollDown()
{
    if (height > 2)
    {
        memmove(txtbuf + width, txtbuf, width * (height - 1) * sizeof(TXCHAR));

        HDC hdc = GetDC(hwnd);
        HDC hmemdc = CreateCompatibleDC(hdc);
        
        HBITMAP holdbmp = (HBITMAP) SelectObject(hmemdc, hbitmap);
        
        RECT rect;
        GetClientRect(hwnd, &rect);
        BitBlt(hmemdc, 0, fontheight, rect.right, rect.bottom-fontheight,
               hmemdc, 0, 0, SRCCOPY);
        
        SelectObject(hmemdc, holdbmp);
        DeleteDC(hmemdc);

        ReleaseDC(hwnd, hdc);
    }
}

// ---------------------------------------------------------------------------
//  �e�X�g�p
//
void WinMonitor::UpdateText()
{
    Puts("???\n");
}

// ---------------------------------------------------------------------------
//  �������݈ʒu�̕ύX
//
void WinMonitor::Locate(int x, int y)
{
    txp.x = Min(x, width);
    txp.y = Min(y, height);
    txtbufptr = txtbuf + txp.x + txp.y * width;
}

// ---------------------------------------------------------------------------
//  �����񏑂�����
//
void WinMonitor::Puts(const char* text)
{
    if (txp.y >= height)
        return;
    
    int c;
    char* txx;
    while (c = *text++)
    {
        if (c == '\n')
        {
            while (txp.x++ < width)
            {
                txtbufptr->ch = ' ';
                txtbufptr->txcol = txcol;
                txtbufptr->bkcol = bkcol;
                txtbufptr++;
            }
            txp.x = 0, txp.y++;
            if (txp.y >= height)
                return;
        }
        else if (c == '\a')
        {
            // set fgcolor
            if (*text != ';')
            {
                txcolprev = txcol;
                txcol = strtoul(text, &txx, 0);
                text = txx + (*txx == ';');
            }
            else
            {
                txcol = txcolprev;
                text++;
            }
        }
        else if (c == '\b')
        {
            // set bkcolor
            if (*text != ';')
            {
                bkcolprev = bkcol;
                bkcol = strtoul(text, &txx, 0);
                text = txx + (*txx == ';');
            }
            else
            {
                bkcol = bkcolprev;
                text++;
            }
        }
        else
        {
            if (txp.x < width)
            {
                txtbufptr->ch = c;
                txtbufptr->txcol = txcol;
                txtbufptr->bkcol = bkcol;
                txtbufptr++;
                txp.x++;
            }
        }
    }
}

// ---------------------------------------------------------------------------
//  �����񏑂�����(�����t)
//
void WinMonitor::Putf(const char* msg, ...)
{
    char buf[512];
    va_list marker;
    va_start(marker, msg);
    wvsprintf(buf, msg, marker);
    va_end(marker);
    Puts(buf);
}

// ---------------------------------------------------------------------------
//  ����������
//
void WinMonitor::Draw(HWND hwnd, HDC hdc)
{
    if (!hbitmap)
        return;

    HDC hmemdc = CreateCompatibleDC(hdc);
    
    HBITMAP holdbmp = (HBITMAP) SelectObject(hmemdc, hbitmap);
    
    SetTextColor(hmemdc, 0xffffff);
    SetBkColor(hmemdc, bkcol);
    HFONT holdfont = (HFONT) SelectObject(hmemdc, hfont);
    
    DrawMain(hmemdc);
    
    SelectObject(hmemdc, holdfont);
    
    BitBlt(hdc, 0, 0, clientwidth, clientheight, hmemdc, 0, 0, SRCCOPY);

    SelectObject(hmemdc, holdbmp);
    DeleteDC(hmemdc);
}


void WinMonitor::Update()
{
    if (hwnd)
    {
        InvalidateRect(hwnd, 0, false);
        RedrawWindow(hwnd, 0, 0, RDW_UPDATENOW);
    }
}

// ---------------------------------------------------------------------------
//  ���C������ݒ�
//
void WinMonitor::SetLines(int nl)
{
    nlines = nl;
    line = Limit(line, nlines, 0);
    if (nlines)
    {
        SCROLLINFO si;
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
        si.nMin = 0;
        si.nMax = Max(1, nlines+height-2);
        si.nPage = height;
        si.nPos = line;
        SetScrollInfo(hwnd, SB_VERT, &si, true);
    }
}

// ---------------------------------------------------------------------------
//  ���C������ݒ�
//
void WinMonitor::SetLine(int nl)
{
    line = nl;
    if (nlines)
    {
        SCROLLINFO si;
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_POS | SIF_DISABLENOSCROLL;
        si.nMin = 0;
        si.nMax = Max(1, nlines+height-2);
        si.nPage = height;
        si.nPos = line;
        SetScrollInfo(hwnd, SB_VERT, &si, true);
    }
}



// ---------------------------------------------------------------------------
//  �X�N���[���o�[����
//
int WinMonitor::VerticalScroll(int msg)
{
    switch (msg)
    {
    case SB_LINEUP:
        if (--line < 0) 
            line = 0;
        else
            ScrollDown();
        break;

    case SB_LINEDOWN:
        if (++line >= nlines) 
            line = Max(0, nlines - 1);
        else
            ScrollUp();
        break;

    case SB_PAGEUP:
        line = Max(line - Max(1, height - 1), 0);
        break;
    
    case SB_PAGEDOWN:
        line = Min(line + Max(1, height - 1), nlines - 1);
        break;

    case SB_TOP:
        line = 0;
        break;

    case SB_BOTTOM:
        line = Max(0, nlines - 1);
        break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        line = Limit(GetScrPos(msg == SB_THUMBTRACK), nlines-1, 0);
        break;
    }
    return line;
}

// ---------------------------------------------------------------------------
//  ���݂̃X�N���[���^�u�̈ʒu�𓾂�
//
int WinMonitor::GetScrPos(bool track)
{
    SCROLLINFO si;
    memset(&si, 0, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = track ? SIF_TRACKPOS : SIF_POS;
    GetScrollInfo(hwnd, SB_VERT, &si);
    return track ? si.nTrackPos : si.nPos;
}

// ---------------------------------------------------------------------------
//  �N���C�G���g�̈���W�𕶎����W�ɕϊ�
//
bool WinMonitor::GetTextPos(POINT* p)
{
    if (fontwidth && fontheight)
    {
        p->x /= fontwidth;
        p->y /= fontheight;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
//  �����X�V�p�x��ݒ�
//
void WinMonitor::SetUpdateTimer(int t)
{
    timerinterval = t;
    if (hwnd)
    {
        if (timer)
            KillTimer(hwnd, timer), timer = 0;
        if (t)
            timer = SetTimer(hwnd, 1, timerinterval, 0);
    }
}

// ---------------------------------------------------------------------------
//
//
bool WinMonitor::EnableStatus(bool s)
{
    if (s)
    {
        if (!hwndstatus)
        {
            hwndstatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE, 0, GetHWnd(), 1);
            if (!hwndstatus)
                return false;
        }
    }
    else
    {
        if (hwndstatus)
        {
            DestroyWindow(hwndstatus);
            hwndstatus = 0;
        }
    }
    ResizeWindow(hwnd);
    return true;
}

bool WinMonitor::PutStatus(const char* text, ...)
{
    if (!hwndstatus)
        return false;

    if (!text)
    {
        SendMessage(hwndstatus, SB_SETTEXT, SBT_OWNERDRAW, 0);
        return true;
    }       

    va_list a;
    va_start(a, text);
    vsprintf(statusbuf, text, a);
    va_end(a);

    SendMessage(hwndstatus, SB_SETTEXT, SBT_OWNERDRAW, (LPARAM) statusbuf);
    return true;
}

// ---------------------------------------------------------------------------
//  �_�C�A���O����
//
INT_PTR WinMonitor::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
    bool r = false;
    int sn;
    SCROLLINFO si;
    
    switch (msg)
    {
    case WM_INITDIALOG:
        SetFont(hdlg, fontheight);
        ResizeWindow(hdlg);

        if (timerinterval && !timer)
            timer = SetTimer(hdlg, 1, timerinterval, 0);
        break;

    case WM_ACTIVATE:
        if (wp == WA_INACTIVE)
            EnableStatus(false);
        break;

    case WM_INITMENU:
        {
            HMENU hmenu = (HMENU) wp;

            CheckMenuItem(hmenu, IDM_MEM_F_1, (fontheight == 12) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hmenu, IDM_MEM_F_2, (fontheight == 14) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(hmenu, IDM_MEM_F_3, (fontheight == 16) ? MF_CHECKED : MF_UNCHECKED);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case IDM_MEM_F_1: fontheight = 12; break;
        case IDM_MEM_F_2: fontheight = 14; break;
        case IDM_MEM_F_3: fontheight = 16; break;
        }
        SetFont(hdlg, fontheight);
        ResizeWindow(hdlg);
        break;

    case WM_CLOSE:
        EnableStatus(false);
        if (timer)
            KillTimer(hdlg, timer), timer = 0;

        DestroyWindow(hdlg);
        hwnd = 0;
        r = true;
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hdlg, &ps);
            Draw(hdlg, hdc);
            EndPaint(hdlg, &ps);
        }
        r = true;
        break;

    case WM_TIMER:
        Update();
        return true;
    
    case WM_SIZE:
        ResizeWindow(hdlg);
        if (hwndstatus)
            PostMessage(hwndstatus, WM_SIZE, wp, lp);
        Update();

        r = true;
        break;

    case WM_VSCROLL:
        line = VerticalScroll(LOWORD(wp));
        
        memset(&si, 0, sizeof(si));
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        si.nPos = line;
        SetScrollInfo(hdlg, SB_VERT, &si, true);

        Update();
        return true;

    case WM_KEYDOWN:
        if (nlines)
        {
            sn = -1;
            switch (wp)
            {
            case VK_UP:     sn = SB_LINEUP; break;
            case VK_PRIOR:  sn = SB_PAGEUP; break;
            case VK_NEXT:   sn = SB_PAGEDOWN; break;
            case VK_DOWN:   sn = SB_LINEDOWN; break;
            case VK_HOME:   sn = SB_TOP; break;
            case VK_END:    sn = SB_BOTTOM; break;
            }
            if (sn != -1)
                SendMessage(hwnd, WM_VSCROLL, MAKELONG(sn, 0), 0L);  
        }
        break;

    case WM_DRAWITEM:
        if ((UINT) wp == 1)
        {
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lp;
            SetBkColor(dis->hDC, GetSysColor(COLOR_3DFACE));
//          SetTextColor(dis->hDC, RGB(255, 0, 0));
            char* text = reinterpret_cast<char*>(dis->itemData);
            if (text)
                TextOut(dis->hDC, dis->rcItem.left, dis->rcItem.top, text, strlen(text));
        }
        break;
    }
    return r;
}

// static
INT_PTR WinMonitor::DlgProcGate
(HWND hwnd, UINT m, WPARAM w, LPARAM l)
{
    WinMonitor* monitor = nullptr;
    if (m == WM_INITDIALOG) {
        if (monitor = reinterpret_cast<WinMonitor*>(l))
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)monitor);
    }
    else {
        monitor = (WinMonitor*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    if (!monitor)
        return FALSE;
    return monitor->DlgProc(hwnd, m, w, l);
}
