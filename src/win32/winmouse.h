// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: winmouse.h,v 1.5 2002/04/07 05:40:11 cisc Exp $

#if !defined(win32_winmouse_h)
#define win32_winmouse_h

#include "common/device.h"
#include "if/ifui.h"

class WinUI;

class WinMouseUI : public IMouseUI {
 public:
  WinMouseUI();
  ~WinMouseUI();

  bool Init(WinUI* ui);

  long IFCALL QueryInterface(REFIID, void**);
  uint32_t IFCALL AddRef();
  uint32_t IFCALL Release();

  bool IFCALL Enable(bool en);
  bool IFCALL GetMovement(POINT*);
  uint32_t IFCALL GetButton();

 private:
  POINT GetWindowCenter();

  WinUI* ui;

  POINT move;
  int32_t activetime;
  bool enable;
  int orgmouseparams[3];

  uint32_t refcount;
};

#endif  // !defined(win32_winmouse_h)
