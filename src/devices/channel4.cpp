// ---------------------------------------------------------------------------
//  FM Sound Generator - Core Unit
//  Copyright (C) cisc 1998, 2003.
// ---------------------------------------------------------------------------
//  $Id: fmgen.cpp,v 1.50 2003/09/10 13:19:34 cisc Exp $
// ---------------------------------------------------------------------------
//  参考:
//      FM sound generator for M.A.M.E., written by Tatsuyuki Satoh.
//
//  謎:
//      OPNB の CSM モード(仕様がよくわからない)
//
//  制限:
//      ・AR!=31 で SSGEC を使うと波形が実際と異なる可能性あり
//
//  謝辞:
//      Tatsuyuki Satoh さん(fm.c)
//      Hiromitsu Shioya さん(ADPCM-A)
//      DMP-SOFT. さん(OPNB)
//      KAJA さん(test program)
//      ほか掲示板等で様々なご助言，ご支援をお寄せいただいた皆様に
// ---------------------------------------------------------------------------

#include "devices/fmgen.h"

#include <assert.h>
#include <math.h>

namespace fmgen {

// ---------------------------------------------------------------------------
//  4-op Channel
//
int Channel4::kftable[64];

Channel4::Channel4() {
  static bool tablehasmade = false;
  if (!tablehasmade) {
    MakeTable();
    tablehasmade = true;
  }

  SetAlgorithm(0);
  pms_ = pmtable[0][0];
}

void Channel4::MakeTable() {
  // 100/64 cent =  2^(i*100/64*1200)
  for (int i = 0; i < 64; ++i)
    kftable[i] = static_cast<int>(0x10000 * pow(2.0, i / 768.0));
}

//  KC/KF を設定
void Channel4::SetKCKF(uint32_t kc, uint32_t kf) {
  const static uint32_t kctable[16] = {
      5197, 5506, 5833, 6180, 6180, 6547, 6937, 7349,
      7349, 7786, 8249, 8740, 8740, 9259, 9810, 10394,
  };

  int oct = 19 - ((kc >> 4) & 7);

  uint32_t kcv = kctable[kc & 0x0f];
  kcv = (kcv + 2) / 4 * 4;

  uint32_t dp = kcv * kftable[kf & 0x3f];

  dp >>= 16 + 3;
  dp <<= 16 + 3;
  dp >>= oct;

  uint32_t bn = (kc >> 2) & 31;

  op[0].SetDPBN(dp, bn);
  op[1].SetDPBN(dp, bn);
  op[2].SetDPBN(dp, bn);
  op[3].SetDPBN(dp, bn);
}

//  キー制御
void Channel4::KeyControl(uint32_t key) {
  if (key & 0x1)
    op[0].KeyOn();
  else
    op[0].KeyOff();
  if (key & 0x2)
    op[1].KeyOn();
  else
    op[1].KeyOff();
  if (key & 0x4)
    op[2].KeyOn();
  else
    op[2].KeyOff();
  if (key & 0x8)
    op[3].KeyOn();
  else
    op[3].KeyOff();
}

//  アルゴリズムを設定
void Channel4::SetAlgorithm(uint32_t algo) {
  static const int table1[8][6] = {
      {0, 1, 1, 2, 2, 3}, {1, 0, 0, 1, 1, 2}, {1, 1, 1, 0, 0, 2},
      {0, 1, 2, 1, 1, 2}, {0, 1, 2, 2, 2, 1}, {0, 1, 0, 1, 0, 1},
      {0, 1, 2, 1, 2, 1}, {1, 0, 1, 0, 1, 0},
  };

  in_[0] = &buf_[table1[algo][0]];
  out_[0] = &buf_[table1[algo][1]];
  in_[1] = &buf_[table1[algo][2]];
  out_[1] = &buf_[table1[algo][3]];
  in_[2] = &buf_[table1[algo][4]];
  out_[2] = &buf_[table1[algo][5]];

  op[0].ResetFB();
  algo_ = algo;
}

}  // namespace fmgen
