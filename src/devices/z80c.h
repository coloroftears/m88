// ---------------------------------------------------------------------------
//  Z80 emulator in C++
//  Copyright (C) cisc 1997, 1999.
// ----------------------------------------------------------------------------
//  $Id: Z80c.h,v 1.26 2001/02/21 11:57:16 cisc Exp $

#pragma once

#include <stdio.h>
#include <string.h>

#include "common/device.h"
#include "common/memory_manager.h"
#include "common/types.h"
#include "devices/z80.h"
#include "devices/z80diag.h"

class IOBus;

#define Z80C_STATISTICS

enum class Z80Int {
  kReset = 0,
  kIRQ,
  kNMI,
};

class Z80C final : public Device {
 public:
  // TODO: extract
  struct Statistics {
    uint32_t execute[0x10000];
    void Clear() { memset(execute, 0, sizeof(execute)); }
  };

 public:
  explicit Z80C(const ID& id);
  ~Z80C();

  // Overrides Device.
  // TODO: extract as Z80Device?
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final;
  bool IFCALL SaveStatus(uint8_t* status) final;
  bool IFCALL LoadStatus(const uint8_t* status) final;

  // Initializes Z80 Emulator
  // Returns true if initialized successfully.
  bool Init(MemoryManager* mem, IOBus* bus, int iack);

  // Executes CPU for specified |count| clocks.
  // Returns clocks actually executed.
  int Exec(int count);

  // TODO: move to private?
  // Executes one instruction.
  int ExecOne();

  // Changes remaining execution clocks.
  void Stop(int count);

  // Returns total executed clock counts.
  int GetCount() const { return execcount + (clockcount << eshift); }

  // Resets CPU.
  void IOCALL Reset(uint32_t = 0, uint32_t = 0);

  // Requests interrupt.
  void IOCALL IRQ(uint32_t, uint32_t d) { intr = d; }

  // Requests NMI.
  void IOCALL NMI(uint32_t = 0, uint32_t = 0);

  // Halts/resumes Z80 CPU execution.
  void Wait(bool flag);

  void SetPC(uint32_t newpc);
  uint32_t GetPC() const { return static_cast<uint32_t>(inst_ - instbase_); }

  const Z80Reg& GetReg() const { return reg; }

  bool GetPages(MemoryPage** rd, MemoryPage** wr) {
    *rd = rdpages, *wr = wrpages;
    return true;
  }
  // TODO: Investigate what this is. (memory wait table?)
  int* GetWaits() { return 0; }

  void TestIntr();
  bool IsIntr() { return !!intr; }

  // TODO: extract this outside this class.
  bool EnableDump(bool dump);
  int GetDumpState() { return !!dumplog_fp_; }

  Statistics* GetStatistics() {
#ifdef Z80C_STATISTICS
    return &statistics;
#else
    return nullptr;
#endif
  }

  int start_count() const { return start_count_; }
  void set_start_count(int count) { start_count_ = count; }

  int delay_count() const { return delay_count_; }
  void set_delay_count(int count) { delay_count_ = count; }

  friend class Z80Util;

 private:
  enum {
    pagebits = MemoryManagerBase::pagebits,
    pagemask = MemoryManagerBase::pagemask,
  };

  enum {
    ssrev = 1,
  };
  struct Status {
    Z80Reg reg;
    uint8_t intr;
    uint8_t wait;
    uint8_t xf;
    uint8_t rev;
    int execcount;
  };

  void DumpLog();

  // Fast path caches pointer to memory that PC register points.
  // Always make sure |inst_| >= |instpage_|, and can be |inst_| > |instlim_|.
  // For the latter case, Fetch8() handles the rest adjustments.
  uint8_t* inst_ = nullptr;
  // Upper limit of |inst_| cache page.
  uint8_t* instlim_ = nullptr;
  // For easier calculation of PC ()= |inst_| - |instbase_|).
  uint8_t* instbase_ = nullptr;
  // Base address of |inst_| cache page.
  uint8_t* instpage_ = nullptr;

  Z80Reg reg;

  IOBus* bus;

  static const Descriptor descriptor;
  static const OutFuncPtr outdef[];

  int execcount;
  int clockcount;
  int stopcount;
  int intack;
  int intr;

  enum {
    kWaitNone = 0,
    kWaitHalt = 1,
    kWaitCPU = 2
  };
  int wait_state_ = kWaitNone;

