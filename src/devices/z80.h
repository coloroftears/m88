// ---------------------------------------------------------------------------
//  Z80 emulator.
//  Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//  Z80 �̃��W�X�^��`
// ---------------------------------------------------------------------------
//  $Id: Z80.h,v 1.1.1.1 1999/02/19 09:00:40 cisc Exp $

#ifndef Z80_H
#define Z80_H

#include "win32/types.h"

#define Z80_WORDREG_IN_INT

// ---------------------------------------------------------------------------
//  Z80 �̃��W�X�^�Z�b�g
// ---------------------------------------------------------------------------

struct Z80Reg {
#ifdef Z80_WORDREG_IN_INT
#define PAD(p) uint8_t p[sizeof(uint) - sizeof(uint16_t)]
  typedef uint wordreg;
#else
#define PAD(p)
  typedef uint16_t wordreg;
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

  wordreg r_af, r_hl, r_de, r_bc; /* �����W�X�^ */
  wordreg pc;
  uint8_t ireg;
  uint8_t rreg, rreg7; /* R(0-6 bit), R(7th bit) */
  uint8_t intmode;     /* ���荞�݃��[�h */
  bool iff1, iff2;
};

#undef PAD

#endif /* Z80_H */
