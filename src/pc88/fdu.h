// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: fdu.h,v 1.2 1999/03/25 11:29:20 cisc Exp $

#pragma once

#include "pc88/floppy.h"

class DiskManager;

namespace PC8801 {

// ---------------------------------------------------------------------------
//  FDU
//  フロッピードライブをエミュレーションするクラス
//
//  ReadSector(ID id, uint8_t* data);
//  セクタを読む
//
//  WriteSector(ID id, uint8_t* data);
//
class FDU {
 public:
  using IDR = FloppyDisk::IDR;
  struct WIDDESC {
    IDR* idr;
    uint8_t n, sc, gpl, d;
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
  uint32_t ReadSector(uint32_t flags, IDR id, uint8_t* data);
  uint32_t WriteSector(uint32_t flags,
                       IDR id,
                       const uint8_t* data,
                       bool deleted);
  uint32_t Seek(uint32_t cyrinder);
  uint32_t SenceDeviceStatus();
  uint32_t ReadID(uint32_t flags, IDR* id);
  uint32_t WriteID(uint32_t flags, WIDDESC* wid);
  uint32_t FindID(uint32_t flags, IDR id);
  uint32_t ReadDiag(uint8_t* data, uint8_t** cursor, IDR idr);
  uint32_t MakeDiagData(uint32_t flags, uint8_t* data, uint32_t* size);

 private:
  struct DiagInfo {
    IDR idr;
    uint8_t* data;
  };

  void SetHead(uint32_t hd);

  FloppyDisk* disk;
  FloppyDisk::Sector* sector;
  DiskManager* diskmgr;
  int cyrinder;
  int head;
  int drive;
  int track;
};
}  // namespace PC8801
