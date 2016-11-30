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

// 関数へのポインタにおいて 常に 0 となるビット (1 bit のみ)
// なければ PTR_IDBIT 自体を define しないでください．
// (x86 版 Z80 エンジンでは必須)

#if defined(_DEBUG)
#define PTR_IDBIT 0x80000000
#else
#define PTR_IDBIT 0x1
#endif

// ワード境界を越えるアクセスを許可
#define ALLOWBOUNDARYACCESS

// x86 版の Z80 エンジンを使用する
#define USE_Z80_X86

// ---------------------------------------------------------------------------

#ifdef USE_Z80_X86
#define MEMCALL __stdcall
#else
#define MEMCALL
#endif

#ifndef IFCALL
#define IFCALL __stdcall
#endif
#ifndef IOCALL
#define IOCALL __stdcall
#endif
