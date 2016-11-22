// ---------------------------------------------------------------------------
//  PC-8801 emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: subsys.h,v 1.7 2000/09/08 15:04:14 cisc Exp $

#ifndef pc88_subsys_h
#define pc88_subsys_h

#include "common/device.h"
#include "pc88/fdc.h"
#include "pc88/pio.h"

class MemoryManager;

namespace PC8801 {

class SubSystem : public Device {
 public:
  enum {
    reset = 0,
    m_set0,
    m_set1,
    m_set2,
    m_setcw,
    s_set0,
    s_set1,
    s_set2,
    s_setcw,
  };
  enum { intack = 0, m_read0, m_read1, m_read2, s_read0, s_read1, s_read2 };

 public:
  SubSystem(const ID& id);
  ~SubSystem();

  bool Init(MemoryManager* mmgr);
  const Descriptor* IFCALL GetDesc() const { return &descriptor; }
  uint IFCALL GetStatusSize();
  bool IFCALL SaveStatus(uint8_t* status);
  bool IFCALL LoadStatus(const uint8_t* status);

  uint8_t* GetRAM() { return ram; }
  uint8_t* GetROM() { return rom; }

  bool IsBusy();

  void IOCALL Reset(uint = 0, uint = 0);
  uint IOCALL IntAck(uint);

  void IOCALL M_Set0(uint, uint data);
  void IOCALL M_Set1(uint, uint data);
  void IOCALL M_Set2(uint, uint data);
  void IOCALL M_SetCW(uint, uint data);
  uint IOCALL M_Read0(uint);
  uint IOCALL M_Read1(uint);
  uint IOCALL M_Read2(uint);

  void IOCALL S_Set0(uint, uint data);
  void IOCALL S_Set1(uint, uint data);
  void IOCALL S_Set2(uint, uint data);
  void IOCALL S_SetCW(uint, uint data);
  uint IOCALL S_Read0(uint);
  uint IOCALL S_Read1(uint);
  uint IOCALL S_Read2(uint);

 private:
  enum {
    ssrev = 1,
  };
  struct Status {
    uint rev;
    uint8_t ps[3], cs;
    uint8_t pm[3], cm;
    uint idlecount;
    uint8_t ram[0x4000];
  };

  bool InitMemory();
  bool LoadROM();
  void PatchROM();

  MemoryManager* mm;
  int mid;
  uint8_t* rom;
  uint8_t* ram;
  uint8_t* dummy;
  PIO piom, pios;
  uint cw_m, cw_s;
  uint idlecount;

 private:
  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};

}  // namespace PC8801

#endif  // pc88_subsys_h
