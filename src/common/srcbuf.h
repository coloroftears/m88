//  $Id: srcbuf.h,v 1.2 2003/05/12 22:26:34 cisc Exp $

#ifndef common_srcbuf_h
#define common_srcbuf_h

#include "win32/critsect.h"
#include "common/soundsrc.h"

// ---------------------------------------------------------------------------
//  SamplingRateConverter
//
class SamplingRateConverter : public SoundSource {
 public:
  SamplingRateConverter();
  ~SamplingRateConverter();

  bool Init(SoundSourceL* source,
            int bufsize,
            uint32 outrate);  // bufsize �̓T���v���P��
  void Cleanup();

  int Get(Sample* dest, int size);
  uint32 GetRate();
  int GetChannels();
  int GetAvail();

  int Fill(int samples);  // �o�b�t�@�ɍő� sample ���f�[�^��ǉ�
  bool IsEmpty();
  void FillWhenEmpty(bool f);  // �o�b�t�@����ɂȂ������[���邩

 private:
  enum {
    osmax = 500,
    osmin = 100,
    M = 30,  // M
  };

  int FillMain(int samples);
  void MakeFilter(uint32 outrate);
  int Avail();

  SoundSourceL* source;
  SampleL* buffer;
  float* h2;

  int buffersize;  // �o�b�t�@�̃T�C�Y (in samples)
  int read;        // �Ǎ��ʒu (in samples)
  int write;       // �������݈ʒu (in samples)
  int ch;          // �`���l����(1sample = ch*Sample)
  bool fillwhenempty;

  int n;
  int nch;
  int oo;
  int ic;
  int oc;

  int outputrate;

  CriticalSection cs;
};

// ---------------------------------------------------------------------------

inline void SamplingRateConverter::FillWhenEmpty(bool f) {
  fillwhenempty = f;
}

inline uint32 SamplingRateConverter::GetRate() {
  return source ? outputrate : 0;
}

inline int SamplingRateConverter::GetChannels() {
  return source ? ch : 0;
}

// ---------------------------------------------------------------------------
//  �o�b�t�@���󂩁C��ɋ߂���Ԃ�?
//
inline int SamplingRateConverter::Avail() {
  if (write >= read)
    return write - read;
  else
    return buffersize + write - read;
}

inline int SamplingRateConverter::GetAvail() {
  CriticalSection::Lock lock(cs);
  return Avail();
}

inline bool SamplingRateConverter::IsEmpty() {
  return GetAvail() == 0;
}

#endif  // common_soundbuf_h
