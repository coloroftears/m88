// $Id: config.h,v 1.2 1999/12/28 10:33:39 cisc Exp $

#pragma once

#include "interface/if_win.h"
#include "interface/ifcommon.h"

class ConfigMP : public IConfigPropSheet {
 public:
  ConfigMP();
  bool Init(HINSTANCE _hinst);
  bool IFCALL Setup(IConfigPropBase*, PROPSHEETPAGE* psp);

 private:
  INT_PTR PageProc(HWND, UINT, WPARAM, LPARAM);
  static INT_PTR PageGate(HWND, UINT, WPARAM, LPARAM);

  HINSTANCE hinst;
  IConfigPropBase* base;
};
