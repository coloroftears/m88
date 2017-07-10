// ----------------------------------------------------------------------------
//  M88 - PC-8801 series emulator
//  Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//  Main 側メモリ(含ALU)の実装
// ----------------------------------------------------------------------------
//  $Id: memory.h,v 1.26 2003/09/28 14:58:54 cisc Exp $

#pragma once

#include <memory>

#include "common/device.h"
#include "pc88/config.h"

class MemoryManager;

namespace pc88core {
class CRTC;

class Memory final : public Device, public IGetMemoryBank {
 public:
  enum IDOut {
    kReset,
    kOut31,
    kOut32,
    kOut34,
    kOut35,
    kOut40,
    kOut5x,
    kOut70,
    kOut71,
    kOut78,
    kOut99,
    kOute2,
    kOute3,
    kOutf0,
    kOutf1,
    kVRTC,
    kOut33
  };

  enum IDIn { kIn32, kIn5c, kIn70, kIn71, kIne2, kIne3, kIn33 };

  union quadbyte {
    uint32_t pack;
    uint8_t byte[4];
  };

  enum ROM { n88 = 0, n88e = 0x8000, n80 = 0x10000, romsize = 0x18000 };

  enum MemID {
    mRAM,
    mTV,
    mN88,
    mN,
    mN80,
    mN80SR,
    mN80SR1,
    mN88E0,
    mN88E1,
    mN88E2,
    mN88E3,
    mG0,
    mG1,
    mG2,
    mALU,
    mCD0,
    mCD1,
    mJISYO,
    mE1,
    mE2,
    mE3,
    mE4,
    mE5,
    mE6,
    mE7,
    mERAM,
  };

 public:
  explicit Memory(const ID& id);
  ~Memory();

  void ApplyConfig(const Config* cfg);
  uint8_t* GetRAM() { return ram.get(); }
  uint8_t* GetTVRAM() { return tvram.get(); }
  quadbyte* GetGVRAM() { return gvram; }
  uint8_t* GetROM() { return rom.get(); }
  uint8_t* GetDirtyFlag() { return dirty; }

  // Overrides IGetMemoryBank.
  uint32_t IFCALL GetRdBank(uint32_t addr) final;
  uint32_t IFCALL GetWrBank(uint32_t addr) final;

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final;
  bool IFCALL LoadStatus(const uint8_t* status) final;
  bool IFCALL SaveStatus(uint8_t* status) final;

  bool IsCDBIOSReady() { return !!cdbios; }

  bool Init(MemoryManager* mgr, IOBus* bus, CRTC* crtc, int* waittbl);
  void IOCALL Reset(uint32_t, uint32_t);
  void IOCALL Out31(uint32_t, uint32_t data);
  void IOCALL Out32(uint32_t, uint32_t data);
  void IOCALL Out33(uint32_t, uint32_t data);
  void IOCALL Out34(uint32_t, uint32_t data);
  void IOCALL Out35(uint32_t, uint32_t data);
  void IOCALL Out40(uint32_t, uint32_t data);
  void IOCALL Out5x(uint32_t port, uint32_t);
  void IOCALL Out70(uint32_t, uint32_t data);
  void IOCALL Out71(uint32_t, uint32_t data);
  void IOCALL Out78(uint32_t, uint32_t data);
  void IOCALL Out99(uint32_t, uint32_t data);
  void IOCALL Oute2(uint32_t, uint32_t data);
  void IOCALL Oute3(uint32_t, uint32_t data);
  void IOCALL Outf0(uint32_t, uint32_t data);
  void IOCALL Outf1(uint32_t, uint32_t data);
  uint32_t IOCALL In32(uint32_t);
  uint32_t IOCALL In33(uint32_t);
  uint32_t IOCALL In5c(uint32_t);
  uint32_t IOCALL In70(uint32_t);
  uint32_t IOCALL In71(uint32_t);
  uint32_t IOCALL Ine2(uint32_t);
  uint32_t IOCALL Ine3(uint32_t);
  void IOCALL VRTC(uint32_t, uint32_t data);

 private:
  struct WaitDesc {
    uint32_t b0, bc, bf;
  };

