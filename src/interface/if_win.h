// ----------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//  拡張モジュール用インターフェース定義
// ----------------------------------------------------------------------------
//  $Id: ifcommon.h,v 1.8 2002/04/07 05:40:09 cisc Exp $

#pragma once

#include <windows.h>

#include <commdlg.h>
#include <stdint.h>

#include "common/types.h"

// ----------------------------------------------------------------------------
//  「設定」ダイアログを操作するためのインターフェース
//
struct IConfigPropSheet;

struct IConfigPropBase {
  virtual bool IFCALL Add(IConfigPropSheet*) = 0;
  virtual bool IFCALL Remove(IConfigPropSheet*) = 0;

  virtual bool IFCALL Apply() = 0;
  virtual bool IFCALL PageSelected(IConfigPropSheet*) = 0;
  virtual bool IFCALL PageChanged(HWND) = 0;
  virtual void IFCALL _ChangeVolume(bool) = 0;
};

// ----------------------------------------------------------------------------
//  「設定」のプロパティシートの基本インターフェース
//
struct IConfigPropSheet {
  virtual bool IFCALL Setup(IConfigPropBase*, PROPSHEETPAGE* psp) = 0;
};
