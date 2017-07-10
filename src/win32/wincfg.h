// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: wincfg.h,v 1.4 2003/05/12 22:26:35 cisc Exp $

#pragma once

#include "interface/if_win.h"
#include "pc88/config.h"
#include "win32/cfgpage.h"

#include <vector>

namespace m88win {

class WinConfig final : public IConfigPropBase {
 public:
  WinConfig();
  ~WinConfig();

  bool Show(HINSTANCE, HWND, Config* config);
  bool ProcMsg(MSG& msg);
  bool IsOpen() { return !!hwndps; }
  void Close();

 private:
  struct PPNode {
    PPNode* next;
    IConfigPropSheet* sheet;
  };

  // Overrides IConfigPropBase.
  bool IFCALL Add(IConfigPropSheet* sheet) final;
  bool IFCALL Remove(IConfigPropSheet* sheet) final;
  bool IFCALL Apply() final;
  bool IFCALL PageSelected(IConfigPropSheet*) final;
  bool IFCALL PageChanged(HWND) final;
  void IFCALL _ChangeVolume(bool) final;

  int PropProc(HWND, UINT, LPARAM);
  static int CALLBACK PropProcGate(HWND, UINT, LPARAM);

  PPNode* pplist;
  using PropSheets = std::vector<IConfigPropSheet*>;
  PropSheets propsheets;

  HWND hwndparent;
  HWND hwndps;
  HINSTANCE hinst;
  Config config;
  Config orgconfig;
  int page;
  int npages;

  ConfigCPU ccpu;
  ConfigScreen cscrn;
  ConfigSound csound;
  ConfigVolume cvol;
  ConfigFunction cfunc;
  ConfigSwitch cswitch;
  ConfigEnv cenv;
  ConfigROMEO cromeo;
};
}  // namespace m88win
