// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: config.h,v 1.23 2003/09/28 14:35:35 cisc Exp $

#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------

namespace pc88core {

class DipSwitch {
 public:
  enum Bits {
    // Boot mode - terminal(0) / BASIC(1)
    kMode = 1,
    // Width - 80(0) / 40(1)
    kWidth = 1 << 1,
    // Lines - 25(0) / 20(1)
    kLine = 1 << 2,
    // S parameter (serial) - Enabled(0) / Disabled(1)
    kSparameter = 1 << 3,
    // DEL handling (serial) - Enabled(0) / Ignored(1)
    kDEL = 1 << 4,
    // Parity Check (serial) - Enabled(0) / Disabled(1)
    kParityCheck = 1 << 5,
    // Parity (serial) - Even(0) / Odd(1)
    kParity = 1 << 6,
    // Data bits (serial) - 8bits(0) / 7bits(1)
    kDataBits = 1 << 7,
    // Stop bits (serial) - 2bits(0) / 1bit(1)
    kStopBits = 1 << 8,
    // X parameter (serial) - Enabled(0) / Disabled(1)
    kXParameter = 1 << 9,
    // Communication (serial) - Half Duplex(0) / Full Duplex(1)
    kCommunication = 1 << 10,
    // Boot from FDD - Enabled(0) / Disabled(1)
    kBootFromFDD = 1 << 11
  };

  DipSwitch() : dipsw_(0) {}

  static constexpr uint32_t DefaultValue() { return 1829; }

  int dipsw() const { return dipsw_; }
  void set_dipsw(int n) { dipsw_ = n; }
  void set_dipsw_bit(int n) { dipsw_ |= (1 << n); }
  void clear_dipsw_bit(int n) { dipsw_ &= ~(1 << n); }
  bool is_dipsw_on(int n) const { return (dipsw_ & (1 << n)) != 0; }

 private:
  uint32_t dipsw_;
};

class Config {
 public:
  enum BASICMode {
    // bit0 H/L
    // bit1 N/N80 (bit5=0)
    // bit4 V1/V2
    // bit5 N/N88
    // bit6 CDROM 有効
    N80 = 0x00,
    N802 = 0x02,
    N80V2 = 0x12,
    N88V1 = 0x20,
    N88V1H = 0x21,
    N88V2 = 0x31,
    N88V2CD = 0x71,
  };
  enum KeyType { AT106 = 0, PC98 = 1, AT101 = 2 };
  enum CPUType {
    ms11 = 0,
    ms21,
    msauto,
  };

