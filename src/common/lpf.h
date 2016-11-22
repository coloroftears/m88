// ---------------------------------------------------------------------------
//  IIR LPF
//  Copyright (C) cisc 2001.
// ---------------------------------------------------------------------------
//  $Id: lpf.h,v 1.1 2002/04/07 05:40:08 cisc Exp $

#pragma once

#include "common/types.h"

// ---------------------------------------------------------------------------
//  フィルタ
//
class IIR_LPF {
  enum {
    maxorder = 8,
    nchs = 2,
    F = 4096,
  };

 public:
  IIR_LPF() : order(0) {}

  void MakeFilter(uint32_t cutoff, uint32_t pcmrate, uint32_t order);
  int Filter(uint32_t ch, int o);

 private:
  uint32_t order;
  int fn[maxorder][4];
  int b[nchs][maxorder][2];
};

// ---------------------------------------------------------------------------

inline int IIR_LPF::Filter(uint32_t ch, int o) {
  for (uint32_t j = 0; j < order; j++) {
    int p = o + (b[ch][j][0] * fn[j][0] + b[ch][j][1] * fn[j][1]) / F;
    o = (p * fn[j][2] + b[ch][j][0] * fn[j][3] + b[ch][j][1] * fn[j][2]) / F;
    b[ch][j][1] = b[ch][j][0];
    b[ch][j][0] = p;
  }
  return o;
}
