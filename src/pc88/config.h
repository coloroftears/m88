// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: config.h,v 1.23 2003/09/28 14:35:35 cisc Exp $

#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------

namespace pc88 {

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
  enum KeyType { AT106 = 0, PC98 = 1 };
  enum CPUType {
    ms11 = 0,
    ms21,
    msauto,
  };

  enum Flags {
    kSubCPUControl = 1 << 0,  // Sub CPU の駆動を制御する
    kSaveDirectory = 1 << 1,  // 起動時に前回終了時のディレクトリに移動
    kFullspeed = 1 << 2,         // 全力動作
    kEnablePad = 1 << 3,         // パッド有効
    kEnableOPNA = 1 << 4,        // OPNA モード (44h)
    kWatchRegister = 1 << 5,     // レジスタ表示
    kAskBeforeReset = 1 << 6,    // 終了・リセット時に確認
    kEnablePCG = 1 << 7,         // PCG 系のフォント書き換えを有効
    kFv15k = 1 << 8,             // 15KHz モニターモード
    kCPUBurst = 1 << 9,          // ノーウェイト
    kSuppressMenu = 1 << 10,     // ALT を GRPH に
    kCPUClockMode = 1 << 11,     // クロック単位で切り替え
    kUseArrowFor10 = 1 << 12,    // 方向キーをテンキーに
    kSwappedButtons = 1 << 13,   // パッドのボタンを入れ替え
    kDisableSing = 1 << 14,      // CMD SING 無効
    kDigitalPalette = 1 << 15,   // ディジタルパレットモード
    kUseQPC = 1 << 16,           // QueryPerformanceCounter つかう
    kForce480 = 1 << 17,         // 全画面を常に 640x480 で
    kOPNOnA8 = 1 << 18,          // OPN (a8h)
    kOPNAOnA8 = 1 << 19,         // OPNA (a8h)
    kDrawPriorityLow = 1 << 20,  // 描画の優先度を落とす
    kDisableF12Reset =
        1 << 21,  // F12 を RESET として使用しない(COPY キーになる)
    kFullline = 1 << 22,       // 偶数ライン表示
    kShowStatusBar = 1 << 23,  // ステータスバー表示
    kShowFDCStatus = 1 << 24,  // FDC のステータスを表示
    kEnableWait = 1 << 25,     // Wait をつける
    kEnableMouse = 1 << 26,    // Mouse を使用
    kMouseJoyMode = 1 << 27,  // Mouse をジョイスティックモードで使用
    kSpecialPalette = 1 << 28,  // デバックパレットモード
    kMixSoundAlways = 1 << 29,  // 処理が重い時も音の合成を続ける
    kPreciseMixing = 1 << 30,   // 高精度な合成を行う
  };
  enum Flag2 {
    kDisableOPN44 = 1 << 0,   // OPN(44h) を無効化 (V2 モード時は OPN)
    kUseWaveOutDrv = 1 << 1,  // PCM の再生に waveOut を使用する
    kMask0 = 1 << 2,          // 選択表示モード
    kMask1 = 1 << 3,
    kMask2 = 1 << 4,
    kGenScrnshotName = 1 << 5,  // スクリーンショットファイル名を自動生成
    kUseFMClock = 1 << 6,  // FM 音源の合成に本来のクロックを使用
    kCompressSnapshot = 1 << 7,  // スナップショットを圧縮する
    kSyncToVSync = 1 << 8,       // 全画面モード時 vsync と同期する
    kShowPlacesBar = 1 << 9,  // ファイルダイアログで PLACESBAR を表示する
    kLPFEnable = 1 << 10,  // LPF を使ってみる
    kFDDNoWait = 1 << 11,  // FDD ノーウェイト
    kUseDSNotify = 1 << 12,
  };

  int flags;
  int flag2;
  int clock;
  int speed;
  int refreshtiming;
  int mainsubratio;
  int opnclock;
  int sound;
  int erambanks;
  int keytype;
  int volfm, volssg, voladpcm, volrhythm;
  int volbd, volsd, voltop, volhh, voltom, volrim;
  int dipsw;
  uint32_t soundbuffer;
  uint32_t mousesensibility;
  int cpumode;
  uint32_t lpffc;  // LPF のカットオフ周波数 (Hz)
  uint32_t lpforder;
  int romeolatency;

  BASICMode basicmode;

  // 15kHz モードの判定を行う．
  // (条件: option 又は N80/SR モード時)
  bool IsFV15k() const { return (basicmode & 2) || (flags & kFv15k); }
};
}  // namespace pc88
