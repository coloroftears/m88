// ---------------------------------------------------------------------------
//  M88 - PC-88 Emulator
//  Copyright (C) cisc 1997, 2001.
// ---------------------------------------------------------------------------
//  Sound Implemention for Win32
// ---------------------------------------------------------------------------
//  $Id: winsound.h,v 1.17 2003/05/12 22:26:36 cisc Exp $

#pragma once

#include <windows.h>

#include <mmsystem.h>
#include <dsound.h>
#include <stdint.h>

#include <string>

#include "common/critical_section.h"
#include "pc88/sound.h"
#include "win32/sounddrv.h"

namespace pc88core {
class Config;
}  // namespace pc88core


namespace m88win {
class OPNMonitor;

class SoundDumpPipe final : public SoundSource<Sample16> {
 public:
  SoundDumpPipe();

  void SetSource(SoundSource<Sample16>* source) { source_ = source; }

  // Overrides SoundSource<Sample16>.
  int Get(Sample16* dest, int samples) final;
  uint32_t GetRate() const final { return source_ ? source_->GetRate() : 0; }
  int GetChannels() const final { return source_ ? source_->GetChannels() : 0; }
  int GetAvail() const final { return INT_MAX; }

  bool DumpStart(char* filename);
  bool DumpStop();
  bool IsDumping() { return dumpstate_ != IDLE; }

 private:
  enum DumpState { IDLE, STANDBY, DUMPING };

  void Dump(Sample16* dest, int samples);

  SoundSource<Sample16>* source_;
  std::string dumpfile_;

  HMMIO hmmio_;        // mmio handle
  MMCKINFO ckparent_;  // RIFF チャンク
  MMCKINFO ckdata_;    // data チャンク

  DumpState dumpstate_;
  int dumpedsample_;
  uint32_t dumprate_;

  CriticalSection cs_;
};

using Config = pc88core::Config;

class WinSound : public pc88core::Sound {
 public:
  WinSound();
  ~WinSound();

  bool Init(pc88core::PC88* pc, HWND hwnd, uint32_t rate, uint32_t buflen);
  bool ChangeRate(uint32_t rate, uint32_t buflen, bool wo);

  void ApplyConfig(const Config* config);

  bool DumpBegin(char* filename);
  bool DumpEnd();
  bool IsDumping() { return dumper.IsDumping(); }

  void SetSoundMonitor(m88win::OPNMonitor* mon) { soundmon = mon; }

 private:
  void Cleanup();
  //  int Get(Sample16* dest, int samples);

  WinSoundDriver::Driver* driver;

  HWND hwnd;
  uint32_t currentrate;
  uint32_t currentbuflen;
  uint32_t samprate;

  m88win::OPNMonitor* soundmon;
  bool wodrv;
  bool useds2;

  SoundDumpPipe dumper;
};
}  // namespace m88win