  enum {
    ssrev = 2,  // Status を更新時に増やすこと
  };
  struct Status {
    uint8_t rev;
    uint8_t p31, p32, p33, p34, p35, p40, p5x;
    uint8_t p70, p71, p99, pe2, pe3, pf0;
    quadbyte alureg;

    uint8_t ram[0x10000];
    uint8_t tvram[0x1000];
    uint8_t gvram[3][0x4000];
    uint8_t eram[1];
  };

  bool InitMemory();
  bool LoadROM();
  bool LoadROMImage(uint8_t* at, const char* file, int length);
  bool LoadOptROM(const char* file,
                  std::unique_ptr<uint8_t[]>& rom,
                  int length);
  void SetWait();
  void SetWaits(uint32_t, uint32_t, uint32_t);
  void SelectJisyo();

  void Update00R();
  void Update60R();
  void Update00W();
  void Update80();
  void UpdateC0();
  void UpdateF0();
  void UpdateN80W();
  void UpdateN80R();
  void UpdateN80G();
  void SelectGVRAM(uint32_t top);
  void SelectALU(uint32_t top);
  void SetRAMPattern(uint8_t* ram, uint32_t length);

  uint32_t GetHiBank(uint32_t addr);

  MemoryManager* mm = nullptr;
  int mid = -1;
  int* waits = nullptr;
  IOBus* bus = nullptr;
  CRTC* crtc = nullptr;

  std::unique_ptr<uint8_t[]> rom;
  std::unique_ptr<uint8_t[]> ram;
  std::unique_ptr<uint8_t[]> eram;
  std::unique_ptr<uint8_t[]> tvram;
  std::unique_ptr<uint8_t[]> dicrom;       // 辞書ROM
  std::unique_ptr<uint8_t[]> cdbios;       // CD-ROM BIOS ROM
  std::unique_ptr<uint8_t[]> n80rom;       // N80-BASIC ROM
  std::unique_ptr<uint8_t[]> n80v2rom;     // N80SR
  std::unique_ptr<uint8_t[]> erom[8 + 1];  // 拡張 ROM

  uint32_t port31;
  uint32_t port32;
  uint32_t port33;
  uint32_t port34;
  uint32_t port35;
  uint32_t port40;
  uint32_t port5x;
  uint32_t port99;
  uint32_t txtwnd;
  uint32_t port71;
  uint32_t porte2;
  uint32_t porte3;
  uint32_t portf0;
  uint32_t sw31;
  uint32_t erommask;
  uint32_t waitmode;
  uint32_t waittype;  // b0 = disp/vrtc,

  bool selgvram;
  bool seldic;
  bool enablewait;
  bool n80mode;
  bool n80srmode;

  uint32_t erambanks;
  uint32_t neweram;
  uint8_t* r00;
  uint8_t* r60;
  uint8_t* w00;
  uint8_t* rc0;
  quadbyte alureg;
  quadbyte maskr, maski, masks, aluread;

  quadbyte gvram[0x4000];
  uint8_t dirty[0x400];

  static const WaitDesc waittable[48];

  static void MEMCALL WrWindow(void* inst, uint32_t addr, uint32_t data);
  static uint32_t MEMCALL RdWindow(void* inst, uint32_t addr);

  static void MEMCALL WrGVRAM0(void* inst, uint32_t addr, uint32_t data);
  static void MEMCALL WrGVRAM1(void* inst, uint32_t addr, uint32_t data);
  static void MEMCALL WrGVRAM2(void* inst, uint32_t addr, uint32_t data);
  static uint32_t MEMCALL RdGVRAM0(void* inst, uint32_t addr);
  static uint32_t MEMCALL RdGVRAM1(void* inst, uint32_t addr);
  static uint32_t MEMCALL RdGVRAM2(void* inst, uint32_t addr);

  static void MEMCALL WrALUSet(void* inst, uint32_t addr, uint32_t data);
  static void MEMCALL WrALURGB(void* inst, uint32_t addr, uint32_t data);
  static void MEMCALL WrALUR(void* inst, uint32_t addr, uint32_t data);
  static void MEMCALL WrALUB(void* inst, uint32_t addr, uint32_t data);
  static uint32_t MEMCALL RdALU(void* inst, uint32_t addr);

  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};
}  // namespace pc88core
