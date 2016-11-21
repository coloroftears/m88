// ---------------------------------------------------------------------------
//  M88 - PC-88 Emulator
//  Copyright (C) cisc 1999, 2000.
// ---------------------------------------------------------------------------
//  DirectSound based driver - another version
// ---------------------------------------------------------------------------
//  $Id: soundds2.h,v 1.2 2002/05/31 09:45:21 cisc Exp $

#if !defined(win32_soundds2_h)
#define win32_soundds2_h

#include "win32/sounddrv.h"

// ---------------------------------------------------------------------------

namespace WinSoundDriver {

class DriverDS2 : public Driver {
 private:
  enum {
    nblocks = 4,  // 2 �ȏ�
  };

 public:
  DriverDS2();
  ~DriverDS2();

  bool Init(SoundSource* sb,
            HWND hwnd,
            uint32_t rate,
            uint32_t ch,
            uint32_t buflen);
  bool Cleanup();

 private:
  static uint32_t WINAPI ThreadEntry(LPVOID arg);
  void Send();

  LPDIRECTSOUND lpds;
  LPDIRECTSOUNDBUFFER lpdsb_primary;
  LPDIRECTSOUNDBUFFER lpdsb;
  LPDIRECTSOUNDNOTIFY lpnotify;

  uint32_t buffer_length;
  HANDLE hthread;
  uint32_t idthread;
  HANDLE hevent;
  uint32_t nextwrite;
};
}

#endif  // !defined(win32_soundds2_h)
