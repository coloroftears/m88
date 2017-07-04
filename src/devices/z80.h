// ---------------------------------------------------------------------------
//  Z80 emulator.
//  Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//  Register definition Z80
// ---------------------------------------------------------------------------
//  $Id: Z80.h,v 1.1.1.1 1999/02/19 09:00:40 cisc Exp $

#pragma once

#include "common/types.h"

#define Z80_WORDREG_IN_INT

struct Z80Reg {
#ifdef Z80_WORDREG_IN_INT
#define PAD(p) uint8_t p[sizeof(uint32_t) - sizeof(uint16_t)]
  using wordreg = uint32_t;
#else
#define PAD(p)
  using wordreg = uint16_t;
#endif

  union regs {
    struct shorts {
      wordreg af;
      wordreg hl, de, bc, ix, iy, sp;
    } w;
    struct words {
#ifdef ENDIAN_IS_BIG
      PAD(p1);
      uint8_t a, flags;
      PAD(p2);
      uint8_t h, l;
      PAD(p3);
      uint8_t d, e;
      PAD(p4);
      uint8_t b, c;
      PAD(p5);
      uint8_t xh, xl;
      PAD(p6);
      uint8_t yh, yl;
      PAD(p7);
      uint8_t sph, spl;
#else
      uint8_t flags, a;
      PAD(p1);
      uint8_t l, h;
      PAD(p2);
      uint8_t e, d;
      PAD(p3);
      uint8_t c, b;
      PAD(p4);
      uint8_t xl, xh;
      PAD(p5);
      uint8_t yl, yh;
      PAD(p6);
      uint8_t spl, sph;
      PAD(p7);
#endif
    } b;
  } r;

  // Shadow register sets
  wordreg r_af, r_hl, r_de, r_bc;
  wordreg pc;
  uint8_t ireg;
  // R (bit 0-6) / R(bit7)
  uint8_t rreg, rreg7;
  // Intterupt mode (0,1,2)
  uint8_t intmode;
  bool iff1, iff2;
};

#undef PAD
