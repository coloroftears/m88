// ----------------------------------------------------------------------------
//  M88 - PC-8801 series emulator
//  Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//  $Id: error.cpp,v 1.6 2002/04/07 05:40:08 cisc Exp $

#include "headers.h"
#include "error.h"

Error::Errno Error::err = Error::unknown;

const char* Error::ErrorText[Error::nerrors] = {
    "�����s���̃G���[���������܂���.",
    "PC88 �� ROM �t�@�C����������܂���.\n�t�@�C�������m�F���Ă�������.",
    "�������̊��蓖�ĂɎ��s���܂���.", "��ʂ̏������Ɏ��s���܂���.",
    "�X���b�h���쐬�ł��܂���.", "�e�L�X�g�t�H���g��������܂���.",
    "���s�t�@�C��������������ꂽ���ꂪ����܂�.\n�̈ӂłȂ���΃E�B���X������"
    "�^���܂�.",
};

const char* Error::GetErrorText() {
  return ErrorText[err];
}

void Error::SetError(Errno e) {
  err = e;
}
