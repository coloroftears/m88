// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: floppy.h,v 1.5 2000/02/29 12:29:52 cisc Exp $

#pragma once

#include <memory>

#include "common/types.h"

// ---------------------------------------------------------------------------
//  フロッピーディスク
//
class FloppyDisk final {
 public:
  enum SectorFlags {
    DELETED = 1,
    DATA_CRC = 2,
    ID_CRC = 4,
    MAM = 8,
    DENSITY = 0x40,       // MFM = 0x40, FM = 0x00
    HIGH_DENSITY = 0x80,  // 2HD?
  };
  enum DiskType { MD2D = 0, MD2DD, MD2HD };
  struct IDR {
    uint8_t c, h, r, n;

    bool operator==(const IDR& i) {
      return ((c == i.c) && (h == i.h) && (r == i.r) && (n == i.n));
    }
  };

  struct Sector {
    IDR id;
    uint32_t flags;
    std::unique_ptr<uint8_t[]> image;
    uint32_t size;
    Sector* next;
  };

  class Track {
   public:
    Track() : sector(0) {}
    ~Track() {
      for (Sector* s = sector; s;) {
        Sector* n = s->next;
        delete s;
        s = n;
      }
    }

    Sector* sector;
  };

 public:
  FloppyDisk();
  ~FloppyDisk();

  bool Init(DiskType type, bool readonly_);

  bool IsReadOnly() { return readonly_; }
  DiskType GetType() { return disk_type_; }

  void Seek(uint32_t tr);
  Sector* GetSector();
  bool FindID(IDR idr, uint32_t density);
  uint32_t GetNumSectors();
  uint32_t GetTrackCapacity();
  uint32_t GetTrackSize();
  uint32_t GetNumTracks() { return num_tracks_; }
  bool Resize(Sector* sector, uint32_t newsize);
  bool FormatTrack(int nsec, int secsize);
  Sector* AddSector(int secsize);
  Sector* GetFirstSector(uint32_t track);
  void IndexHole() { current_sector_ = 0; }

 private:
  Track tracks_[168];
  int num_tracks_;
  DiskType disk_type_;
  bool readonly_;

  Track* current_track_;
  Sector* current_sector_;
  uint32_t current_track_number_;
};
