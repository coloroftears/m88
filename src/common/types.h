// ----------------------------------------------------------------------------
//  M88 - PC-8801 series emulator
//  Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//  $Id: types.h,v 1.10 1999/12/28 10:34:53 cisc Exp $

#pragma once

#include <stdint.h>

// 8 bit 数値をまとめて処理するときに使う型
using packed = uint32_t;
#define PACK(p) ((p) | ((p) << 8) | ((p) << 16) | ((p) << 24))

// ---------------------------------------------------------------------------

#define MEMCALL

#ifndef IFCALL
#define IFCALL __stdcall
#endif
#ifndef IOCALL
#define IOCALL __stdcall
#endif
