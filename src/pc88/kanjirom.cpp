// ---------------------------------------------------------------------------
//  M88 - PC-88 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  漢字 ROM
// ---------------------------------------------------------------------------
//  $Id: kanjirom.cpp,v 1.6 2000/02/29 12:29:52 cisc Exp $

#include "pc88/kanjirom.h"

#include <string.h>

#include "common/file.h"

namespace pc88core {

// ---------------------------------------------------------------------------
//  構築/消滅
// ---------------------------------------------------------------------------

KanjiROM::KanjiROM(const ID& id) : Device(id), address_(0) {}

KanjiROM::~KanjiROM() {}

// ---------------------------------------------------------------------------
//  初期化
//
bool KanjiROM::Init(const char* filename) {
  if (!image_)
    image_.reset(new uint8_t[0x20000]);
  memset(image_.get(), 0xff, 0x20000);

  FileIO file(filename, FileIO::readonly);
  if (file.GetFlags() & FileIO::open) {
    file.Seek(0, FileIO::begin);
    file.Read(image_.get(), 0x20000);
  }
  return true;
}

// ---------------------------------------------------------------------------
//  I/O
//
void IOCALL KanjiROM::SetL(uint32_t, uint32_t d) {
  address_ = (address_ & ~0xff) | d;
}

void IOCALL KanjiROM::SetH(uint32_t, uint32_t d) {
  address_ = (address_ & 0xff) | (d * 0x100);
}

uint32_t IOCALL KanjiROM::ReadL(uint32_t) {
  return image_[address_ * 2 + 1];
}

uint32_t IOCALL KanjiROM::ReadH(uint32_t) {
  return image_[address_ * 2];
}

// ---------------------------------------------------------------------------
//  device description
//
const Device::Descriptor KanjiROM::descriptor = {KanjiROM::indef,
                                                 KanjiROM::outdef};

const Device::OutFuncPtr KanjiROM::outdef[] = {
    static_cast<Device::OutFuncPtr>(&KanjiROM::SetL),
    static_cast<Device::OutFuncPtr>(&KanjiROM::SetH),
};

const Device::InFuncPtr KanjiROM::indef[] = {
    static_cast<Device::InFuncPtr>(&KanjiROM::ReadL),
    static_cast<Device::InFuncPtr>(&KanjiROM::ReadH),
};
}  // namespace pc88core
