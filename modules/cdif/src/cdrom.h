// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: cdrom.h,v 1.1 1999/08/26 08:04:37 cisc Exp $

#ifndef incl_cdrom_h
#define incl_cdrom_h

#include "win32/types.h"
#include "common/misc.h"

class ASPI;

// ---------------------------------------------------------------------------
//  CDROM ����N���X
//
class CDROM {
 public:
  struct Track {
    uint32_t addr;
    uint32_t control;  // b2 = data/~audio
  };
  struct MSF {
    uint8_t min;
    uint8_t sec;
    uint8_t frame;
  };

 public:
  CDROM();
  ~CDROM();

  int ReadTOC();
  int GetNumTracks();
  bool Init();
  bool PlayTrack(int tr, bool one = false);
  bool PlayAudio(uint32_t begin, uint32_t stop);
  bool ReadSubCh(uint8_t* dest, bool msf);
  bool Pause(bool pause);
  bool Stop();
  bool Read(uint32_t sector, uint8_t* dest, int length);
  bool Read2(uint32_t sector, uint8_t* dest, int length);
  bool ReadCDDA(uint32_t sector, uint8_t* dest, int length);
  const Track* GetTrackInfo(int t);
  bool CheckMedia();
  MSF ToMSF(uint32_t lba);
  uint32_t ToLBA(MSF msf);

 private:
  bool FindDrive();

  ASPI* aspi;
  int ha;       // CD-ROM �h���C�u�̐ڑ�����Ă���z�X�g�A�_�v�^
  int id;       // CD-ROM �h���C�u�� ID
  int ntracks;  // �g���b�N��
  int trstart;  // �g���b�N�̊J�n�ʒu

  Track track[100];
};

// ---------------------------------------------------------------------------
//  LBA ���Ԃ� MSF ���Ԃɕϊ�
//
inline CDROM::MSF CDROM::ToMSF(uint32_t lba) {
  lba += trstart;
  MSF msf;
  msf.min = NtoBCD(lba / 75 / 60);
  msf.sec = NtoBCD(lba / 75 % 60);
  msf.frame = NtoBCD(lba % 75);
  return msf;
}

// ---------------------------------------------------------------------------
//  LBA ���Ԃ� MSF ���Ԃɕϊ�
//
inline uint32_t CDROM::ToLBA(MSF msf) {
  return (BCDtoN(msf.min) * 60 + BCDtoN(msf.sec)) * 75 + BCDtoN(msf.frame) -
         trstart;
}

// ---------------------------------------------------------------------------
//  CD �̃g���b�N�����擾
//  ReadTOC ��ɗL��
//
inline const CDROM::Track* CDROM::GetTrackInfo(int t) {
  if (t < 0 || t > ntracks + 1)
    return 0;
  return &track[t ? t - 1 : ntracks];
}

// ---------------------------------------------------------------------------
//  CD ���̃g���b�N�����擾
//  ReadTOC ��ɗL��
//
inline int CDROM::GetNumTracks() {
  return ntracks;
}

#endif  // incl_cdrom_h
