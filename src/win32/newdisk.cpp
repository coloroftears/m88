// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  NewDisk Dialog Box for M88
// ---------------------------------------------------------------------------
//  $Id: newdisk.cpp,v 1.5 1999/12/28 10:34:52 cisc Exp $

#include "win32/headers.h"
#include "win32/resource.h"
#include "win32/newdisk.h"

// ---------------------------------------------------------------------------
//  構築/消滅
//
WinNewDisk::WinNewDisk() {
  info.title[0] = 0;
}

// ---------------------------------------------------------------------------
//  ダイアログ表示
//
bool WinNewDisk::Show(HINSTANCE hinst, HWND hwndparent) {
  info.title[0] = 0;
  info.type = MD2D;
  info.basicformat = 0;
  return DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_NEWDISK), hwndparent,
                        DLGPROC((void*)DlgProcGate), (LPARAM) this) > 0;
}

// ---------------------------------------------------------------------------
//  ダイアログ処理
//
INT_PTR WinNewDisk::DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_INITDIALOG:
      SetFocus(GetDlgItem(hdlg, IDC_NEWDISK_TITLE));
      CheckDlgButton(hdlg, IDC_NEWDISK_2D, BST_CHECKED);
      return 0;

    case WM_COMMAND:
      switch (LOWORD(wp)) {
        case IDOK:
          GetDlgItemText(hdlg, IDC_NEWDISK_TITLE, info.title, 16);
          info.title[16] = 0;
          EndDialog(hdlg, true);
          break;

        case IDCANCEL:
          EndDialog(hdlg, false);
          break;

        case IDC_NEWDISK_2D:
          info.type = MD2D;
          break;
        case IDC_NEWDISK_2DD:
          info.type = MD2DD;
          break;
        case IDC_NEWDISK_2HD:
          info.type = MD2HD;
          break;

        case IDC_NEWDISK_N88FORMAT:
          info.basicformat = !info.basicformat;
          break;
      }
      EnableWindow(GetDlgItem(hdlg, IDC_NEWDISK_N88FORMAT), info.type == MD2D);
      return true;

    case WM_CLOSE:
      EndDialog(hdlg, false);
      return true;

    default:
      return false;
  }
}

INT_PTR CALLBACK WinNewDisk::DlgProcGate(HWND hwnd,
                                         UINT m,
                                         WPARAM w,
                                         LPARAM l) {
  WinNewDisk* newdisk = nullptr;
  if (m == WM_INITDIALOG) {
    if (newdisk = reinterpret_cast<WinNewDisk*>(l))
      ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)newdisk);
  } else {
    newdisk = (WinNewDisk*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
  }
  if (!newdisk)
    return FALSE;
  return newdisk->DlgProc(hwnd, m, w, l);
}
