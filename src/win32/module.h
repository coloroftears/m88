// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: module.h,v 1.2 1999/10/10 15:59:54 cisc Exp $

#pragma once

#include "interface/ifcommon.h"

namespace PC8801 {

//
class ExtendModule {
 public:
  ExtendModule();
  ~ExtendModule();

  static ExtendModule* Create(const char* dllname, ISystem* pc);

  bool Connect(const char* dllname, ISystem* pc);
  bool Disconnect();

  IDevice::ID GetID();
  void* QueryIF(REFIID iid);

 private:
  using F_CONNECT2 = IModule*(__cdecl*)(ISystem*);

  HMODULE hdll;
  IModule* mod;
};

inline void* ExtendModule::QueryIF(REFIID iid) {
  return mod ? mod->QueryIF(iid) : 0;
}
}  // namespace PC8801
