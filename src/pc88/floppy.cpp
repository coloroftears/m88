// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: floppy.cpp,v 1.4 1999/07/29 14:35:31 cisc Exp $

#include "pc88/floppy.h"

#include <assert.h>

// ---------------------------------------------------------------------------
//  構築
//
FloppyDisk::FloppyDisk() {
  num_tracks_ = 0;
  current_track_ = 0;
  current_sector_ = 0;
  current_track_number_ = ~0;
  readonly_ = false;
  disk_type_ = MD2D;
}

FloppyDisk::~FloppyDisk() {}

// ---------------------------------------------------------------------------
//  初期化
//
bool FloppyDisk::Init(DiskType _type, bool _readonly) {
  static const int trtbl[] = {84, 164, 164};

  disk_type_ = _type;
  readonly_ = _readonly;
  num_tracks_ = trtbl[disk_type_];

  current_track_ = 0;
  current_sector_ = 0;
  current_track_number_ = ~0;
  return true;
}

// ---------------------------------------------------------------------------
//  指定のトラックにシーク
//
void FloppyDisk::Seek(uint32_t tr) {
  if (tr != current_track_number_) {
    current_track_number_ = tr;
    current_track_ = tr < 168 ? tracks_ + tr : 0;
    current_sector_ = 0;
  }
}

// ---------------------------------------------------------------------------
//  セクタ一つ読み出し
//
FloppyDisk::Sector* FloppyDisk::GetSector() {
  if (!current_sector_) {
    if (current_track_)
      current_sector_ = current_track_->sector;
  }

  Sector* ret = current_sector_;

  if (current_sector_)
    current_sector_ = current_sector_->next;

  return ret;
}

// ---------------------------------------------------------------------------
//  指定した ID を検索
//
bool FloppyDisk::FindID(IDR idr, uint32_t density) {
  if (!current_track_)
    return false;

  Sector* first = current_sector_;
  do {
    if (current_sector_) {
      if (current_sector_->id == idr) {
        if ((current_sector_->flags & 0xc0) == (density & 0xc0))
          return true;
      }
      current_sector_ = current_sector_->next;
    } else {
      current_sector_ = current_track_->sector;
    }
  } while (current_sector_ != first);

  return false;
}

// ---------------------------------------------------------------------------
//  セクタ数を得る
//
uint32_t FloppyDisk::GetNumSectors() {
  int n = 0;
  if (current_track_) {
    Sector* sec = current_track_->sector;
    while (sec) {
      sec = sec->next;
      n++;
    }
  }
  return n;
}

// ---------------------------------------------------------------------------
//  トラック中のセクタデータの総量を得る
//
uint32_t FloppyDisk::GetTrackSize() {
  int size = 0;

  if (current_track_) {
    Sector* sec = current_track_->sector;
    while (sec) {
      size += sec->size;
      sec = sec->next;
    }
  }
  return size;
}

// ---------------------------------------------------------------------------
//  Floppy::Resize
//  セクタのサイズを大きくした場合におけるセクタ潰しの再現
//  sector は現在選択しているトラックに属している必要がある．
//
bool FloppyDisk::Resize(Sector* sec, uint32_t newsize) {
  assert(current_track_ && sec);

  int extend = newsize - sec->size - 0x40;

  // sector 自身の resize
  sec->image.reset(new uint8_t[newsize]);
  sec->size = newsize;

  if (!sec->image) {
    sec->size = 0;
    return false;
  }

  current_sector_ = sec->next;
  while (extend > 0 && current_sector_) {
    Sector* next = current_sector_->next;
    extend -= current_sector_->size + 0xc0;
    delete current_sector_;
    sec->next = current_sector_ = next;
  }
  if (extend > 0) {
    int gapsize = GetTrackCapacity() - GetTrackSize() - 0x60 * GetNumSectors();
    extend -= gapsize;
  }
  while (extend > 0 && current_sector_) {
    Sector* next = current_sector_->next;
    extend -= current_sector_->size + 0xc0;
    delete current_sector_;
    current_track_->sector = current_sector_ = next;
  }
  if (extend > 0)
    return false;

  return true;
}

// ---------------------------------------------------------------------------
//  FloppyDisk::FormatTrack
//
bool FloppyDisk::FormatTrack(int nsec, int secsize) {
  Sector* sec;

  if (!current_track_)
    return false;

  // 今あるトラックを破棄
  sec = current_track_->sector;
  while (sec) {
    Sector* next = sec->next;
    delete sec;
    sec = next;
  }
  current_track_->sector = 0;

  if (nsec) {
    // セクタを作成
    current_sector_ = 0;
    for (int i = 0; i < nsec; i++) {
      Sector* newsector = new Sector;
      if (!newsector)
        return false;
      current_track_->sector = newsector;
      newsector->next = current_sector_;
      newsector->size = secsize;
      if (secsize) {
        newsector->image.reset(new uint8_t[secsize]);
        if (!newsector->image) {
          newsector->size = 0;
          return false;
        }
      } else {
        newsector->image.reset();
      }
      current_sector_ = newsector;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------
//  セクタ一つ追加
//
FloppyDisk::Sector* FloppyDisk::AddSector(int size) {
  if (!current_track_)
    return 0;

  Sector* newsector = new Sector;
  if (!newsector)
    return 0;
  if (size) {
    newsector->image.reset(new uint8_t[size]);
    if (!newsector->image) {
      delete newsector;
      return nullptr;
    }
  } else {
    newsector->image.reset();
  }

  if (!current_sector_)
    current_sector_ = current_track_->sector;

  if (current_sector_) {
    newsector->next = current_sector_->next;
    current_sector_->next = newsector;
  } else {
    newsector->next = 0;
    current_track_->sector = newsector;
  }
  current_sector_ = newsector;
  return newsector;
}

// ---------------------------------------------------------------------------
//  トラックの容量を得る
//
uint32_t FloppyDisk::GetTrackCapacity() {
  static const int table[3] = {6250, 6250, 10416};
  return table[disk_type_];
}

// ---------------------------------------------------------------------------
//  トラックを得る
//
FloppyDisk::Sector* FloppyDisk::GetFirstSector(uint32_t tr) {
  if (tr < 168)
    return tracks_[tr].sector;
  return 0;
}
