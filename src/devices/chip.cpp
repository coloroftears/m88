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

// ---------------------------------------------------------------------------
//  Table/etc
//
namespace fmgen {
// ---------------------------------------------------------------------------
//  チップ内で共通な部分
//

//  クロック・サンプリングレート比に依存するテーブルを作成
void Chip::SetRatio(uint32_t ratio) {
  if (ratio_ != ratio) {
    ratio_ = ratio;
    MakeTable();
  }
}

void Chip::MakeTable() {
  // PG Part
  static const float dt2lv[4] = {1.0f, 1.414f, 1.581f, 1.732f};
  for (int h = 0; h < 4; ++h) {
    assert(2 + FM_RATIOBITS - FM_PGBITS >= 0);
    double rr = dt2lv[h] * double(ratio_);
    for (int l = 0; l < 16; ++l) {
      int mul = l ? l * 2 : 1;
      multable_[h][l] = uint32_t(mul * rr);
    }
  }
}

}  // namespace fmgen
