// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: status.h,v 1.6 2002/04/07 05:40:10 cisc Exp $

#pragma once

#include "common/critical_section.h"
#include "common/types.h"

#include <memory>

class StatusImpl;

class StatusDisplay final {
 public:
  StatusDisplay();
  ~StatusDisplay();

  bool Show(int priority, int duration, const char* msg, ...);
  void FDAccess(uint32_t dr, bool hd, bool active);
  void UpdateDisplay();
  void WaitSubSys();

  StatusImpl* impl() { return impl_.get(); }

 private:
  friend class StatusImpl;
  std::unique_ptr<StatusImpl> impl_;
};

extern StatusDisplay statusdisplay;
