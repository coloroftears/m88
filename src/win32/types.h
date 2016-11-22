// ----------------------------------------------------------------------------
//  M88 - PC-8801 series emulator
//  Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//  $Id: types.h,v 1.10 1999/12/28 10:34:53 cisc Exp $

#if !defined(win32_types_h)
#define win32_types_h

#include <stdint.h>

#define ENDIAN_IS_SMALL

// 8 bit ���l���܂Ƃ߂ď�������Ƃ��Ɏg���^
typedef uint32_t packed;
#define PACK(p) ((p) | ((p) << 8) | ((p) << 16) | ((p) << 24))

// �{�C���^�l��\���ł��鐮���^
typedef uint32_t intpointer;

// �֐��ւ̃|�C���^�ɂ����� ��� 0 �ƂȂ�r�b�g (1 bit �̂�)
// �Ȃ���� PTR_IDBIT ���̂� define ���Ȃ��ł��������D
// (x86 �� Z80 �G���W���ł͕K�{)

#if defined(_DEBUG)
#define PTR_IDBIT 0x80000000
#else
#define PTR_IDBIT 0x1
#endif

// ���[�h���E���z����A�N�Z�X������
#define ALLOWBOUNDARYACCESS

// x86 �ł� Z80 �G���W�����g�p����
#define USE_Z80_X86

// C++ �̐V�����L���X�g���g�p����(�A�� win32 �R�[�h�ł͊֌W�Ȃ��g�p����)
#define USE_NEW_CAST

// ---------------------------------------------------------------------------

#ifdef USE_Z80_X86
#define MEMCALL __stdcall
#else
#define MEMCALL
#endif

#if defined(USE_NEW_CAST) && defined(__cplusplus)
#define STATIC_CAST(t, o) static_cast<t>(o)
#define REINTERPRET_CAST(t, o) reinterpret_cast<t>(o)
#else
#define STATIC_CAST(t, o) ((t)(o))
#define REINTERPRET_CAST(t, o) (*(t*)(void*)&(o))
#endif

#endif  // win32_types_h
