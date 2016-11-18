// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: timekeep.h,v 1.1 2002/04/07 05:40:11 cisc Exp $

#if !defined(win32_timekeep_h)
#define win32_timekeep_h

#include "types.h"

// ---------------------------------------------------------------------------
//  TimeKeeper
//  ���݂̎��Ԃ𐔒l(1/unit �~���b�P��)�ŗ^����N���X�D
//  GetTime() �ŗ^������l���͓̂��ʂȈӖ��������Ă��炸�C
//  �A�������Ăяo�����s���Ƃ��C�Ԃ����l�̍������C
//  ���̌Ăяo���̊ԂɌo�߂������Ԃ������D
//  �����CGetTime() ���Ă�ł��� N (1/unit �~���b) ��� GetTime() ���ĂԂƁC
//  2�x�ڂɕԂ����l�͍ŏ��ɕԂ����l��� N ������D  
//
class TimeKeeper
{
public:
    enum
    {
        unit = 100,         // �Œ� 1 �Ƃ������ƂŁD
    };

public:
    TimeKeeper();
    ~TimeKeeper();

    uint32 GetTime();

private:
    uint32 freq;            // �\�[�X�N���b�N�̎���
    uint32 base;            // �Ō�̌Ăяo���̍ۂ̌��N���b�N�̒l
    uint32 time;            // �Ō�̌Ăяo���ɕԂ����l
};

#endif // !defined(win32_timekeep_h)
