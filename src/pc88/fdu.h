// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: fdu.h,v 1.2 1999/03/25 11:29:20 cisc Exp $

#ifndef FDU_H
#define FDU_H

#include "floppy.h"

class DiskManager;

namespace PC8801 {

// ---------------------------------------------------------------------------
//  FDU
//  �t���b�s�[�h���C�u���G�~�����[�V��������N���X
//
//  ReadSector(ID id, uint8* data);
//  �Z�N�^��ǂ�
//
//  WriteSector(ID id, uint8* data);
//
class FDU {
 public:
  typedef FloppyDisk::IDR IDR;
  struct WIDDESC {
    IDR* idr;
    uint8 n, sc, gpl, d;
  };

 public:
  enum Flags {
    MFM = 0x40,
    head1 = 0x01,
  };

  FDU();
  ~FDU();

  bool Init(DiskManager* diskmgr, int dr);

  bool Mount(FloppyDisk* disk);
  bool Unmount();

  bool IsMounted() { return disk != 0; }
  uint ReadSector(uint flags, IDR id, uint8* data);
  uint WriteSector(uint flags, IDR id, const uint8* data, bool deleted);
  uint Seek(uint cyrinder);
  uint SenceDeviceStatus();
  uint ReadID(uint flags, IDR* id);
  uint WriteID(uint flags, WIDDESC* wid);
  uint FindID(uint flags, IDR id);
  uint ReadDiag(uint8* data, uint8** cursor, IDR idr);
  uint MakeDiagData(uint flags, uint8* data, uint* size);

 private:
  struct DiagInfo {
    IDR idr;
    uint8* data;
  };

  void SetHead(uint hd);

  FloppyDisk* disk;
  FloppyDisk::Sector* sector;
  DiskManager* diskmgr;
  int cyrinder;
  int head;
  int drive;
  int track;
};
}

#endif  // FDU_H
