// ---------------------------------------------------------------------------
//  M88 - PC-88 Emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  waveOut based driver
// ---------------------------------------------------------------------------
//  $Id: soundwo.h,v 1.5 2002/05/31 09:45:21 cisc Exp $

#pragma once

#include <windows.h>

#include <mmsystem.h>

#include "common/critical_section.h"
#include "common/sound_source.h"
#include "win32/sounddrv.h"

// ---------------------------------------------------------------------------

namespace WinSoundDriver {

class DriverWO final : public Driver {
 public:
  DriverWO();
  ~DriverWO();

  // Overrides Driver.
  bool Init(SoundSource<Sample16>* sb,
            HWND hwnd,
            uint32_t rate,
            uint32_t ch,
            uint32_t buflen) final;
  bool Cleanup() final;

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
  int numblocks;     // WAVEHDR(PCM ブロック)の数
  WAVEHDR* wavehdr;  // WAVEHDR の配列
  bool dontmix;      // WAVE を送る際に音声の合成をしない
};
}  // namespace WinSoundDriver