  int eshift;

  int start_count_ = 0;
  int delay_count_ = 0;

  // HL/IX/IY
  enum index { USEHL, USEIX, USEIY } index_mode;
  // Uncalculated flag.
  uint8_t uf;
  // Remembers last addition/subtraction.
  uint8_t nfa;
  // Undefined flags (3rd and 5th bits)
  uint8_t xf;
  // Data for flag calculation.
  uint32_t fx32, fy32;
  uint32_t fx, fy;

  // Table for H/XH/YH
  uint8_t* ref_h[3];
  // Table for L/YH/YL
  uint8_t* ref_l[3];
  // Table for HL/IX/IY
  Z80Reg::wordreg* ref_hl[3];
  // Table for BCDEHL A
  uint8_t* ref_byte[8];

  FILE* dumplog_fp_;
  Z80Diag diag;

  MemoryPage rdpages[0x10000 >> MemoryManager::pagebits];
  MemoryPage wrpages[0x10000 >> MemoryManager::pagebits];

#ifdef Z80C_STATISTICS
  Statistics statistics;
#endif

 private:
  void CLK(int count) { clockcount += count; }
  void SetXYForBlockInstruction(uint32_t n);

  // Memory and I/O interfaces.

  // Data read
  uint32_t Read8(uint32_t addr);
  uint32_t Read16(uint32_t a);
  // Opcode/operand read (fast + slow path)
  uint32_t Fetch8();
  uint32_t Fetch16();
  // Data write
  void Write8(uint32_t addr, uint32_t data);
  void Write16(uint32_t a, uint32_t d);
  // I/O
  uint32_t Inp(uint32_t port);
  void Outp(uint32_t port, uint32_t data);

  // Execution related interfaces.
  void SingleStep() { SingleStep(Fetch8()); }
  void SingleStep(uint32_t inst);
  void Init();
  int Exec0(int stop, int d);
  int Exec1(int stop, int d);
  bool Sync();
  void OutTestIntr();

  void SetPCi(uint32_t newpc);
  void PCInc(uint32_t inc);
  void PCDec(uint32_t dec);

  // Instruction executors.
  void Call();
  void Jump(uint32_t dest);
  void JumpR();

  uint8_t GetCF();
  uint8_t GetZF();
  uint8_t GetSF();
  uint8_t GetHF();
  uint8_t GetPF();

  void SetM(uint32_t n);
  uint8_t GetM();

  void Push(uint32_t n);
  uint32_t Pop();

  // 8bit ALU
  void ADDA(uint8_t);
  void ADCA(uint8_t);
  void SUBA(uint8_t);
  void SBCA(uint8_t);
  void ANDA(uint8_t);
  void ORA(uint8_t);
  void XORA(uint8_t);
  void CPA(uint8_t);

  uint8_t Inc8(uint8_t);
  uint8_t Dec8(uint8_t);

  // 16bit ALU
  uint32_t ADD16(uint32_t x, uint32_t y);
  void ADCHL(uint32_t y);
  void SBCHL(uint32_t y);

  uint32_t GetAF();
  void SetAF(uint32_t n);
  void SetZS(uint8_t a);
  void SetZSP(uint8_t a);

  void CPI();
  void CPD();

  void CodeCB();

  // Shift/Rotate
  uint8_t RLC(uint8_t);
  uint8_t RRC(uint8_t);
  uint8_t RL(uint8_t);
  uint8_t RR(uint8_t);
  uint8_t SLA(uint8_t);
  uint8_t SRA(uint8_t);
  uint8_t SLL(uint8_t);
  uint8_t SRL(uint8_t);
};

class Z80Util {
 public:
  Z80Util() {}

  static int ExecSingle(Z80C* first, Z80C* second, int count);
  static int ExecDual(Z80C* first, Z80C* second, int count);
  static int ExecDual2(Z80C* first, Z80C* second, int count);

  static void StopDual(int count) {
    if (current_cpu_)
      current_cpu_->Stop(count);
  }

  static int GetCCount() {
    return current_cpu_ ?
        current_cpu_->GetCount() - current_cpu_->start_count() : 0;
  }

  static void SetCurrentCPU(Z80C* cpu) { current_cpu_ = cpu; }

 private:
  static Z80C* current_cpu_;
};
