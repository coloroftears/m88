// ---------------------------------------------------------------------------
//  M88 - PC88 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  GDI �ɂ���ʕ`�� (HiColor �ȏ�)
// ---------------------------------------------------------------------------
//  $Id: DrawGDI.cpp,v 1.13 2003/04/22 13:16:35 cisc Exp $

//  bug:�p���b�g�`(T-T

#include "headers.h"
#include "drawgdi.h"

// ---------------------------------------------------------------------------
//  �\�z/����
//
WinDrawGDI::WinDrawGDI()
{
    hwnd = 0;
    hbitmap = 0;
    updatepal = false;
    bitmapimage = 0;
    image = 0;
}

WinDrawGDI::~WinDrawGDI()
{
    Cleanup();
}

// ---------------------------------------------------------------------------
//  ����������
//
bool WinDrawGDI::Init(HWND hwindow, uint w, uint h, GUID*)
{
    hwnd = hwindow;

    if (!Resize(w, h))
        return false;
    return true;
}

// ---------------------------------------------------------------------------
//  ��ʗL���͈͂�ύX
//
bool WinDrawGDI::Resize(uint w, uint h)
{
    width = w;
    height = h;

    if (!MakeBitmap())
        return false;

    memset(image, 0x40, width * height);
    status |= Draw::shouldrefresh;
    return true;
}

// ---------------------------------------------------------------------------
//  ��Еt��
//
bool WinDrawGDI::Cleanup()
{
    if (hbitmap)
    {
        DeleteObject(hbitmap), hbitmap = 0;
    }
    return true;
}

// ---------------------------------------------------------------------------
//  BITMAP �쐬
//
bool WinDrawGDI::MakeBitmap()
{
    binfo.header.biSize          = sizeof(BITMAPINFOHEADER);
    binfo.header.biWidth         = width;
    binfo.header.biHeight        = -(int) height;
    binfo.header.biPlanes        = 1;
    binfo.header.biBitCount      = 8;
    binfo.header.biCompression   = BI_RGB;
    binfo.header.biSizeImage     = 0;
    binfo.header.biXPelsPerMeter = 0;
    binfo.header.biYPelsPerMeter = 0;
    binfo.header.biClrUsed       = 256;
    binfo.header.biClrImportant  = 0;

    // �p���b�g�Ȃ��ꍇ
    
    HDC hdc = GetDC(hwnd);
    memset(binfo.colors, 0, sizeof(RGBQUAD) * 256);

    if (hbitmap)
        DeleteObject(hbitmap);
    hbitmap = CreateDIBSection( hdc,
                                (BITMAPINFO*) &binfo,
                                DIB_RGB_COLORS,
                                (void**)(&bitmapimage),
                                NULL, 
                                0 );

    ReleaseDC(hwnd, hdc);

    if (!hbitmap)
        return false;
    
    bpl = width;
    image = bitmapimage;
    return true;
}

// ---------------------------------------------------------------------------
//  �p���b�g�ݒ�
//  index �Ԗڂ̃p���b�g�� pe ���Z�b�g
//
void WinDrawGDI::SetPalette(PALETTEENTRY* pe, int index, int nentries)
{
    for (; nentries>0; nentries--)
    {
        binfo.colors[index].rgbRed = pe->peRed;
        binfo.colors[index].rgbBlue = pe->peBlue;
        binfo.colors[index].rgbGreen = pe->peGreen;
        index++, pe++;
    }
    updatepal = true;
}

// ---------------------------------------------------------------------------
//  �`��
//
void WinDrawGDI::DrawScreen(const RECT& _rect, bool refresh)
{
    RECT rect = _rect;
    bool valid = rect.left < rect.right && rect.top < rect.bottom;

    if (refresh || updatepal)
        SetRect(&rect, 0, 0, width, height), valid = true;
    
    if (valid)
    {
        HDC hdc = GetDC(hwnd);
        HDC hmemdc = CreateCompatibleDC(hdc);
        HBITMAP oldbitmap = (HBITMAP) SelectObject(hmemdc, hbitmap);
        if (updatepal)
        {
            updatepal = false;
            SetDIBColorTable(hmemdc, 0, 0x100, binfo.colors);
        }
        
        BitBlt(hdc, rect.left, rect.top, 
                    rect.right - rect.left, rect.bottom - rect.top, 
               hmemdc, rect.left, rect.top, 
               SRCCOPY);
        
        SelectObject(hmemdc, oldbitmap);
        DeleteDC(hmemdc);
        ReleaseDC(hwnd, hdc);
    }
}

// ---------------------------------------------------------------------------
//  ��ʃC���[�W�̎g�p�v��
//
bool WinDrawGDI::Lock(uint8** pimage, int* pbpl)
{
    *pimage = image;
    *pbpl = bpl;
    return image != 0;
}

// ---------------------------------------------------------------------------
//  ��ʃC���[�W�̎g�p�I��
//
bool WinDrawGDI::Unlock()
{
    status &= ~Draw::shouldrefresh;
    return true;
}

