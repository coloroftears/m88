// ---------------------------------------------------------------------------
//  M88 - PC-88 Emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  waveOut based driver
// ---------------------------------------------------------------------------
//  $Id: soundwo.h,v 1.5 2002/05/31 09:45:21 cisc Exp $

#if !defined(win32_soundwo_h)
#define win32_soundwo_h

#include "win32/sounddrv.h"
#include "win32/critsect.h"

// ---------------------------------------------------------------------------

namespace WinSoundDriver {

class DriverWO : public Driver {
 public:
  DriverWO();
  ~DriverWO();

  bool Init(SoundSource* sb,
            HWND hwnd,
            uint32_t rate,
            uint32_t ch,
            uint32_t buflen);
  bool Cleanup();

 private:
  bool SendBlock(WAVEHDR*);
  void BlockDone(WAVEHDR*);
  static uint32_t __stdcall ThreadEntry(LPVOID arg);
  static void CALLBACK
  WOProc(HWAVEOUT hwo, uint32_t umsg, DWORD inst, DWORD p1, DWORD p2);
  void DeleteBuffers();

  HWAVEOUT hwo;
  HANDLE hthread;
  uint32_t idthread;
  int numblocks;     // WAVEHDR(PCM �u���b�N)�̐�
  WAVEHDR* wavehdr;  // WAVEHDR �̔z��
  bool dontmix;      // WAVE �𑗂�ۂɉ����̍��������Ȃ�
};
}

#endif  // win32_soundwo_h