  enum Flags {
    kSubCPUControl = 1 << 0,   // Sub CPU の駆動を制御する
    kSaveDirectory = 1 << 1,   // 起動時に前回終了時のディレクトリに移動
    kFullspeed = 1 << 2,       // 全力動作
    kEnablePad = 1 << 3,       // パッド有効
    kEnableOPNA = 1 << 4,      // OPNA モード (44h)
    kWatchRegister = 1 << 5,   // レジスタ表示
    kAskBeforeReset = 1 << 6,  // 終了・リセット時に確認
    kEnablePCG = 1 << 7,       // PCG 系のフォント書き換えを有効
    kFv15k = 1 << 8,           // 15KHz モニターモード
    kCPUBurst = 1 << 9,        // ノーウェイト
    // Suppress ALT key as menu selection (Windows)
    // This enables ALT key as GRPH key.
    kSuppressMenu = 1 << 10,
    // クロック単位で切り替え
    kCPUClockMode = 1 << 11,
    // Map arrow keys to ten keys
    kUseArrowFor10 = 1 << 12,
    // Swap pad buttons
    kSwappedButtons = 1 << 13,
    // Disable CMD SING
    kDisableSing = 1 << 14,
    // Digital Palette mode (force digital palette)
    kDigitalPalette = 1 << 15,
    // Use QueryPerformanceCounter for timing
    // kUseQPC = 1 << 16,      // Not used
    // Force 640x480 mode on fullscreen (DirectDraw)
    kForce480 = 1 << 17,
    // Connect OPN on port A8H.
    kOPNOnA8 = 1 << 18,
    // Connect OPNA on port A8H.
    kOPNAOnA8 = 1 << 19,
    kDrawPriorityLow = 1 << 20,  // 描画の優先度を落とす
    kDisableF12Reset =
        1 << 21,  // F12 を RESET として使用しない(COPY キーになる)
    // Draw even lines (on 200line mode)
    kFullline = 1 << 22,
    kShowStatusBar = 1 << 23,  // ステータスバー表示
    kShowFDCStatus = 1 << 24,  // FDC のステータスを表示
    kEnableWait = 1 << 25,     // Wait をつける
    kEnableMouse = 1 << 26,    // Mouse を使用
    kMouseJoyMode = 1 << 27,   // Mouse をジョイスティックモードで使用
    kSpecialPalette = 1 << 28, // デバックパレットモード
    kMixSoundAlways = 1 << 29, // 処理が重い時も音の合成を続ける
    kPreciseMixing = 1 << 30,  // 高精度な合成を行う
  };
  enum Flag2 {
    kDisableOPN44 = 1 << 0,    // OPN(44h) を無効化 (V2 モード時は OPN)
    kUseWaveOutDrv = 1 << 1,   // PCM の再生に waveOut を使用する
    kMask0 = 1 << 2,           // 選択表示モード
    kMask1 = 1 << 3,
    kMask2 = 1 << 4,
    kGenScrnshotName = 1 << 5, // スクリーンショットファイル名を自動生成
    kUseFMClock = 1 << 6,      // FM 音源の合成に本来のクロックを使用
    kCompressSnapshot = 1 << 7,  // スナップショットを圧縮する
    // Synchronize display VSsync on fullscreen
    kSyncToVSync = 1 << 8,
    // Show places bar on file dialog (Windows)
    kShowPlacesBar = 1 << 9,
    // kLPFEnable = 1 << 10,
    kFDDNoWait = 1 << 11,      // FDD ノーウェイト
    // Use notify based driver for DirectSound
    kUseDSNotify = 1 << 12,
  };

  int flags;
  int flag2;
  int refreshtiming;
  int mainsubratio;
  // PCM mixing frequency (in Hz, e.g. 44100)
  int sound;
  int erambanks;
  // enum KeyType (PC98 or AT106)
  KeyType keytype;
  // fmgen volume parameters
  int volfm;
  int volssg;
  int voladpcm;
  int volrhythm;
  int volbd;
  int volsd;
  int voltop;
  int volhh;
  int voltom;
  int volrim;

  // TODO: Instead of forwarding methods, just inherit?
  int dipsw() const { return dipsw_.dipsw(); }
  void set_dipsw(int n) { dipsw_.set_dipsw(n); }
  void set_dipsw_bit(int n) { dipsw_.set_dipsw_bit(n); }
  void clear_dipsw_bit(int n) { dipsw_.clear_dipsw_bit(n); }
  bool is_dipsw_on(int n) const { return dipsw_.is_dipsw_on(n); }

  int clock() const { return clock_; }
  void set_clock(int clock) { clock_ = clock; }

  int speed() const { return speed_; }
  void set_speed(int speed) { speed_ = speed; }

  // Sound buffer size (in milliseconds)
  uint32_t soundbuffer;
  // Mouse sensitivity threshold?
  uint32_t mousesensibility;

  int cpumode;
  int romeolatency;

  BASICMode basicmode;

  // 15kHz モードの判定を行う．
  // (条件: option 又は N80/SR モード時)
  bool IsFV15k() const { return (basicmode & 2) || (flags & kFv15k); }

 private:
  // Dip switch
  DipSwitch dipsw_;
  // CPU base clock (in 0.1MHz, e.g. 40 = 4MHz)
  int clock_;
  // Relative emulation speed (in permill, 1000 = 1.000x)
  int speed_;
};
}  // namespace pc88core
