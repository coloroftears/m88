// ---------------------------------------------------------------------------
//  FM Sound Generator
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: fmgen.h,v 1.37 2003/08/25 13:33:11 cisc Exp $
//
// ---------------------------------------------------------------------------
//  FM Sound Generator
//  Copyright (C) cisc 1998, 2003.
// ---------------------------------------------------------------------------
//  $Id: fmgeninl.h,v 1.27 2003/09/10 13:22:50 cisc Exp $

#pragma once

#include <stdint.h>

#include "common/clamp.h"

// ---------------------------------------------------------------------------
//  定数その１
//  静的テーブルのサイズ

const int FM_LFOBITS = 8;  // 変更不可
const int FM_TLBITS = 7;

const int FM_TLENTS = (1 << FM_TLBITS);
const int FM_LFOENTS = (1 << FM_LFOBITS);
const int FM_TLPOS(FM_TLENTS / 4);

//  サイン波の精度は 2^(1/256)
const int FM_CLENTS = (0x1000 * 2);  // sin + TL + LFO

// ---------------------------------------------------------------------------

namespace fmgen {

// Output sample precision (int32_t or int16_t)
using Sample = int32_t;
// Internal sample precision
using ISample = int32_t;

enum OpType { typeN = 0, typeM = 1 };

inline void StoreSample(Sample& dest, ISample data) {
  if (sizeof(Sample) == 2)
    dest = (Sample)Limit(dest + data, 0x7fff, -0x8000);
  else
    dest += data;
}

class Chip;

//  Operator -------------------------------------------------------------
class Operator {
 public:
  Operator();
  void SetChip(Chip* chip) { chip_ = chip; }

  ISample Calc(ISample in);
  ISample CalcL(ISample in);
  ISample CalcFB(uint32_t fb);
  ISample CalcFBL(uint32_t fb);
  ISample CalcN(uint32_t noise);
  void Prepare();
  void KeyOn();
  void KeyOff();
  void Reset();
  void ResetFB();
  int IsOn();

  void SetDT(uint32_t dt);
  void SetDT2(uint32_t dt2);
  void SetMULTI(uint32_t multi);
  void SetTL(uint32_t tl, bool csm);
  void SetKS(uint32_t ks);
  void SetAR(uint32_t ar);
  void SetDR(uint32_t dr);
  void SetSR(uint32_t sr);
  void SetRR(uint32_t rr);
  void SetSL(uint32_t sl);
  void SetSSGEC(uint32_t ssgec);
  void SetFNum(uint32_t fnum);
  void SetDPBN(uint32_t dp, uint32_t bn);
  void SetAMON(bool on);
  void SetMS(uint32_t ms);
  void Mute(bool);

  int Out() { return out_; }

  int dbgGetIn2() { return in2_; }
  void dbgStopPG() {
    pg_diff_ = 0;
    pg_diff_lfo_ = 0;
  }

 private:
  using Counter = uint32_t;

  Chip* chip_;
  ISample out_, out2_;
  ISample in2_;

  //  Phase Generator ------------------------------------------------------
  uint32_t PGCalc();
  uint32_t PGCalcL();

  uint32_t dp_;          // ΔP
  uint32_t detune_;      // Detune
  uint32_t detune2_;     // DT2
  uint32_t multiple_;    // Multiple
  uint32_t pg_count_;    // Phase 現在値
  uint32_t pg_diff_;     // Phase 差分値
  int32_t pg_diff_lfo_;  // Phase 差分値 >> x

  //  Envelope Generator ---------------------------------------------------
  enum EGPhase { next, attack, decay, sustain, release, off };

  void EGCalc();
  void EGStep();
  void ShiftPhase(EGPhase nextphase);
  void SetEGRate(uint32_t);
  void EGUpdate();
  ISample LogToLin(uint32_t a);

  OpType type_;                 // OP の種類 (M, N...)
  uint32_t bn_;                 // Block/Note
  int eg_level_;                // EG の出力値
  int eg_level_on_next_phase_;  // 次の eg_phase_ に移る値
  int eg_count_;                // EG の次の変移までの時間
  int eg_count_diff_;           // eg_count_ の差分
  int eg_out_;                  // EG+TL を合わせた出力値
  int tl_out_;                  // TL 分の出力値
                                //      int     pm_depth_;      // PM depth
                                //      int     am_depth_;      // AM depth
  int eg_rate_;
  int eg_curve_count_;
  int ssg_offset_;
  int ssg_vector_;
  int ssg_phase_;

  uint32_t key_scale_rate_;  // key scale rate
  EGPhase eg_phase_;
  uint32_t* ams_;
  uint32_t ms_;

