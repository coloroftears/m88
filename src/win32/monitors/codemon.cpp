// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1999, 2000.
// ---------------------------------------------------------------------------
//  $Id: codemon.cpp,v 1.8 2001/02/21 11:58:53 cisc Exp $

#include "win32/headers.h"
#include "win32/resource.h"
#include "pc88/pc88.h"
#include "win32/monitors/codemon.h"
#include "common/misc.h"
#include "common/device_i.h"
#include "win32/winvars.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//  �\�z/����
//
CodeMonitor::CodeMonitor() {}

CodeMonitor::~CodeMonitor() {}

bool CodeMonitor::Init(PC88* pc88) {
  if (!MemViewMonitor::Init(MAKEINTRESOURCE(IDD_CODEMON), pc88))
    return false;
  if (!diag.Init(GetBus()))
    return false;

  SetLines(0x10000);
  SetUpdateTimer(500);
  return true;
}

// ---------------------------------------------------------------------------
//  �_�C�A���O����
//
BOOL CodeMonitor::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_COMMAND:
      if (LOWORD(wp) == IDM_MEM_SAVEIMAGE) {
        DumpImage();
      }
      break;
  }
  return MemViewMonitor::DlgProc(hdlg, msg, wp, lp);
}

// ---------------------------------------------------------------------------
//  �X�N���[���o�[����
//
int CodeMonitor::VerticalScroll(int msg) {
  int addr = GetLine();

  switch (msg) {
    int i;

    case SB_LINEUP:
      addr = diag.InstDec(addr);
      if (addr >= 0)
        ScrollDown();
      break;

    case SB_LINEDOWN:
      addr = diag.InstInc(addr);
      if (addr < 0x10000)
        ScrollUp();
      break;

    case SB_PAGEUP:
      for (i = 1; i < GetHeight() && addr > 0; i++)
        addr = diag.InstDec(addr);
      break;

    case SB_PAGEDOWN:
      for (i = 1; i < GetHeight() && addr < 0x10000; i++)
        addr = diag.InstInc(addr);
      break;

    case SB_TOP:
      addr = 0;
      break;

    case SB_BOTTOM:
      addr = 0x10000 - 1;
      break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
      addr = GetScrPos(msg == SB_THUMBTRACK);
      break;
  }
  return Limit(addr, 0xffff, 0);
}

static inline void ToHex(char** p, uint32_t d) {
  static const char hex[] = "0123456789abcdef";

  *(*p)++ = hex[d >> 4];
  *(*p)++ = hex[d & 15];
}

void CodeMonitor::UpdateText() {
  char buf[128];
  int a = GetLine();

  for (int y = 0; y < GetHeight(); y++) {
    if (a < 0x10000) {
      // ���s�񐔂Ɋ�Â��ĐF�����Ă݂�
      SetBkCol(RGB(StatExec(a), 0, 0x20));

      int next = diag.Disassemble(a, buf + 8);
      char* ptr = buf;
      int c, d = next - a;
      for (c = 0; c < d; c++)
        ToHex(&ptr, GetBus()->Read8((a + c) & 0xffff));
      for (; c < 4; c++)
        *ptr++ = ' ', *ptr++ = ' ';
      Putf("%.4x: %.8s   %s\n", a, buf, buf + 8);
      a = next;
    } else
      Puts("\n");
  }
  StatClear();
}

bool CodeMonitor::Dump(FILE* fp, int from, int to) {
  char buf[128];
  for (int a = from; a < to; a) {
    int next = diag.Disassemble(a, buf + 8);

    char* ptr = buf;
    int c, d = next - a;
    for (c = 0; c < d; c++)
      ToHex(&ptr, GetBus()->Read8((a + c) & 0xffff));
    for (; c < 4; c++)
      *ptr++ = ' ', *ptr++ = ' ';

    fprintf(fp, "%.4x %.8s\t\t%s\n", a, buf, buf + 8);
    a = next;
  }
  return true;
}

// ----------------------------------------------------------------------------
//  ���_���v����������
//
bool CodeMonitor::DumpImage() {
  // �_�C�A���O
  OFNV5 ofn;
  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize = WINVAR(OFNSIZE);
  ofn.FlagsEx = OFN_EX_NOPLACESBAR;

  char filename[MAX_PATH];
  filename[0] = 0;

  ofn.hwndOwner = GetHWnd();
  ofn.lpstrFilter =
      "assembly file (*.asm)\0*.asm\0"
      "All Files (*.*)\0*.*\0";
  ofn.lpstrFile = filename;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_CREATEPROMPT;
  ofn.lpstrDefExt = "asm";
  ofn.lpstrTitle = 0;

  if (!GetSaveFileName(&ofn))
    return false;

  // ��������
  FILE* fp = fopen(filename, "w");
  if (fp) {
    Dump(fp, 0, 0x10000);
    fclose(fp);
    return true;
  }
  return false;
}
