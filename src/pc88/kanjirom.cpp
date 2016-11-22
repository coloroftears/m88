// ---------------------------------------------------------------------------
//  M88 - PC-88 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  漢字 ROM
// ---------------------------------------------------------------------------
//  $Id: kanjirom.cpp,v 1.6 2000/02/29 12:29:52 cisc Exp $

#include "win32/headers.h"
#include "common/file.h"
#include "pc88/kanjirom.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//  構築/消滅
// ---------------------------------------------------------------------------

KanjiROM::KanjiROM(const ID& id) : Device(id) {
  image = 0;
  adr = 0;
}

KanjiROM::~KanjiROM() {
  delete[] image;
}

// ---------------------------------------------------------------------------
//  初期化
//
bool KanjiROM::Init(const char* filename) {
  if (!image)
    image = new uint8_t[0x20000];
  if (!image)
    return false;
  memset(image, 0xff, 0x20000);

  FileIO file(filename, FileIO::readonly);
  if (file.GetFlags() & FileIO::open) {
    file.Seek(0, FileIO::begin);
    file.Read(image, 0x20000);
  }
  return true;
}

// ---------------------------------------------------------------------------
//  I/O
//
void IOCALL KanjiROM::SetL(uint32_t, uint32_t d) {
  adr = (adr & ~0xff) | d;
}

void IOCALL KanjiROM::SetH(uint32_t, uint32_t d) {
  adr = (adr & 0xff) | (d * 0x100);
}

uint32_t IOCALL KanjiROM::ReadL(uint32_t) {
  return image[adr * 2 + 1];
}

uint32_t IOCALL KanjiROM::ReadH(uint32_t) {
  return image[adr * 2];
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
