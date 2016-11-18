// ---------------------------------------------------------------------------
//  class SoundBuffer
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: soundbuf.h,v 1.7 2002/04/07 05:40:08 cisc Exp $

#ifndef common_soundbuf_h
#define common_soundbuf_h

#include "win32/types.h"
#include "win32/critsect.h"
#include "if/ifcommon.h"

// ---------------------------------------------------------------------------
//  SoundBuffer
//
class SoundBuffer {
 public:
  typedef int16 Sample;

 public:
  SoundBuffer();
  ~SoundBuffer();

  bool Init(int nch, int bufsize);  // bufsize �̓T���v���P��
  void Cleanup();

  void Put(int sample);               // �o�b�t�@�ɍő� sample ���f�[�^��ǉ�
  void Get(Sample* ptr, int sample);  // �o�b�t�@���� sample ���̃f�[�^�𓾂�
  bool IsEmpty();
  void FillWhenEmpty(bool f);  // �o�b�t�@����ɂȂ������[���邩

 private:
  virtual void Mix(Sample* b1,
                   int s1,
                   Sample* b2 = 0,
                   int s2 = 0) = 0;  // sample ���̃f�[�^����
  void PutMain(int sample);

  Sample* buffer;
  CriticalSection cs;

  int buffersize;  // �o�b�t�@�̃T�C�Y (in samples)
  int read;        // �Ǎ��ʒu (in samples)
  int write;       // �������݈ʒu (in samples)
  int ch;          // �`���l����(1sample = ch*Sample)
  bool fillwhenempty;
};

// ---------------------------------------------------------------------------

inline void SoundBuffer::FillWhenEmpty(bool f) {
  fillwhenempty = f;
}

#endif  // common_soundbuf_h
