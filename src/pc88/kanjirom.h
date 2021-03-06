// ---------------------------------------------------------------------------
//  M88 - PC-88 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  漢字 ROM
// ---------------------------------------------------------------------------
//  $Id: kanjirom.h,v 1.5 1999/10/10 01:47:07 cisc Exp $

#pragma once

#include "common/device.h"

#include <memory>

namespace pc88core {

class KanjiROM final : public Device {
 public:
  enum { kSetL, kSetH };

  enum { kReadL, kReadH };

 public:
  explicit KanjiROM(const ID& id);
  ~KanjiROM();

  bool Init(const char* filename);

  void IOCALL SetL(uint32_t p, uint32_t d);
  void IOCALL SetH(uint32_t p, uint32_t d);
  uint32_t IOCALL ReadL(uint32_t p);
  uint32_t IOCALL ReadH(uint32_t p);

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final { return sizeof(uint32_t); }
  bool IFCALL SaveStatus(uint8_t* status) final {
    *(uint32_t*)status = address_;
    return true;
  }
  bool IFCALL LoadStatus(const uint8_t* status) final {
    address_ = *(const uint32_t*)status;
    return true;
  }

 private:
  uint32_t address_;
  std::unique_ptr<uint8_t[]> image_;

  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};
}  // namespace pc88core
