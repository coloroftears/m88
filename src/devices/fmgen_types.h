#pragma once

#include "common/clamp.h"
#include "common/types.h"

namespace fmgen {

using FMSample = int32_t;  // for output
using ISample = int32_t;   // for internal calculation

inline void StoreSample(FMSample& dest, ISample data) {
  if (sizeof(FMSample) == 2)
    dest = (FMSample)Limit16(dest + data);
  else
    dest += data;
}

// ---------------------------------------------------------------------------
//  静的テーブルのサイズ

const int FM_LFOBITS = 8;  // 変更不可
const unsigned int FM_LFOENTS = 1 << FM_LFOBITS;

const int FM_CLENTS = 0x1000 * 2;  // sin + TL + LFO

const double FM_PI = 3.14159265358979323846;

const int FM_SINEPRESIS = 2;  // EGとサイン波の精度の差  0(低)-2(高)

const int FM_EGCBITS = 18;  // eg の count のシフト値
const int FM_LFOCBITS = 14;

#ifdef FM_TUNEBUILD
const int FM_PGBITS = 2;
const int FM_RATIOBITS = 0;
#else
const int FM_PGBITS = 9;
const int FM_RATIOBITS = 7;  // 8-12 くらいまで？
#endif

}  // namespace fmgen