  uint32_t tl_;        // Total Level   (0-127)
  uint32_t tl_latch_;  // Total Level Latch (for CSM mode)
  uint32_t ar_;        // Attack Rate   (0-63)
  uint32_t dr_;        // Decay Rate    (0-63)
  uint32_t sr_;        // Sustain Rate  (0-63)
  uint32_t sl_;        // Sustain Level (0-127)
  uint32_t rr_;        // Release Rate  (0-63)
  uint32_t ks_;        // Keyscale      (0-3)
  uint32_t ssg_type_;  // SSG-Type Envelope Control

  bool keyon_;
  bool amon_;           // enable Amplitude Modulation
  bool param_changed_;  // パラメータが更新された
  bool mute_;

  //  Tables ---------------------------------------------------------------
  static Counter rate_table[16];
  static uint32_t multable[4][16];

  static const uint8_t notetable[128];
  static const int8_t dttable[256];
  static const int8_t decaytable1[64][8];
  static const int decaytable2[16];
  static const int8_t attacktable[64][8];
  static const int ssgenvtable[8][2][3][2];

  static uint32_t sinetable[1024];
  static int32_t cltable[FM_CLENTS];

  static bool tablehasmade;
  static void MakeTable();

  //  friends --------------------------------------------------------------
  friend class Channel4;

 public:
  int dbgopout_;
  int dbgpgout_;
  static const int32_t* dbgGetClTable() { return cltable; }
  static const uint32_t* dbgGetSineTable() { return sinetable; }
};

//  4-op Channel ---------------------------------------------------------
class Channel4 {
 public:
  Channel4();
  void SetChip(Chip* chip);
  void SetType(OpType type);

  ISample Calc();
  ISample CalcL();
  ISample CalcN(uint32_t noise);
  ISample CalcLN(uint32_t noise);
  void SetFNum(uint32_t fnum);
  void SetFB(uint32_t fb);
  void SetKCKF(uint32_t kc, uint32_t kf);
  void SetAlgorithm(uint32_t algo);
  int Prepare();
  void KeyControl(uint32_t key);
  void Reset();
  void SetMS(uint32_t ms);
  void Mute(bool);
  void Refresh();

  void dbgStopPG() {
    for (int i = 0; i < 4; i++)
      op[i].dbgStopPG();
  }

 private:
  static const uint8_t fbtable[8];
  uint32_t fb;
  int buf[4];
  int* in[3];   // 各 OP の入力ポインタ
  int* out[3];  // 各 OP の出力ポインタ
  int* pms;
  int algo_;
  Chip* chip_;

  static void MakeTable();

  static bool tablehasmade;
  static int kftable[64];

 public:
  Operator op[4];
};

//  Chip resource
class Chip {
 public:
  Chip();
  void SetRatio(uint32_t ratio);
  void SetAMLevel(uint32_t l);
  void SetPMLevel(uint32_t l);
  void SetPMV(int pmv) { pmv_ = pmv; }

  uint32_t GetMulValue(uint32_t dt2, uint32_t mul) {
    return multable_[dt2][mul];
  }
  uint32_t GetAML() { return aml_; }
  uint32_t GetPML() { return pml_; }
  int GetPMV() { return pmv_; }
  uint32_t GetRatio() { return ratio_; }

 private:
  void MakeTable();

  uint32_t ratio_;
  uint32_t aml_;
  uint32_t pml_;
  int pmv_;
  OpType optype_;
  uint32_t multable_[4][16];
};
}  // namespace fmgen

// ---------------------------------------------------------------------------
//  定数その２
//
#define FM_PI 3.14159265358979323846

#define FM_SINEPRESIS 2  // EGとサイン波の精度の差  0(低)-2(高)

#define FM_OPSINBITS 10
#define FM_OPSINENTS (1 << FM_OPSINBITS)

#define FM_EGCBITS 18  // eg の count のシフト値
#define FM_LFOCBITS 14

#ifdef FM_TUNEBUILD
#define FM_PGBITS 2
#define FM_RATIOBITS 0
#else
#define FM_PGBITS 9
#define FM_RATIOBITS 7  // 8-12 くらいまで？
#endif

#define FM_EGBITS 16

// extern int paramcount[];
//#define PARAMCHANGE(i) paramcount[i]++;
#define PARAMCHANGE(i)

