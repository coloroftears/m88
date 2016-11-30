// ---------------------------------------------------------------------------
//  Z80 emulator for x86/VC5.
//  Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//  $Id: Z80_x86.h,v 1.15 2000/11/02 12:43:44 cisc Exp $

#pragma once

#include "common/device.h"
#include "common/memory_manager.h"
#include "devices/z80.h"

#ifndef PTR_IDBIT
#error[Z80_x86] PTR_IDBIT definition is required
#endif

// ---------------------------------------------------------------------------

class Z80_x86 final : public Device {
 public:
  enum {
    reset = 0,
    irq,
    nmi,
  };

 public:
  explicit Z80_x86(const ID& id);
  ~Z80_x86();

  bool Init(MemoryManager* mm, IOBus* bus, int iack);

  int Exec(int clocks);
  int ExecOne();
  static int __stdcall ExecSingle(Z80_x86* first, Z80_x86* second, int clocks);
  static int __stdcall ExecDual(Z80_x86* first, Z80_x86* second, int clocks);
  static int __stdcall ExecDual2(Z80_x86* first, Z80_x86* second, int clocks);
  void Stop(int clocks);
  static void StopDual(int clocks);
  static int GetCCount();
  int GetCount();

  void IOCALL Reset(uint32_t = 0, uint32_t = 0);
  void IOCALL IRQ(uint32_t, uint32_t d);
  void IOCALL NMI(uint32_t = 0, uint32_t = 0);
  void Wait(bool);

  uint32_t GetPC() { return (uint32_t)inst + (uint32_t)instbase; }

  bool GetPages(MemoryPage** rd, MemoryPage** wr) {
    *rd = rdpages, *wr = wrpages;
    return true;
  }
  int* GetWaits() { return waittable; }

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final { return sizeof(CPUState); }
  bool IFCALL LoadStatus(const uint8_t* status) final;
  bool IFCALL SaveStatus(uint8_t* status) final;

  bool EnableDump(bool) { return false; }
  int GetDumpState() { return -1; }

 public:
  // Debug Service Functions
  void TestIntr();
  void SetPC(uint32_t n) {
    inst = (uint8_t*)n;
    instbase = 0;
    instpage = (uint8_t*)-1;
    instlim = 0;
  }
  const Z80Reg& GetReg() { return reg; }
  bool IsIntr() { return !!intr; }

 private:
  enum {
    pagebits = MemoryManager::pagebits,
    idbit = MemoryManager::idbit,
  };
  enum {
    ssrev = 1,
  };
  struct CPUState {
    Z80Reg reg;
    uint8_t intr;
    uint8_t waitstate;
    uint8_t flagn;
    uint8_t rev;
    uint32_t execcount;
  };

 private:
  Z80Reg reg;
  uint8_t* inst;
  uint8_t* instbase;
  uint8_t* instlim;
  uint8_t* instpage;
  uint32_t instwait;
  const IOBus::InBank* ins;
  const IOBus::OutBank* outs;
  const uint8_t* ioflags;

  uint8_t intr;
  uint8_t waitstate;
  uint8_t flagn;
  uint8_t eshift;

  uint32_t execcount;
  int stopcount;
  int delaycount;
  int clockcount;
  int intack;
  int startcount;

  MemoryPage rdpages[0x10000 >> MemoryManager::pagebits];
  MemoryPage wrpages[0x10000 >> MemoryManager::pagebits];
  int waittable[0x10000 >> MemoryManager::pagebits];

  static const Descriptor descriptor;
  static const OutFuncPtr outdef[];
  static Z80_x86* currentcpu;
};

// ---------------------------------------------------------------------------

inline int Z80_x86::GetCount() {
  return execcount + (clockcount << eshift);
}
