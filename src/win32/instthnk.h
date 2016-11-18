// ---------------------------------------------------------------------------
//  class InstanceThunk
//  Copyright (C) cisc 1997.
// ---------------------------------------------------------------------------
//  $Id: instthnk.h,v 1.3 1999/07/31 12:42:19 cisc Exp $

#if !defined(WIN32_INSTANCETHUNK_H)
#define WIN32_INSTANCETHUNK_H

// ---------------------------------------------------------------------------
//  InstanceThunk
//  WinProc �Ȃǂ̃R�[���o�b�N�֐��� C++ �N���X�ɓK�����Ղ����邽�߂�
//  ���₵���Ȏ�i�D
//
//  static �� _stdcall ���������֐����Ăяo���ۂɁC
//  �����̍ŏ��ɏ��������Ɏw�肵���|�C���^�ϐ���t�����邱�Ƃ��ł���D
//
//  MFC �� hashed map �Ő؂蔲���Ă���悤�����ǁDOWL �������悤�Ȏ�@��
//  �g���Ă���݂����D
//
//  �g�������ԈႦ��Ƒ��\������̂ŋC�����āD
//
class InstanceThunk {
 public:
  InstanceThunk() {}
  ~InstanceThunk() {}

  void SetDestination(void* func, void* arg0);
  operator void*() { return EntryThunk; }

 private:
  BYTE EntryThunk[16];
};

#endif  // !defined(WIN32_INSTANCETHUNK_H)