namespace fmgen {

// ---------------------------------------------------------------------------
//  Operator
//
//  フィードバックバッファをクリア
inline void Operator::ResetFB() {
  out_ = out2_ = 0;
}

//  キーオン
inline void Operator::KeyOn() {
  if (!keyon_) {
    keyon_ = true;
    if (eg_phase_ == off || eg_phase_ == release) {
      ssg_phase_ = -1;
      ShiftPhase(attack);
      EGUpdate();
      in2_ = out_ = out2_ = 0;
      pg_count_ = 0;
    }
  }
}

//  キーオフ
inline void Operator::KeyOff() {
  if (keyon_) {
    keyon_ = false;
    ShiftPhase(release);
  }
}

//  オペレータは稼働中か？
inline int Operator::IsOn() {
  return eg_phase_ - off;
}

//  Detune (0-7)
inline void Operator::SetDT(uint32_t dt) {
  detune_ = dt * 0x20, param_changed_ = true;
  PARAMCHANGE(4);
}

//  DT2 (0-3)
inline void Operator::SetDT2(uint32_t dt2) {
  detune2_ = dt2 & 3, param_changed_ = true;
  PARAMCHANGE(5);
}

//  Multiple (0-15)
inline void Operator::SetMULTI(uint32_t mul) {
  multiple_ = mul, param_changed_ = true;
  PARAMCHANGE(6);
}

//  Total Level (0-127) (0.75dB step)
inline void Operator::SetTL(uint32_t tl, bool csm) {
  if (!csm) {
    tl_ = tl, param_changed_ = true;
    PARAMCHANGE(7);
  }
  tl_latch_ = tl;
}

//  Attack Rate (0-63)
inline void Operator::SetAR(uint32_t ar) {
  ar_ = ar;
  param_changed_ = true;
  PARAMCHANGE(8);
}

//  Decay Rate (0-63)
inline void Operator::SetDR(uint32_t dr) {
  dr_ = dr;
  param_changed_ = true;
  PARAMCHANGE(9);
}

//  Sustain Rate (0-63)
inline void Operator::SetSR(uint32_t sr) {
  sr_ = sr;
  param_changed_ = true;
  PARAMCHANGE(10);
}

//  Sustain Level (0-127)
inline void Operator::SetSL(uint32_t sl) {
  sl_ = sl;
  param_changed_ = true;
  PARAMCHANGE(11);
}

//  Release Rate (0-63)
inline void Operator::SetRR(uint32_t rr) {
  rr_ = rr;
  param_changed_ = true;
  PARAMCHANGE(12);
}

//  Keyscale (0-3)
inline void Operator::SetKS(uint32_t ks) {
  ks_ = ks;
  param_changed_ = true;
  PARAMCHANGE(13);
}

//  SSG-type Envelope (0-15)
inline void Operator::SetSSGEC(uint32_t ssgec) {
  if (ssgec & 8)
    ssg_type_ = ssgec;
  else
    ssg_type_ = 0;
  param_changed_ = true;
}

inline void Operator::SetAMON(bool amon) {
  amon_ = amon;
  param_changed_ = true;
  PARAMCHANGE(14);
}

inline void Operator::Mute(bool mute) {
  mute_ = mute;
  param_changed_ = true;
  PARAMCHANGE(15);
}

inline void Operator::SetMS(uint32_t ms) {
  ms_ = ms;
  param_changed_ = true;
  PARAMCHANGE(16);
}

// ---------------------------------------------------------------------------
//  4-op Channel

//  オペレータの種類 (LFO) を設定
inline void Channel4::SetType(OpType type) {
  for (int i = 0; i < 4; i++)
    op[i].type_ = type;
}

//  セルフ・フィードバックレートの設定 (0-7)
inline void Channel4::SetFB(uint32_t feedback) {
  fb = fbtable[feedback];
}

//  OPNA 系 LFO の設定
inline void Channel4::SetMS(uint32_t ms) {
  op[0].SetMS(ms);
  op[1].SetMS(ms);
  op[2].SetMS(ms);
  op[3].SetMS(ms);
}

//  チャンネル・マスク
inline void Channel4::Mute(bool m) {
  for (int i = 0; i < 4; i++)
    op[i].Mute(m);
}

//  内部パラメータを再計算
inline void Channel4::Refresh() {
  for (int i = 0; i < 4; i++)
    op[i].param_changed_ = true;
  PARAMCHANGE(3);
}

inline void Channel4::SetChip(Chip* chip) {
  chip_ = chip;
  for (int i = 0; i < 4; i++)
    op[i].SetChip(chip);
}

inline void Chip::SetAMLevel(uint32_t l) {
  aml_ = l & (FM_LFOENTS - 1);
}

inline void Chip::SetPMLevel(uint32_t l) {
  pml_ = l & (FM_LFOENTS - 1);
}
}  // namespace fmgen
