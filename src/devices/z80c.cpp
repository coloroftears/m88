// ---------------------------------------------------------------------------
//  Z80 emulator in C++
//  Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//  $Id: Z80c.cpp,v 1.37 2003/04/22 13:11:19 cisc Exp $

#include "devices/z80c.h"

//#define LOGNAME "Z80C"
#include "common/diag.h"

// Register accessor macros
#define RegA (reg.r.b.a)
#define RegB (reg.r.b.b)
#define RegC (reg.r.b.c)
#define RegD (reg.r.b.d)
#define RegE (reg.r.b.e)
#define RegH (reg.r.b.h)
#define RegL (reg.r.b.l)
#define RegXH (*ref_h[index_mode])
#define RegXL (*ref_l[index_mode])
#define RegF (reg.r.b.flags)

#define RegXHL (*ref_hl[index_mode])
#define RegHL (reg.r.w.hl)
#define RegDE (reg.r.w.de)
#define RegBC (reg.r.w.bc)
#define RegAF (reg.r.w.af)
#define RegSP (reg.r.w.sp)

#if !defined(NDEBUG) && defined(LOGNAME)
enum {
  kFetch8 = 0,
  kFetch8B = 1,
  kFetch8BSpecial = 2,
  kFetch16 = 3,
  kSetPCMemory = 4,
  kSetPCSpecial = 5,
  kGetPC = 6,
  kPCDecIn = 7,
  kPCDecOut = 19,
  kJumpIn = 9,
  kJumpOut = 10,
  kReinitPage = 11,
  kRead8Direct = 12,
  kRead8Special = 8,
  kRead16Direct = 13,
  kWrite8Direct = 14,
  kWrite8Special = 16,
  kWrite16 = 15,
  kJumpRelativeIn = 17,
  kJumpRelativeOut = 18
};
static int testcount[24];
#define DEBUGCOUNT(i) testcount[i]++
#else
#define DEBUGCOUNT(i) 0
#endif

Z80C::Z80C(const ID& id) : Device(id) {
  /* テーブル初期化 */
  ref_h[USEHL] = &RegH;
  ref_l[USEHL] = &RegL;
  ref_h[USEIX] = &reg.r.b.xh;
  ref_l[USEIX] = &reg.r.b.xl;
  ref_h[USEIY] = &reg.r.b.yh;
  ref_l[USEIY] = &reg.r.b.yl;
  ref_hl[USEHL] = &RegHL;
  ref_hl[USEIX] = &reg.r.w.ix;
  ref_hl[USEIY] = &reg.r.w.iy;
  ref_byte[0] = &RegB;
  ref_byte[1] = &RegC;
  ref_byte[2] = &RegD;
  ref_byte[3] = &RegE;
  ref_byte[4] = &RegH;
  ref_byte[5] = &RegL;
  ref_byte[6] = nullptr;
  ref_byte[7] = &RegA;

  dumplog_fp_ = nullptr;
}

Z80C::~Z80C() {
#if defined(LOGNAME) && defined(_DEBUG)
  Log("Fetch8            = %10d\n", testcount[0]);
  Log("Fetch8B           = %10d\n", testcount[1]);
  Log("Fetch8B(special)  = %10d\n", testcount[2]);
  Log("Fetch16           = %10d\n", testcount[3]);
  Log("SetPC(memory)     = %10d\n", testcount[4]);
  Log("SetPC(special)    = %10d\n", testcount[5]);
  Log("GetPC             = %10d\n", testcount[6]);
  Log("PCDec(in)         = %10d\n", testcount[7]);
  Log("PCDec(out)        = %10d\n", testcount[19]);
  Log("Jump(in)          = %10d\n", testcount[9]);
  Log("Jump(out)         = %10d\n", testcount[10]);
  Log("ReinitPage(Out)   = %10d\n", testcount[11]);
  Log("Read8(direct)     = %10d\n", testcount[12]);
  Log("Read8(special)    = %10d\n", testcount[8]);
  Log("Read16(direct)    = %10d\n", testcount[13]);
  Log("Write8(direct)    = %10d\n", testcount[14]);
  Log("Write8(special)   = %10d\n", testcount[16]);
  Log("Write16           = %10d\n", testcount[15]);
  Log("JumpRelative(in)  = %10d\n", testcount[17]);
  Log("JumpRelative(out) = %10d\n", testcount[18]);
  Log("\n");
#endif
  if (dumplog_fp_)
    fclose(dumplog_fp_);
}

void Z80C::SetPC(uint32_t newpc) {
  const uint32_t pages_mask = (1 << (16 - pagebits)) - 1;
  MemoryPage& page = rdpages[(newpc >> pagebits) & pages_mask];

  if (!page.func) {
    DEBUGCOUNT(kSetPCMemory);
    // instruction is on memory
    instpage_ = reinterpret_cast<uint8_t*>(page.ptr);
    instbase_ =
        reinterpret_cast<uint8_t*>(page.ptr) - (newpc & ~pagemask & 0xffff);
    instlim_ = reinterpret_cast<uint8_t*>(page.ptr) + (1 << pagebits);
    inst_ = instpage_ + (newpc & pagemask);
  } else {
    DEBUGCOUNT(kSetPCSpecial);
    instbase_ = nullptr;
    instlim_ = nullptr;
    instpage_ = reinterpret_cast<uint8_t*>(~0);
    inst_ = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(newpc));
  }
}

#if 0
// TODO: Use this instead of inlined version in the header.
inline uint32_t Z80C::GetPC()
{
  DEBUGCOUNT(kGetPC);
  return static_cast<uint32_t>(inst_ - instbase_);
}
#endif

inline void Z80C::PCInc(uint32_t inc) {
  inst_ += inc;
}

inline void Z80C::PCDec(uint32_t dec) {
  inst_ -= dec;
  if (inst_ >= instpage_) {
    DEBUGCOUNT(kPCDecIn);
    return;
  }
  DEBUGCOUNT(kPCDecOut);
  SetPC(GetPC());
  return;
}

inline void Z80C::Jump(uint32_t dest) {
  inst_ = instbase_ + dest;
  if (inst_ >= instpage_) {
    DEBUGCOUNT(kJumpIn);
    return;
  }
  DEBUGCOUNT(kJumpOut);
  SetPC(dest);
  return;
}

inline void Z80C::JumpR() {
  inst_ += int8_t(Fetch8());
  CLK(5);
  if (inst_ >= instpage_) {
    DEBUGCOUNT(kJumpRelativeIn);
    return;
  }
  DEBUGCOUNT(kJumpRelativeOut);
  SetPC(GetPC());
  return;
}

inline uint32_t Z80C::Fetch8() {
  // Fast path.
  DEBUGCOUNT(kFetch8);
  if (inst_ < instlim_)
    return *inst_++;

  DEBUGCOUNT(kFetch8B);
  if (instlim_) {
    SetPC(GetPC());
    if (instlim_)
      return *inst_++;
  }
  DEBUGCOUNT(kFetch8BSpecial);
  return Read8(inst_++ - instbase_);
}

inline uint32_t Z80C::Fetch16() {
  DEBUGCOUNT(kFetch16);
  uint32_t r = Fetch8();
  return r | (Fetch8() << 8);
}

void Z80C::Wait(bool wait) {
  if (wait)
    wait_state_ |= kWaitCPU;
  else
    wait_state_ &= ~kWaitCPU;
}

bool Z80C::Init(MemoryManager* mem, IOBus* _bus, int iack) {
  bus = _bus, intack = iack;

  index_mode = USEHL;
  clockcount = 0;
  execcount = 0;
  eshift = 0;

  diag.Init(mem);
  Reset();
  return true;
}

int Z80C::ExecOne() {
  execcount += clockcount;
  clockcount = 0;
  SingleStep();
  GetAF();
  return clockcount;
}

int Z80C::Exec(int clocks) {
  SingleStep();
  TestIntr();
  Z80Util::SetCurrentCPU(this);
  int cbase = GetCount();
  execcount += clockcount + clocks;
  stopcount = execcount;
  delay_count_ = clocks;

  for (clockcount = -clocks; clockcount < 0;) {
    SingleStep();
    SingleStep();
    SingleStep();
    SingleStep();
  }
  return GetCount() - cbase;
}

// Execute main CPU
int Z80C::Exec0(int stop, int other) {
  int clocks = stop - GetCount();
  if (clocks > 0) {
    eshift = 0;
    Z80Util::SetCurrentCPU(this);
    stopcount = stop;
    delay_count_ = other;
    execcount += clockcount + clocks;

    if (dumplog_fp_) {
      for (clockcount = -clocks; clockcount < 0;) {
        DumpLog();
        SingleStep();
      }
    } else {
      for (clockcount = -clocks; clockcount < 0;)
        SingleStep();
    }
    Z80Util::SetCurrentCPU(nullptr);
    return stopcount;
  } else {
    return stop;
  }
}

// Execute sub CPU
int Z80C::Exec1(int stop, int other) {
  int clocks = stop - GetCount();
  if (clocks > 0) {
    eshift = 1;
    Z80Util::SetCurrentCPU(this);
    stopcount = stop;
    delay_count_ = other;
    execcount += clockcount * 2 + clocks;
    if (dumplog_fp_) {
      for (clockcount = -clocks / 2; clockcount < 0;) {
        DumpLog();
        SingleStep();
      }
    } else {
      for (clockcount = -clocks / 2; clockcount < 0;) {
        SingleStep();
      }
    }
    Z80Util::SetCurrentCPU(nullptr);
    return stopcount;
  } else {
    return stop;
  }
}

// Check
bool Z80C::Sync() {
  // Is this CPU has delay from other one?
  if (GetCount() - delay_count_ <= 1)
    return true;

  // If advanced, exit EXEC0
  execcount += clockcount << eshift;
  clockcount = 0;
  return false;
}

// Abort Exec().
void Z80C::Stop(int count) {
  execcount = stopcount = GetCount() + count;
  clockcount = -count >> eshift;
}

inline uint32_t Z80C::Read8(uint32_t addr) {
  addr &= 0xffff;
  MemoryPage& page = rdpages[addr >> pagebits];
  if (!page.func) {
    DEBUGCOUNT(kRead8Direct);
    return ((uint8_t*)page.ptr)[addr & pagemask];
  } else {
    DEBUGCOUNT(kRead8Special);
    return (*MemoryManager::RdFunc(intptr_t(page.ptr)))(page.inst, addr);
  }
}

inline void Z80C::Write8(uint32_t addr, uint32_t data) {
  addr &= 0xffff;
  MemoryPage& page = wrpages[addr >> pagebits];
  if (!page.func) {
    DEBUGCOUNT(kWrite8Direct);
    ((uint8_t*)page.ptr)[addr & pagemask] = data;
  } else {
    DEBUGCOUNT(kWrite8Special);
    (*MemoryManager::WrFunc(intptr_t(page.ptr)))(page.inst, addr, data);
  }
}

inline uint32_t Z80C::Read16(uint32_t addr) {
  return Read8(addr) + Read8(addr + 1) * 256;
}

inline void Z80C::Write16(uint32_t addr, uint32_t data) {
  DEBUGCOUNT(kWrite16);
  Write8(addr, data & 0xff);
  Write8(addr + 1, data >> 8);
}

inline uint32_t Z80C::Inp(uint32_t port) {
  return bus->In(port & 0xff);
}

inline void Z80C::Outp(uint32_t port, uint32_t data) {
  bus->Out(port & 0xff, data);
  SetPC(GetPC());
  DEBUGCOUNT(kReinitPage);
}

// Flags
#define CF (uint8_t(1 << 0))
#define NF (uint8_t(1 << 1))
#define PF (uint8_t(1 << 2))
#define HF (uint8_t(1 << 4))
#define ZF (uint8_t(1 << 6))
#define SF (uint8_t(1 << 7))

#define WF (uint8_t(1 << 3))
#define YF (uint8_t(1 << 5))

// Macros
#define RegA (reg.r.b.a)
#define RegB (reg.r.b.b)
#define RegC (reg.r.b.c)
#define RegD (reg.r.b.d)
#define RegE (reg.r.b.e)
#define RegH (reg.r.b.h)
#define RegL (reg.r.b.l)
#define RegXH (*ref_h[index_mode])
#define RegXL (*ref_l[index_mode])
#define RegF (reg.r.b.flags)

#define RegXHL (*ref_hl[index_mode])
#define RegHL (reg.r.w.hl)
#define RegDE (reg.r.w.de)
#define RegBC (reg.r.w.bc)
#define RegAF (reg.r.w.af)
#define RegSP (reg.r.w.sp)

#define GetNF() (RegF & NF)
#define SetXF(n) (xf = n)

#define SetFlags(m, f) (void)((RegF = (RegF & ~(m)) | (f)), (uf &= ~(m)))

#define Ret() Jump(Pop())

#define RES(n, bit) ((n) & ~(1 << (bit)))
#define SET(n, bit) ((n) | (1 << (bit)))
#define BIT(n, bit)                                                   \
  (void)SetFlags(                                                     \
      ZF | HF | NF | SF | PF,                                         \
      HF | (((n) & (1 << (bit))) ? n & SF & (1 << (bit)) : ZF | PF)), \
      SetXF(n)

void IOCALL Z80C::Reset(uint32_t, uint32_t) {
  memset(&reg, 0, sizeof(reg));

  reg.iff1 = false;
  reg.iff2 = false;
  reg.ireg = 0; /* I, R = 0 */
  reg.rreg = 0;

  RegF = 0;
  uf = 0;
  instlim_ = nullptr;
  instbase_ = nullptr;

  //  SetFlags(0xff, 0); // Reset flags
  reg.intmode = 0; /* IM0 */
  SetPC(0);        /* pc, sp = 0 */
  RegSP = 0;
  wait_state_ = kWaitNone;
  intr = false;  // Clear interrupts
  execcount = 0;
}

// Non-maskable interrupt
void IOCALL Z80C::NMI(uint32_t, uint32_t) {
  reg.iff2 = reg.iff1;
  reg.iff1 = false;
  Push(GetPC());
  CLK(11);
  SetPC(0x66);
}

// Interrupt?
void Z80C::TestIntr() {
  if (reg.iff1 && intr) {
    reg.iff1 = false;
    reg.iff2 = false;

    if (wait_state_ & kWaitHalt) {
      wait_state_ = kWaitNone;
      PCInc(1);
    }
    int intno = bus->In(intack);

    switch (reg.intmode) {
      case 0:
        SingleStep(intno);
        CLK(13);
        break;

      case 1:
        Push(GetPC());
        SetPC(0x38);
        CLK(13);
        break;

      case 2:
        Push(GetPC());
        SetPC(Read16(reg.ireg * 256 + intno));
        CLK(19);
        break;
    }
  }
}

inline void Z80C::Call() {
  uint32_t d = Fetch16();
  Push(GetPC());
  Jump(d);
  CLK(7);
}

// Auxlliary accessors.
void Z80C::SetM(uint32_t n) {
  if (index_mode == USEHL)
    Write8(RegHL, n);
  else {
    Write8(RegXHL + int8_t(Fetch8()), n);
    CLK(12);
  }
}

uint8_t Z80C::GetM() {
  if (index_mode == USEHL)
    return Read8(RegHL);
  else {
    int r = Read8(RegXHL + int8_t(Fetch8()));
    CLK(12);
    return r;
  }
}

uint32_t Z80C::GetAF() {
  RegF = (RegF & 0xd7) | (xf & 0x28);
  if (uf & (CF | ZF | SF | HF | PF))
    GetCF(), GetZF(), GetSF(), GetHF(), GetPF();
  return RegAF;
}

inline void Z80C::SetAF(uint32_t n) {
  RegAF = n;
  uf = 0;
  SetXF(n);
}

inline void Z80C::Push(uint32_t n) {
  RegSP -= 2;
  Write16(RegSP, n);
}

inline uint32_t Z80C::Pop() {
  uint32_t a = Read16(RegSP);
  RegSP += 2;
  return a;
}

void Z80C::ADDA(uint8_t n) {
  fx = uint32_t(RegA) * 2;
  fy = uint32_t(n) * 2;
  uf = SF | ZF | HF | PF | CF;
  nfa = 0;
  RegF &= ~NF;
  RegA += n;
  SetXF(RegA);
}

void Z80C::ADCA(uint8_t n) {
  uint8_t a = RegA;
  uint8_t cy = GetCF();

  RegA = a + n + cy;
  SetXF(RegA);

  fx = uint32_t(a) * 2 + 1;
  fy = uint32_t(n) * 2 + cy;
  uf = SF | ZF | HF | PF | CF;
  nfa = 0;
  RegF &= ~NF;
}

void Z80C::SUBA(uint8_t n) {
  fx = RegA * 2;
  fy = n * 2;
  uf = SF | ZF | HF | PF | CF;
  nfa = 1;
  RegF |= NF;
  RegA -= n;
  SetXF(RegA);
}

void Z80C::CPA(uint8_t n) {
  fx = RegA * 2;
  fy = n * 2;
  SetXF(n);
  uf = SF | ZF | HF | PF | CF;
  nfa = 1;
  RegF |= NF;
}

void Z80C::SBCA(uint8_t n) {
  uint8_t a = RegA;
  uint8_t cy = GetCF();
  RegA = (a - n - cy);

  fx = a * 2;
  fy = n * 2 + cy;
  uf = SF | ZF | HF | PF | CF;
  nfa = 1;
  RegF |= NF;
  SetXF(RegA);
}

void Z80C::ANDA(uint8_t n) {
  uint8_t b = RegA & n;
  SetZSP(b);
  SetFlags(HF | NF | CF, HF);
  RegA = (b);
  SetXF(RegA);
}

void Z80C::ORA(uint8_t n) {
  uint8_t b = RegA | n;
  SetZSP(b);
  SetFlags(HF | NF | CF, 0);
  RegA = (b);
  SetXF(RegA);
}

void Z80C::XORA(uint8_t n) {
  uint8_t b = RegA ^ n;
  SetZSP(b);
  SetFlags(HF | NF | CF, 0);
  RegA = (b);
  SetXF(RegA);
}

uint8_t Z80C::Inc8(uint8_t y) {
  y++;
  SetFlags(SF | ZF | HF | PF | NF, ((y == 0) ? ZF : 0) | ((y & 0x80) ? SF : 0) |
                                       ((y == 0x80) ? PF : 0) |
                                       ((y & 0x0f) ? 0 : HF));
  SetXF(y);
  return y;
}

uint8_t Z80C::Dec8(uint8_t y) {
  y--;
  SetFlags(SF | ZF | HF | PF | NF, ((y == 0) ? ZF : 0) | ((y & 0x80) ? SF : 0) |
                                       ((y == 0x7f) ? PF : 0) |
                                       (((y & 0x0f) == 0xf) ? HF : 0) | NF);
  SetXF(y);
  return y;
}

uint32_t Z80C::ADD16(uint32_t x, uint32_t y) {
  GetSF(), GetZF(), GetPF();
  fx32 = (x & 0xffff) * 2;
  fy32 = (y & 0xffff) * 2;
  uf = CF | HF | WF;
  SetFlags(NF, 0);
  nfa = 0;
  SetXF(((x + y) >> 8));
  return x + y;
}

void Z80C::ADCHL(uint32_t y) {
  uint32_t cy = GetCF();
  uint32_t x = RegHL;

  fx32 = (uint32_t)(x & 0xffff) * 2 + 1;
  fy32 = (uint32_t)(y & 0xffff) * 2 + cy;
  RegHL = x + y + cy;
  uf = SF | ZF | HF | PF | CF | WF;
  nfa = 0;
  RegF &= ~NF;
  SetXF(RegH);
}

void Z80C::SBCHL(uint32_t y) {
  uint32_t cy = GetCF();

  fx32 = (uint32_t)(RegHL & 0xffff) * 2;
  fy32 = (uint32_t)(y & 0xffff) * 2 + cy;
  RegHL = RegHL - y - cy;
  uf = SF | ZF | HF | PF | CF | WF;
  nfa = 1;
  RegF |= NF;
  SetXF(RegH);
}

uint8_t Z80C::RLC(uint8_t d) {
  uint8_t f = (d & 0x80) ? CF : 0;
  d = (d << 1) + f; /* CF == 1 */

  SetZSP(d);
  SetFlags(CF | NF | HF, f);
  return d;
}

uint8_t Z80C::RRC(uint8_t d) {
  uint8_t f = d & 1;
  d = (d >> 1) + (f ? 0x80 : 0);

  SetZSP(d);
  SetFlags(CF | NF | HF, f);
  return d;
}

uint8_t Z80C::RL(uint8_t d) {
  uint8_t f = (d & 0x80) ? CF : 0;
  d = (d << 1) + GetCF();

  SetZSP(d);
  SetFlags(CF | NF | HF, f);
  return d;
}

uint8_t Z80C::RR(uint8_t d) {
  uint8_t f = d & 1;
  d = (d >> 1) + (GetCF() ? 0x80 : 0);

  SetZSP(d);
  SetFlags(CF | NF | HF, f);
  return d;
}

uint8_t Z80C::SLA(uint8_t d) {
  SetFlags(NF | HF | CF, (d & 0x80) ? CF : 0);
  d <<= 1;
  SetZSP(d);
  return d;
}

uint8_t Z80C::SRA(uint8_t d) {
  SetFlags(NF | HF | CF, d & 1);
  d = int8_t(d) >> 1;
  SetZSP(d);
  return d;
}

uint8_t Z80C::SLL(uint8_t d) {
  SetFlags(NF | HF | CF, (d & 0x80) ? CF : 0);
  d = (d << 1) + 1;
  SetZSP(d);
  return d;
}

uint8_t Z80C::SRL(uint8_t d) {
  SetFlags(NF | HF | CF, d & 1);
  d >>= 1;
  SetZSP(d);
  return d;
}

// Flag table
static const uint8_t ZSPTable[256] = {
    ZF | PF, 0,       0,       PF,      0,       PF,      PF,      0,
    0,       PF,      PF,      0,       PF,      0,       0,       PF,
    0,       PF,      PF,      0,       PF,      0,       0,       PF,
    PF,      0,       0,       PF,      0,       PF,      PF,      0,
    0,       PF,      PF,      0,       PF,      0,       0,       PF,
    PF,      0,       0,       PF,      0,       PF,      PF,      0,
    PF,      0,       0,       PF,      0,       PF,      PF,      0,
    0,       PF,      PF,      0,       PF,      0,       0,       PF,
    0,       PF,      PF,      0,       PF,      0,       0,       PF,
    PF,      0,       0,       PF,      0,       PF,      PF,      0,
    PF,      0,       0,       PF,      0,       PF,      PF,      0,
    0,       PF,      PF,      0,       PF,      0,       0,       PF,
    PF,      0,       0,       PF,      0,       PF,      PF,      0,
    0,       PF,      PF,      0,       PF,      0,       0,       PF,
    0,       PF,      PF,      0,       PF,      0,       0,       PF,
    PF,      0,       0,       PF,      0,       PF,      PF,      0,
    SF,      PF | SF, PF | SF, SF,      PF | SF, SF,      SF,      PF | SF,
    PF | SF, SF,      SF,      PF | SF, SF,      PF | SF, PF | SF, SF,
    PF | SF, SF,      SF,      PF | SF, SF,      PF | SF, PF | SF, SF,
    SF,      PF | SF, PF | SF, SF,      PF | SF, SF,      SF,      PF | SF,
    PF | SF, SF,      SF,      PF | SF, SF,      PF | SF, PF | SF, SF,
    SF,      PF | SF, PF | SF, SF,      PF | SF, SF,      SF,      PF | SF,
    SF,      PF | SF, PF | SF, SF,      PF | SF, SF,      SF,      PF | SF,
    PF | SF, SF,      SF,      PF | SF, SF,      PF | SF, PF | SF, SF,
    PF | SF, SF,      SF,      PF | SF, SF,      PF | SF, PF | SF, SF,
    SF,      PF | SF, PF | SF, SF,      PF | SF, SF,      SF,      PF | SF,
    SF,      PF | SF, PF | SF, SF,      PF | SF, SF,      SF,      PF | SF,
    PF | SF, SF,      SF,      PF | SF, SF,      PF | SF, PF | SF, SF,
    SF,      PF | SF, PF | SF, SF,      PF | SF, SF,      SF,      PF | SF,
    PF | SF, SF,      SF,      PF | SF, SF,      PF | SF, PF | SF, SF,
    PF | SF, SF,      SF,      PF | SF, SF,      PF | SF, PF | SF, SF,
    SF,      PF | SF, PF | SF, SF,      PF | SF, SF,      SF,      PF | SF,
};

void Z80C::SingleStep(uint32_t m) {
  // M1 state - increment R register.
  reg.rreg++;

  switch (m) {
    uint8_t b;
    uint32_t w;

    // Rotate/Shift

    case 0x07:  // RLCA
      b = (0 != (RegA & 0x80));
      RegA = RegA * 2 + b;
      SetFlags(NF | HF | CF, b); /* Cn = 1 */
      CLK(4);
      SetXF(RegA);
      break;

    case 0x0f:  // RRCA
      b = RegA & 1;
      RegA = (RegA >> 1) + (b ? 0x80 : 0);
      SetFlags(NF | HF | CF, b);
      CLK(4);
      SetXF(RegA);
      break;

    case 0x17:  // RLA
      b = RegA;
      RegA = (b << 1) + GetCF();
      SetFlags(NF | HF | CF, (b & 0x80) ? CF : 0);
      CLK(4);
      SetXF(RegA);
      break;

    case 0x1f:  // RRA
      b = RegA;
      RegA = (GetCF() ? 0x80 : 0) + (b >> 1);
      SetFlags(NF | HF | CF, b & 1);
      CLK(4);
      SetXF(RegA);
      break;

    // arthimatic operation

    case 0x27: {  // DAA
      b = 0;
      uint8_t new_a = RegA;
      if (!GetNF()) {  // addition
        if ((RegA & 0x0f) > 9 || GetHF()) {
          if ((RegA & 0x0f) > 9)
            b = HF;
          new_a += 6;
        }
        if (RegA > 0x99 || GetCF()) {
          new_a += 0x60;
          b |= CF;
        }
      } else {  // subtraction
        if ((RegA & 0x0f) > 9 || GetHF()) {
          if ((RegA & 0x0f) < 6)
            b = HF;
          new_a -= 6;
        }
        if (RegA > 0x99 || GetCF()) {
          new_a -= 0x60;
          b |= CF;
        }
      }
      RegA = new_a;
      SetZSP(RegA);
      SetXF(RegA);
      SetFlags(HF | CF, b);
      CLK(4);
    } break;

    case 0x2f:  // CPL
      RegA = ~RegA;
      SetFlags(NF | HF, NF | HF);
      SetXF(RegA);
      CLK(4);
      break;

    case 0x37:  // SCF
      SetFlags(CF | NF | HF, CF);
      SetXF(RegA);
      CLK(4);
      break;

    case 0x3f:  // CCF
      b = GetCF();
      SetFlags(CF | NF | HF, b ^ CF | ((b & CF) << 4));
      SetXF(RegA);
      CLK(4);
      break;

    //  I/O access

    case 0xdb:  // IN A,(n)
      w = /*(uint32_t(RegA) << 8) +*/ Fetch8();
      if (bus->IsSyncPort(w) && !Sync()) {
        PCDec(2);
        break;
      }
      RegA = Inp(w);
      CLK(11);
      break;

    case 0xd3:  // OUT (n),A
      w = /*(uint32_t(RegA) << 8) + */ Fetch8();
      if (bus->IsSyncPort(w) && !Sync()) {
        PCDec(2);
        break;
      }
      Outp(w, RegA);
      CLK(11);
      OutTestIntr();
      break;

    //  branch op.

    case 0xc3:  // JP
      Jump(Fetch16());
      CLK(10);
      break;

    case 0xc2: /*NZ*/
      if (!GetZF())
        Jump(Fetch16());
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xca: /* Z*/
      if (GetZF())
        Jump(Fetch16());
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xd2: /*NC*/
      if (!GetCF())
        Jump(Fetch16());
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xda: /* C*/
      if (GetCF())
        Jump(Fetch16());
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xe2: /*PO*/
      if (!GetPF())
        Jump(Fetch16());
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xea: /*PE*/
      if (GetPF())
        Jump(Fetch16());
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xf2: /* P*/
      if (!GetSF())
        Jump(Fetch16());
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xfa: /* M*/
      if (GetSF())
        Jump(Fetch16());
      else
        PCInc(2);
      CLK(10);
      break;

    case 0xcd:  // CALL
      Call();
      CLK(10);
      break;

    case 0xc4: /*NZ*/
      if (!GetZF())
        Call();
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xcc: /* Z*/
      if (GetZF())
        Call();
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xd4: /*NC*/
      if (!GetCF())
        Call();
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xdc: /* C*/
      if (GetCF())
        Call();
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xe4: /*PO*/
      if (!GetPF())
        Call();
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xec: /*PE*/
      if (GetPF())
        Call();
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xf4: /* P*/
      if (!GetSF())
        Call();
      else
        PCInc(2);
      CLK(10);
      break;
    case 0xfc: /* M*/
      if (GetSF())
        Call();
      else
        PCInc(2);
      CLK(10);
      break;

    case 0xc9:  // RET
      Ret();
      CLK(4);
      break;

    case 0xc0: /*NZ*/
      if (!GetZF())
        Ret();
      CLK(4);
      break;
    case 0xc8: /* Z*/
      if (GetZF())
        Ret();
      CLK(4);
      break;
    case 0xd0: /*NC*/
      if (!GetCF())
        Ret();
      CLK(4);
      break;
    case 0xd8: /* C*/
      if (GetCF())
        Ret();
      CLK(4);
      break;
    case 0xe0: /*PO*/
      if (!GetPF())
        Ret();
      CLK(4);
      break;
    case 0xe8: /*PE*/
      if (GetPF())
        Ret();
      CLK(4);
      break;
    case 0xf0: /* P*/
      if (!GetSF())
        Ret();
      CLK(4);
      break;
    case 0xf8: /* M*/
      if (GetSF())
        Ret();
      CLK(4);
      break;

    case 0x18:  // JR
      JumpR();
      CLK(7);
      break;

    case 0x20: /*NZ*/
      if (!GetZF())
        JumpR();
      else
        PCInc(1);
      CLK(7);
      break;
    case 0x28: /* Z*/
      if (GetZF())
        JumpR();
      else
        PCInc(1);
      CLK(7);
      break;
    case 0x30: /*NC*/
      if (!GetCF())
        JumpR();
      else
        PCInc(1);
      CLK(7);
      break;
    case 0x38: /* C*/
      if (GetCF())
        JumpR();
      else
        PCInc(1);
      CLK(7);
      break;

    case 0xe9:  // JP (HL)
      SetPC(RegXHL);
      CLK(4);
      break;

    case 0x10:  // DJNZ
      if (0 != --RegB)
        JumpR();
      else
        PCInc(1);
      CLK(5);
      break;

    case 0xc7: /* RST 00H */
      Push(GetPC());
      Jump(0x00);
      CLK(4);
      break;
    case 0xcf: /* RST 08H */
      Push(GetPC());
      Jump(0x08);
      CLK(4);
      break;
    case 0xd7: /* RST 10H */
      Push(GetPC());
      Jump(0x10);
      CLK(4);
      break;
    case 0xdf: /* RST 18H */
      Push(GetPC());
      Jump(0x18);
      CLK(4);
      break;
    case 0xe7: /* RST 20H */
      Push(GetPC());
      Jump(0x20);
      CLK(4);
      break;
    case 0xef: /* RST 28H */
      Push(GetPC());
      Jump(0x28);
      CLK(4);
      break;
    case 0xf7: /* RST 30H */
      Push(GetPC());
      Jump(0x30);
      CLK(4);
      break;
    case 0xff: /* RST 38H */
      Push(GetPC());
      Jump(0x38);
      CLK(4);
      break;

    // 16 bit arithmatic operations

    // ADD XHL,dd
    case 0x09: /*BC*/
      RegXHL = ADD16(RegXHL, RegBC);
      CLK(11);
      break;
    case 0x19: /*DE*/
      RegXHL = ADD16(RegXHL, RegDE);
      CLK(11);
      break;
    case 0x29: /*xHL*/
      w = RegXHL;
      RegXHL = ADD16(w, w);
      CLK(11);
      break;
    case 0x39: /*SP*/
      RegXHL = ADD16(RegXHL, RegSP);
      CLK(11);
      break;

    // INC dd
    case 0x03: /*BC*/
      RegBC++;
      CLK(6);
      break;
    case 0x13: /*DE*/
      RegDE++;
      CLK(6);
      break;
    case 0x23: /*xHL*/
      RegXHL++;
      CLK(6);
      break;
    case 0x33: /*SP*/
      RegSP++;
      CLK(6);
      break;

    // DEC dd
    case 0x0B: /*BC*/
      RegBC--;
      CLK(6);
      break;
    case 0x1B: /*DE*/
      RegDE--;
      CLK(6);
      break;
    case 0x2B: /*xHL*/
      RegXHL--;
      CLK(6);
      break;
    case 0x3B: /*SP*/
      RegSP--;
      CLK(6);
      break;

    // exchange

    case 0x08:  // EX AF,AF'
      w = GetAF();
      SetAF(reg.r_af);
      reg.r_af = w;
      CLK(4);
      break;

    case 0xe3:  // EX (SP),xHL
      w = Read16(RegSP);
      Write16(RegSP, RegXHL);
      RegXHL = w;
      CLK(19);
      break;

    case 0xeb:  // EX DE,HL
      w = RegDE;
      RegDE = RegHL;
      RegHL = w;
      CLK(4);
      break;

    case 0xd9:  // EXX
      w = RegHL;
      RegHL = reg.r_hl;
      reg.r_hl = w;
      w = RegDE;
      RegDE = reg.r_de;
      reg.r_de = w;
      w = RegBC;
      RegBC = reg.r_bc;
      reg.r_bc = w;
      CLK(4);
      break;

    // CPU control

    case 0xf3:  // DI
      reg.iff1 = reg.iff2 = false;
      CLK(4);
      break;

    case 0xfb:  // EI
      w = Fetch8();
      CLK(4);
      if ((w & 0xf7) != 0xf3) {
        SingleStep(w);
        reg.iff1 = reg.iff2 = true;
        TestIntr();
      } else
        PCDec(1);
      break;

    case 0x00:  // NOP
      CLK(4);
      break;

    case 0x76:  // HALT
      PCDec(1);
      wait_state_ = kWaitHalt;
      if (intr) {
        TestIntr();
        CLK(64);
      } else
        clockcount = 0;
      break;

    // 8 bit arithmatic

    // ADD A,-
    case 0x80: /*B*/
      ADDA(RegB);
      CLK(4);
      break;
    case 0x81: /*C*/
      ADDA(RegC);
      CLK(4);
      break;
    case 0x82: /*D*/
      ADDA(RegD);
      CLK(4);
      break;
    case 0x83: /*E*/
      ADDA(RegE);
      CLK(4);
      break;
    case 0x84: /*H*/
      ADDA(RegXH);
      CLK(4);
      break;
    case 0x85: /*L*/
      ADDA(RegXL);
      CLK(4);
      break;
    case 0x86: /*M*/
      ADDA(GetM());
      CLK(7);
      break;
    case 0x87: /*A*/
      ADDA(RegA);
      CLK(4);
      break;
    case 0xc6: /*n*/
      ADDA(Fetch8());
      CLK(7);
      break;

    // ADC A,-
    case 0x88: /*B*/
      ADCA(RegB);
      CLK(4);
      break;
    case 0x89: /*C*/
      ADCA(RegC);
      CLK(4);
      break;
    case 0x8a: /*D*/
      ADCA(RegD);
      CLK(4);
      break;
    case 0x8b: /*E*/
      ADCA(RegE);
      CLK(4);
      break;
    case 0x8c: /*H*/
      ADCA(RegXH);
      CLK(4);
      break;
    case 0x8d: /*L*/
      ADCA(RegXL);
      CLK(4);
      break;
    case 0x8e: /*M*/
      ADCA(GetM());
      CLK(7);
      break;
    case 0x8f: /*A*/
      ADCA(RegA);
      CLK(4);
      break;
    case 0xce: /*n*/
      ADCA(Fetch8());
      CLK(7);
      break;

    // SUB -
    case 0x90: /*B*/
      SUBA(RegB);
      CLK(4);
      break;
    case 0x91: /*C*/
      SUBA(RegC);
      CLK(4);
      break;
    case 0x92: /*D*/
      SUBA(RegD);
      CLK(4);
      break;
    case 0x93: /*E*/
      SUBA(RegE);
      CLK(4);
      break;
    case 0x94: /*H*/
      SUBA(RegXH);
      CLK(4);
      break;
    case 0x95: /*L*/
      SUBA(RegXL);
      CLK(4);
      break;
    case 0x96: /*M*/
      SUBA(GetM());
      CLK(7);
      break;
    case 0x97: /*A*/
      SUBA(RegA);
      CLK(4);
      break;
    case 0xd6: /*n*/
      SUBA(Fetch8());
      CLK(7);
      break;

    // SBC A,-
    case 0x98: /*B*/
      SBCA(RegB);
      CLK(4);
      break;
    case 0x99: /*C*/
      SBCA(RegC);
      CLK(4);
      break;
    case 0x9a: /*D*/
      SBCA(RegD);
      CLK(4);
      break;
    case 0x9b: /*E*/
      SBCA(RegE);
      CLK(4);
      break;
    case 0x9c: /*H*/
      SBCA(RegXH);
      CLK(4);
      break;
    case 0x9d: /*L*/
      SBCA(RegXL);
      CLK(4);
      break;
    case 0x9e: /*M*/
      SBCA(GetM());
      CLK(7);
      break;
    case 0x9f: /*A*/
      SBCA(RegA);
      CLK(4);
      break;
    case 0xde: /*n*/
      SBCA(Fetch8());
      CLK(7);
      break;

    // AND -
    case 0xa0: /*B*/
      ANDA(RegB);
      CLK(4);
      break;
    case 0xa1: /*C*/
      ANDA(RegC);
      CLK(4);
      break;
    case 0xa2: /*D*/
      ANDA(RegD);
      CLK(4);
      break;
    case 0xa3: /*E*/
      ANDA(RegE);
      CLK(4);
      break;
    case 0xa4: /*H*/
      ANDA(RegXH);
      CLK(4);
      break;
    case 0xa5: /*L*/
      ANDA(RegXL);
      CLK(4);
      break;
    case 0xa6: /*M*/
      ANDA(GetM());
      CLK(7);
      break;
    case 0xa7: /*A*/
      ANDA(RegA);
      CLK(4);
      break;
    case 0xe6: /*n*/
      ANDA(Fetch8());
      CLK(7);
      break;

    // XOR -
    case 0xa8: /*B*/
      XORA(RegB);
      CLK(4);
      break;
    case 0xa9: /*C*/
      XORA(RegC);
      CLK(4);
      break;
    case 0xaa: /*D*/
      XORA(RegD);
      CLK(4);
      break;
    case 0xab: /*E*/
      XORA(RegE);
      CLK(4);
      break;
    case 0xac: /*H*/
      XORA(RegXH);
      CLK(4);
      break;
    case 0xad: /*L*/
      XORA(RegXL);
      CLK(4);
      break;
    case 0xae: /*M*/
      XORA(GetM());
      CLK(7);
      break;
    case 0xaf: /*A*/
      XORA(RegA);
      CLK(4);
      break;
    case 0xee: /*n*/
      XORA(Fetch8());
      CLK(7);
      break;

    // OR -
    case 0xb0: /*B*/
      ORA(RegB);
      CLK(4);
      break;
    case 0xb1: /*C*/
      ORA(RegC);
      CLK(4);
      break;
    case 0xb2: /*D*/
      ORA(RegD);
      CLK(4);
      break;
    case 0xb3: /*E*/
      ORA(RegE);
      CLK(4);
      break;
    case 0xb4: /*H*/
      ORA(RegXH);
      CLK(4);
      break;
    case 0xb5: /*L*/
      ORA(RegXL);
      CLK(4);
      break;
    case 0xb6: /*M*/
      ORA(GetM());
      CLK(7);
      break;
    case 0xb7: /*A*/
      ORA(RegA);
      CLK(4);
      break;
    case 0xf6: /*n*/
      ORA(Fetch8());
      CLK(7);
      break;

    // CP -
    case 0xb8: /*B*/
      CPA(RegB);
      CLK(4);
      break;
    case 0xb9: /*C*/
      CPA(RegC);
      CLK(4);
      break;
    case 0xba: /*D*/
      CPA(RegD);
      CLK(4);
      break;
    case 0xbb: /*E*/
      CPA(RegE);
      CLK(4);
      break;
    case 0xbc: /*H*/
      CPA(RegXH);
      CLK(4);
      break;
    case 0xbd: /*L*/
      CPA(RegXL);
      CLK(4);
      break;
    case 0xbe: /*M*/
      CPA(GetM());
      CLK(7);
      break;
    case 0xbf: /*A*/
      CPA(RegA);
      CLK(4);
      break;
    case 0xfe: /*n*/
      CPA(Fetch8());
      CLK(7);
      break;

    // INC r
    case 0x04: /*B*/
      RegB = (Inc8(RegB));
      CLK(4);
      break;
    case 0x0c: /*C*/
      RegC = (Inc8(RegC));
      CLK(4);
      break;
    case 0x14: /*D*/
      RegD = (Inc8(RegD));
      CLK(4);
      break;
    case 0x1c: /*E*/
      RegE = (Inc8(RegE));
      CLK(4);
      break;
    case 0x24: /*H*/
      RegXH = (Inc8(RegXH));
      CLK(4);
      break;
    case 0x2c: /*L*/
      RegXL = (Inc8(RegXL));
      CLK(4);
      break;
    case 0x3c: /*A*/
      RegA = (Inc8(RegA));
      CLK(4);
      break;

    case 0x34: /*M*/
      w = RegXHL;
      if (index_mode != USEHL) {
        w += int8_t(Fetch8());
        CLK(23 - 11);
      }
      Write8(w, Inc8(Read8(w)));
      CLK(11);
      break;

    // DEC r
    case 0x05: /*B*/
      RegB = Dec8(RegB);
      CLK(4);
      break;
    case 0x0d: /*C*/
      RegC = Dec8(RegC);
      CLK(4);
      break;
    case 0x15: /*D*/
      RegD = Dec8(RegD);
      CLK(4);
      break;
    case 0x1d: /*E*/
      RegE = Dec8(RegE);
      CLK(4);
      break;
    case 0x25: /*H*/
      RegXH = Dec8(RegXH);
      CLK(4);
      break;
    case 0x2d: /*L*/
      RegXL = Dec8(RegXL);
      CLK(4);
      break;
    case 0x3d: /*A*/
      RegA = Dec8(RegA);
      CLK(4);
      break;

    case 0x35: /*M*/
      w = RegXHL;
      if (index_mode != USEHL) {
        w += (int8_t)(Fetch8());
        CLK(23 - 11);
      }
      Write8(w, Dec8(Read8(w)));
      CLK(11);
      break;

    // stack op.

    // PUSH
    case 0xc5: /*BC*/
      Push(RegBC);
      CLK(11);
      break;
    case 0xd5: /*DE*/
      Push(RegDE);
      CLK(11);
      break;
    case 0xe5: /*xHL*/
      Push(RegXHL);
      CLK(11);
      break;
    case 0xf5: /*AF*/
      Push(GetAF());
      CLK(11);
      break;

    // POP
    case 0xc1: /*BC*/
      RegBC = Pop();
      CLK(10);
      break;
    case 0xd1: /*DE*/
      RegDE = Pop();
      CLK(10);
      break;
    case 0xe1: /*xHL*/
      RegXHL = Pop();
      CLK(10);
      break;
    case 0xf1: /*AF*/
      SetAF(Pop());
      CLK(10);
      break;

    // 16 bit load

    // LD dd,nn
    case 0x01: /*BC*/
      RegBC = Fetch16();
      CLK(10);
      break;
    case 0x11: /*DE*/
      RegDE = Fetch16();
      CLK(10);
      break;
    case 0x21: /*xHL*/
      RegXHL = Fetch16();
      CLK(10);
      break;
    case 0x31: /*SP*/
      RegSP = Fetch16();
      CLK(10);
      break;

    case 0x22:  // LD (nn),xHL
      Write16(Fetch16(), RegXHL);
      CLK(22);
      break;

    case 0x2a:  // LD xHL,(nn)
      RegXHL = Read16(Fetch16());
      CLK(22);
      break;

    case 0xf9:  // LD SP,HL
      RegSP = RegXHL;
      CLK(6);
      break;

    // 8 bit LDs

    // LD B,-
    case 0x40: /*B*/
      CLK(4);
      break;
    case 0x41: /*C*/
      RegB = RegC;
      CLK(4);
      break;
    case 0x42: /*D*/
      RegB = RegD;
      CLK(4);
      break;
    case 0x43: /*E*/
      RegB = RegE;
      CLK(4);
      break;
    case 0x44: /*H*/
      RegB = RegXH;
      CLK(4);
      break;
    case 0x45: /*L*/
      RegB = RegXL;
      CLK(4);
      break;
    case 0x46: /*M*/
      RegB = GetM();
      CLK(7);
      break;
    case 0x47: /*A*/
      RegB = RegA;
      CLK(4);
      break;
    case 0x06: /*n*/
      RegB = Fetch8();
      CLK(7);
      break;

    // LD C,-
    case 0x48: /*B*/
      RegC = RegB;
      CLK(4);
      break;
    case 0x49: /*C*/
      CLK(4);
      break;
    case 0x4a: /*D*/
      RegC = RegD;
      CLK(4);
      break;
    case 0x4b: /*E*/
      RegC = RegE;
      CLK(4);
      break;
    case 0x4c: /*H*/
      RegC = RegXH;
      CLK(4);
      break;
    case 0x4d: /*L*/
      RegC = RegXL;
      CLK(4);
      break;
    case 0x4e: /*M*/
      RegC = GetM();
      CLK(7);
      break;
    case 0x4f: /*A*/
      RegC = RegA;
      CLK(4);
      break;
    case 0x0e: /*n*/
      RegC = Fetch8();
      CLK(7);
      break;

    // LD D,-
    case 0x50: /*B*/
      RegD = RegB;
      CLK(4);
      break;
    case 0x51: /*C*/
      RegD = RegC;
      CLK(4);
      break;
    case 0x52: /*D*/
      CLK(4);
      break;
    case 0x53: /*E*/
      RegD = RegE;
      CLK(4);
      break;
    case 0x54: /*H*/
      RegD = RegXH;
      CLK(4);
      break;
    case 0x55: /*L*/
      RegD = RegXL;
      CLK(4);
      break;
    case 0x56: /*M*/
      RegD = GetM();
      CLK(7);
      break;
    case 0x57: /*A*/
      RegD = RegA;
      CLK(4);
      break;
    case 0x16: /*n*/
      RegD = Fetch8();
      CLK(7);
      break;

    // LD E,-
    case 0x58: /*B*/
      RegE = RegB;
      CLK(4);
      break;
    case 0x59: /*C*/
      RegE = RegC;
      CLK(4);
      break;
    case 0x5a: /*D*/
      RegE = RegD;
      CLK(4);
      break;
    case 0x5b: /*E*/
      CLK(4);
      break;
    case 0x5c: /*H*/
      RegE = RegXH;
      CLK(4);
      break;
    case 0x5d: /*L*/
      RegE = RegXL;
      CLK(4);
      break;
    case 0x5e: /*M*/
      RegE = GetM();
      CLK(7);
      break;
    case 0x5f: /*A*/
      RegE = RegA;
      CLK(4);
      break;
    case 0x1e: /*n*/
      RegE = Fetch8();
      CLK(7);
      break;

    // LD H,-
    case 0x60: /*B*/
      RegXH = RegB;
      CLK(4);
      break;
    case 0x61: /*C*/
      RegXH = RegC;
      CLK(4);
      break;
    case 0x62: /*D*/
      RegXH = RegD;
      CLK(4);
      break;
    case 0x63: /*E*/
      RegXH = RegE;
      CLK(4);
      break;
    case 0x64: /*H*/
      CLK(4);
      break;
    case 0x65: /*L*/
      RegXH = RegXL;
      CLK(4);
      break;
    case 0x66: /*M*/
      RegH = GetM();
      CLK(7);
      break;
    case 0x67: /*A*/
      RegXH = RegA;
      CLK(4);
      break;
    case 0x26: /*n*/
      RegXH = Fetch8();
      CLK(7);
      break;

    // LD L,-
    case 0x68: /*B*/
      RegXL = RegB;
      CLK(4);
      break;
    case 0x69: /*C*/
      RegXL = RegC;
      CLK(4);
      break;
    case 0x6a: /*D*/
      RegXL = RegD;
      CLK(4);
      break;
    case 0x6b: /*E*/
      RegXL = RegE;
      CLK(4);
      break;
    case 0x6c: /*H*/
      RegXL = RegXH;
      CLK(4);
      break;
    case 0x6d: /*L*/
      CLK(4);
      break;
    case 0x6e: /*M*/
      RegL = GetM();
      CLK(7);
      break;
    case 0x6f: /*A*/
      RegXL = RegA;
      CLK(4);
      break;
    case 0x2e: /*n*/
      RegXL = Fetch8();
      CLK(7);
      break;

    // LD M,-
    case 0x70: /*B*/
      SetM(RegB);
      CLK(7);
      break;
    case 0x71: /*C*/
      SetM(RegC);
      CLK(7);
      break;
    case 0x72: /*D*/
      SetM(RegD);
      CLK(7);
      break;
    case 0x73: /*E*/
      SetM(RegE);
      CLK(7);
      break;
    case 0x74: /*H*/
      SetM(RegH);
      CLK(7);
      break;
    case 0x75: /*L*/
      SetM(RegL);
      CLK(7);
      break;
    case 0x77: /*A*/
      SetM(RegA);
      CLK(7);
      break;
    case 0x36: /*n*/
      w = RegXHL;
      if (index_mode != USEHL) {
        w += int8_t(Fetch8());
        CLK(19 - 10);
      }
      Write8(w, Fetch8());
      CLK(11);
      break;

    // LD A,-
    case 0x78: /*B*/
      RegA = RegB;
      CLK(4);
      break;
    case 0x79: /*C*/
      RegA = RegC;
      CLK(4);
      break;
    case 0x7a: /*D*/
      RegA = RegD;
      CLK(4);
      break;
    case 0x7b: /*E*/
      RegA = RegE;
      CLK(4);
      break;
    case 0x7c: /*H*/
      RegA = RegXH;
      CLK(4);
      break;
    case 0x7d: /*L*/
      RegA = RegXL;
      CLK(4);
      break;
    case 0x7e: /*M*/
      RegA = GetM();
      CLK(7);
      break;
    case 0x7f: /*A*/
      CLK(4);
      break;
    case 0x3e: /*n*/
      RegA = Fetch8();
      CLK(7);
      break;

    // LD (--), A
    case 0x02: /*BC*/
      Write8(RegBC, RegA);
      CLK(7);
      break;
    case 0x12: /*DE*/
      Write8(RegDE, RegA);
      CLK(7);
      break;
    case 0x32: /*nn*/
      Write8(Fetch16(), RegA);
      CLK(13);
      break;

    // LD A, (--)
    case 0x0a: /*BC*/
      RegA = Read8(RegBC);
      CLK(7);
      break;
    case 0x1a: /*DE*/
      RegA = Read8(RegDE);
      CLK(7);
      break;
    case 0x3a: /*nn*/
      RegA = Read8(Fetch16());
      CLK(13);
      break;

    // DD / FD
    case 0xdd:
      w = Fetch8();
      if ((w & 0xdf) != 0xdd) {  // not DD nor FD
        index_mode = USEIX;
        SingleStep(w);
        index_mode = USEHL;
        CLK(4);
        break;
      }
      PCDec(1);
      CLK(4);
      break;

    case 0xfd:
      w = Fetch8();
      if ((w & 0xdf) != 0xdd) {
        index_mode = USEIY;
        SingleStep(w);
        index_mode = USEHL;
        CLK(4);
        break;
      }
      PCDec(1);
      CLK(4);
      break;

    // CB
    case 0xcb:
      if (index_mode == USEHL)
        reg.rreg++;
      CodeCB();
      break;

    // ED
    case 0xed:
      w = Fetch8();
      reg.rreg++;
      switch (w) {
        // IN r,(c)
        case 0x40:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          SetZSP(RegB = Inp(RegBC));
          SetFlags(NF | HF, 0);
          CLK(12);
          break;

        case 0x48:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          SetZSP(RegC = Inp(RegBC));
          SetFlags(NF | HF, 0);
          CLK(12);
          break;

        case 0x50:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          SetZSP(RegD = Inp(RegBC));
          SetFlags(NF | HF, 0);
          CLK(12);
          break;

        case 0x58:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          SetZSP(RegE = Inp(RegBC));
          SetFlags(NF | HF, 0);
          CLK(12);
          break;

        case 0x60:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          SetZSP(RegH = Inp(RegBC));
          SetFlags(NF | HF, 0);
          CLK(12);
          break;

        case 0x68:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          SetZSP(RegL = Inp(RegBC));
          SetFlags(NF | HF, 0);
          CLK(12);
          break;

        case 0x70:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          SetZSP(Inp(RegBC));
          SetFlags(NF | HF, 0);
          CLK(12);
          break;

        case 0x78:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          SetZSP(RegA = Inp(RegBC));
          SetFlags(NF | HF, 0);
          CLK(12);
          break;

        // OUT (C),r
        case 0x41:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Outp(RegBC, RegB);
          CLK(12);
          OutTestIntr();
          break;

        case 0x49:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Outp(RegBC, RegC);
          CLK(12);
          OutTestIntr();
          break;

        case 0x51:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Outp(RegBC, RegD);
          CLK(12);
          OutTestIntr();
          break;

        case 0x59:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Outp(RegBC, RegE);
          CLK(12);
          OutTestIntr();
          break;

        case 0x61:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Outp(RegBC, RegH);
          CLK(12);
          OutTestIntr();
          break;

        case 0x69:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Outp(RegBC, RegL);
          CLK(12);
          OutTestIntr();
          break;

        case 0x71:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Outp(RegBC, 0);
          CLK(12);
          OutTestIntr();
          break;

        case 0x79:
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Outp(RegBC, RegA);
          CLK(12);
          OutTestIntr();
          break;

        case 0xa2:  // INI
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Write8(RegHL++, Inp(RegBC));
          SetFlags(ZF | NF, --RegB ? NF : NF | ZF);
          CLK(16);
          break;

        case 0xaa:  // IND
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Write8(RegHL--, Inp(RegBC));
          SetFlags(ZF | NF, --RegB ? NF : NF | ZF);
          CLK(16);
          break;

        case 0xa3:  // OUTI
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Outp(RegBC, Read8(RegHL++));
          SetFlags(ZF | NF, --RegB ? NF : NF | ZF);
          CLK(16);
          OutTestIntr();
          break;

        case 0xab:  // OUTD
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Outp(RegBC, Read8(RegHL--));
          SetFlags(ZF | NF, --RegB ? NF : NF | ZF);
          CLK(16);
          OutTestIntr();
          break;

        case 0xb2:  // INIR
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Write8(RegHL++, Inp(RegBC));
          SetFlags(ZF | NF, --RegB ? NF : NF | ZF);
          CLK(16);
          if (RegB)
            PCDec(2);
          break;

        case 0xba:  // INDR
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Write8(RegHL--, Inp(RegBC));
          SetFlags(ZF | NF, --RegB ? NF : NF | ZF);
          CLK(16);
          if (RegB)
            PCDec(2);
          break;

        case 0xb3:  // OTIR
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Outp(RegBC, Read8(RegHL++));
          SetFlags(ZF | NF, --RegB ? NF : NF | ZF);
          CLK(16);
          if (RegB)
            PCDec(2);
          OutTestIntr();
          break;

        case 0xbb:  // OTDR
          if (bus->IsSyncPort(RegBC & 0xff) && !Sync()) {
            PCDec(2);
            break;
          }
          Outp(RegBC, Read8(RegHL--));
          SetFlags(ZF | NF, --RegB ? NF : NF | ZF);
          CLK(16);
          if (RegB)
            PCDec(2);
          OutTestIntr();
          break;

        // Block transfer
        case 0xa0: {  // LDI
          uint8_t n = Read8(RegHL++);
          Write8(RegDE++, n);
          SetFlags(PF | NF | HF, --RegBC & 0xffff ? PF : 0);
          SetXYForBlockInstruction(n + RegA);
          CLK(16);
        } break;

        case 0xa8: {  // LDD
          uint8_t n = Read8(RegHL--);
          Write8(RegDE--, n);
          SetFlags(PF | NF | HF, --RegBC & 0xffff ? PF : 0);
          SetXYForBlockInstruction(n + RegA);
          CLK(16);
        } break;

        case 0xb0: {  // LDIR
          uint8_t n = Read8(RegHL++);
          Write8(RegDE++, n);
          SetXYForBlockInstruction(n + RegA);
          if (--RegBC & 0xffff) {
            SetFlags(PF | NF | HF, PF);
            PCDec(2), CLK(21);
          } else {
            SetFlags(PF | NF | HF, 0);
            CLK(16);
          }
        } break;

        case 0xb8: {  // LDDR
          uint8_t n = Read8(RegHL--);
          Write8(RegDE--, n);
          SetXYForBlockInstruction(n + RegA);
          if (--RegBC & 0xffff) {
            SetFlags(PF | NF | HF, PF);
            PCDec(2), CLK(21);
          } else {
            SetFlags(PF | NF | HF, 0);
            CLK(16);
          }
        } break;

        // Block search
        case 0xa1:  // CPI
          CPI();
          break;

        case 0xa9:  // CPD
          CPD();
          break;

        case 0xb1:  // CPIR
          CPI();
          if (!GetZF() && RegBC)
            PCDec(2);
          break;

        case 0xb9:  // CPDR
          CPD();
          if (!GetZF() && RegBC)
            PCDec(2);
          break;

        // misc

        case 0x44:
        case 0x4c:
        case 0x54:
        case 0x5c:  // NEG
        case 0x64:
        case 0x6c:
        case 0x74:
        case 0x7c:  // NEG
          b = RegA;
          RegA = 0;
          SUBA(b);
          CLK(8);
          break;

        case 0x46:
        case 0x4e:
        case 0x66:
        case 0x6e:  // IM 0
          reg.intmode = 0;
          CLK(8);
          break;

        case 0x56:
        case 0x76: /* IM 1 */
          reg.intmode = 1;
          CLK(8);
          break;
        case 0x5e:
        case 0x7e: /* IM 2 */
          reg.intmode = 2;
          CLK(8);
          break;

        case 0x57:  // LD A,I
          RegA = (reg.ireg);
          SetZS(reg.ireg);
          SetFlags(NF | HF | PF, reg.iff1 ? PF : 0);
          CLK(9);
          break;

        case 0x5F:  // LD A,R
          RegA = (reg.rreg & 0x7f) + (reg.rreg7 & 0x80);
          SetZS(RegA);
          SetFlags(NF | HF | PF, (reg.iff1 ? PF : 0));
          CLK(9);
          break;

        case 0x47:  // LD I,A
          reg.ireg = RegA;
          CLK(9);
          break;

        case 0x4f:  // LD R,A
          reg.rreg7 = reg.rreg = RegA;
          CLK(9);
          break;

        case 0x45:  // RETN
        case 0x4d:  // RETI
        case 0x55:
        case 0x5d:
        case 0x65:
        case 0x6d:
        case 0x75:
        case 0x7d:
          reg.iff1 = reg.iff2;
          Ret();
          CLK(14);
          break;

        // 桁移動命令
        case 0x6f:  // RLD
        {
          uint8_t d, e;

          d = Read8(RegHL);
          e = RegA & 0x0f;
          RegA = (RegA & 0xf0) + (d >> 4);
          d = ((d << 4) & 0xf0) + e;
          Write8(RegHL, d);

          SetZSP(RegA);
          SetFlags(NF | HF, 0);
          CLK(18);
        } break;

        case 0x67:  // RRD
        {
          uint8_t d, e;

          d = Read8(RegHL);
          e = RegA & 0x0f;
          RegA = (RegA & 0xf0) + (d & 0x0f);
          d = (d >> 4) + (e << 4);
          Write8(RegHL, d);

          SetZSP(RegA);
          SetFlags(NF | HF, 0);
          CLK(18);
        } break;

        // 16bit load
        // LD (nn),dd
        case 0x43: /*BC*/
          Write16(Fetch16(), RegBC);
          CLK(20);
          break;
        case 0x53: /*DE*/
          Write16(Fetch16(), RegDE);
          CLK(20);
          break;
        case 0x63: /*HL*/
          Write16(Fetch16(), RegHL);
          CLK(20);
          break;
        case 0x73: /*SP*/
          Write16(Fetch16(), RegSP);
          CLK(20);
          break;

        // LD dd,(nn)
        case 0x4b: /*BC*/
          RegBC = Read16(Fetch16());
          CLK(20);
          break;
        case 0x5b: /*DE*/
          RegDE = Read16(Fetch16());
          CLK(20);
          break;
        case 0x6b: /*HL*/
          RegHL = Read16(Fetch16());
          CLK(20);
          break;
        case 0x7b: /*SP*/
          RegSP = Read16(Fetch16());
          CLK(20);
          break;

        // 16bit arihmetic
        // ADC HL,dd
        case 0x4a: /*BC*/
          ADCHL(RegBC);
          CLK(15);
          break;
        case 0x5a: /*DE*/
          ADCHL(RegDE);
          CLK(15);
          break;
        case 0x6a: /*HL*/
          ADCHL(RegHL);
          CLK(15);
          break;
        case 0x7a: /*SP*/
          ADCHL(RegSP);
          CLK(15);
          break;

        // SBC HL,dd
        case 0x42: /*BC*/
          SBCHL(RegBC);
          CLK(15);
          break;
        case 0x52: /*DE*/
          SBCHL(RegDE);
          CLK(15);
          break;
        case 0x62: /*HL*/
          SBCHL(RegHL);
          CLK(15);
          break;
        case 0x72: /*SP*/
          SBCHL(RegSP);
          CLK(15);
          break;
      }
      break;  // 0xed
  }
}

// ---------------------------------------------------------------------------
//  CB - bit manipulation
//
void Z80C::CodeCB() {
  using RotFuncPtr = uint8_t (Z80C::*)(uint8_t);

  static const RotFuncPtr func[8] = {&Z80C::RLC, &Z80C::RRC, &Z80C::RL,
                                     &Z80C::RR,  &Z80C::SLA, &Z80C::SRA,
                                     &Z80C::SLL, &Z80C::SRL};

  int8_t ref = (index_mode == USEHL) ? 0 : int8_t(Fetch8());
  uint8_t fn = Fetch8();
  uint32_t rg = fn & 7;
  uint32_t bit = (fn >> 3) & 7;

  if (rg != 6) {
    uint8_t* p = ref_byte[rg];  // pointer to operand
    switch ((fn >> 6) & 3) {
      case 0:
        *p = (this->*func[bit])(*p);
        break;
      case 1:
        BIT(*p, bit);
        break;
      case 2:
        *p = RES(*p, bit);
        break;
      case 3:
        *p = SET(*p, bit);
        break;
    }
    CLK(8);
  } else {
    uint32_t b = *ref_hl[index_mode] + ref;
    uint8_t d = Read8(b);
    switch ((fn >> 6) & 3) {
      case 0:
        Write8(b, (this->*func[bit])(d));
        CLK(15);
        break;
      case 1:
        BIT(d, bit);
        CLK(12);
        SetXF(b >> 8);
        break;
      case 2:
        Write8(b, RES(d, bit));
        CLK(15);
        break;
      case 3:
        Write8(b, SET(d, bit));
        CLK(15);
        break;
    }
  }
}

void Z80C::CPI() {
  uint8_t n, f;
  n = Read8(RegHL++);
  RegBC = (RegBC - 1) & 0xffff;
  f = (((RegA & 0x0f) < (n & 0x0f)) ? HF : 0) | (RegBC ? PF : 0) | NF;

  SetFlags(HF | PF | NF, f);
  SetZS(RegA - n);
  SetXYForBlockInstruction(RegA - n - ((f & HF) >> 4));
  CLK(16);
}

void Z80C::SetXYForBlockInstruction(uint32_t n) {
  uint8_t xy_flag = 0;
  xy_flag |= ((n << 4) & YF);
  xy_flag |= (n & WF);
  SetXF(xy_flag);
}

void Z80C::CPD() {
  uint8_t n, f;
  n = Read8(RegHL--);
  RegBC = (RegBC - 1) & 0xffff;
  f = (((RegA & 0x0f) < (n & 0x0f)) ? HF : 0) | (RegBC ? PF : 0) | NF;
  SetFlags(HF | PF | NF, f);
  SetZS(RegA - n);
  SetXYForBlockInstruction(RegA - n - ((f & HF) >> 4));
  CLK(16);
}

uint8_t Z80C::GetCF() {
  if (uf & CF) {
    if (uf & WF) {
      if (nfa)
        SetFlags(CF, (fx32 < fy32) ? CF : 0);
      else
        SetFlags(CF, ((fx32 + fy32) & 0x20000ul) ? CF : 0);
    } else {
      if (nfa)
        SetFlags(CF, (fx < fy) ? CF : 0);
      else
        SetFlags(CF, ((fx + fy) & 0x200) ? CF : 0);
    }
  }
  return RegF & CF;
}

uint8_t Z80C::GetZF() {
  if (uf & ZF) {
    if (uf & WF) {
      if (nfa)
        SetFlags(ZF, ((fx32 - fy32) & 0x1fffeul) ? 0 : ZF);
      else
        SetFlags(ZF, ((fx32 + fy32) & 0x1fffeul) ? 0 : ZF);
    } else {
      if (nfa)
        SetFlags(ZF, ((fx - fy) & 0x1fe) ? 0 : ZF);
      else
        SetFlags(ZF, ((fx + fy) & 0x1fe) ? 0 : ZF);
    }
  }
  return RegF & ZF;
}

uint8_t Z80C::GetSF() {
  if (uf & SF) {
    if (uf & WF) {
      if (nfa)
        SetFlags(SF, ((fx32 - fy32) & 0x10000ul) ? SF : 0);
      else
        SetFlags(SF, ((fx32 + fy32) & 0x10000ul) ? SF : 0);
    } else {
      if (nfa)
        SetFlags(SF, ((fx - fy) & 0x100) ? SF : 0);
      else
        SetFlags(SF, ((fx + fy) & 0x100) ? SF : 0);
    }
  }
  return RegF & SF;
}

uint8_t Z80C::GetHF() {
  if (uf & HF) {
    if (uf & WF) {
      if (nfa)
        SetFlags(HF, ((fx32 & 0x1ffful) < (fy32 & 0x1ffful)) ? HF : 0);
      else
        SetFlags(HF,
                 (((fx32 & 0x1ffful) + (fy32 & 0x1ffful)) & 0x2000ul) ? HF : 0);
    } else {
      if (nfa)
        SetFlags(HF, ((fx & 0x1f) < (fy & 0x1f)) ? HF : 0);
      else
        SetFlags(HF, (((fx & 0x1f) + (fy & 0x1f)) & 0x20) ? HF : 0);
    }
  }
  return RegF & HF;
}

uint8_t Z80C::GetPF() {
  if (uf & PF) {
    if (uf & WF) {
      if (nfa)
        SetFlags(PF,
                 ((fx32 ^ fy32) & (fx32 ^ (fx32 - fy32)) & 0x10000ul) ? PF : 0);
      else
        SetFlags(
            PF, (~(fx32 ^ fy32) & (fx32 ^ (fx32 + fy32)) & 0x10000ul) ? PF : 0);
    } else {
      if (nfa)
        SetFlags(PF, ((fx ^ fy) & (fx ^ (fx - fy)) & 0x100) ? PF : 0);
      else
        SetFlags(PF, (~(fx ^ fy) & (fx ^ (fx + fy)) & 0x100) ? PF : 0);
    }
  }
  return RegF & PF;
}

void Z80C::SetZS(uint8_t a) {
  SetFlags(ZF | SF, ZSPTable[a] & (ZF | SF));
  SetXF(a);
}

void Z80C::SetZSP(uint8_t a) {
  SetFlags(ZF | SF | PF, ZSPTable[a]);
  SetXF(a);
}

void Z80C::OutTestIntr() {
  if (reg.iff1 && intr) {
    uint32_t w = Fetch8();
    if (w == 0xed) {
      w = Fetch8();
      if (((w & 0xc7) == 0x41) || ((w & 0xe7) == 0xa3)) {
        PCDec(2);
        return;
      } else {
        PCDec(1);
        SingleStep(0xed);
      }
    } else if (w == 0xd3) {
      PCDec(1);
      return;
    } else {
      SingleStep(w);
    }
    TestIntr();
  }
}

static inline void ToHex(char** p, uint32_t d) {
  static const char hex[] = "0123456789abcdef";
  *(*p)++ = hex[(d >> 12) & 15];
  *(*p)++ = hex[(d >> 8) & 15];
  *(*p)++ = hex[(d >> 4) & 15];
  *(*p)++ = hex[(d >> 0) & 15];
}

// ---------------------------------------------------------------------------
//  Dump Log
//  format
//  0         1         2         3         4         5         6
//  0123456789012345678901234567890123456789012345678901234567890
//  0000: 01234567890123456789 @:%%%% h:%%%% d:@@@@ b:@@@@ s:@@@@
//
void Z80C::DumpLog() {
  char buf[64];
  memset(buf, ' ', 64);

  // pc
  char* ptr = buf;
  uint32_t pc = GetPC();
  ToHex(&ptr, pc);
  buf[4] = ':';

  // inst
  diag.DisassembleS(pc, buf + 6);

  // regs
  ptr = buf + 27;
  *ptr++ = 'a';
  *ptr++ = ':';
  ToHex(&ptr, GetAF());
  ptr++;
  *ptr++ = 'h';
  *ptr++ = ':';
  ToHex(&ptr, RegHL);
  ptr++;
  *ptr++ = 'd';
  *ptr++ = ':';
  ToHex(&ptr, RegDE);
  ptr++;
  *ptr++ = 'b';
  *ptr++ = ':';
  ToHex(&ptr, RegBC);
  ptr++;
  *ptr++ = 's';
  *ptr++ = ':';
  ToHex(&ptr, RegSP);
  ptr++;
  *ptr++ = 10;

  if (dumplog_fp_)
    fwrite(buf, 1, ptr - buf, dumplog_fp_);
}

// ---------------------------------------------------------------------------
//
//
bool Z80C::EnableDump(bool dump) {
  if (dump) {
    if (!dumplog_fp_) {
      char buf[12];
      *(uint32_t*)buf = GetID();
      strcpy(buf + 4, ".dmp");
      dumplog_fp_ = fopen(buf, "w");
    }
  } else {
    if (dumplog_fp_) {
      fclose(dumplog_fp_);
      dumplog_fp_ = nullptr;
    }
  }
  return true;
}

uint32_t IFCALL Z80C::GetStatusSize() {
  return sizeof(Status);
}

bool IFCALL Z80C::SaveStatus(uint8_t* s) {
  Status* st = (Status*)s;
  GetAF();
  st->rev = ssrev;
  st->reg = reg;
  st->reg.pc = GetPC();
  st->intr = intr;
  st->wait = wait_state_;
  st->xf = xf;
  st->execcount = execcount;

  return true;
}

bool IFCALL Z80C::LoadStatus(const uint8_t* s) {
  const Status* st = (const Status*)s;
  if (st->rev != ssrev)
    return false;
  reg = st->reg;
  instbase_ = nullptr;
  instlim_ = nullptr;
  instpage_ = reinterpret_cast<uint8_t*>(~0);
  inst_ = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(reg.pc));

  intr = st->intr;
  wait_state_ = st->wait;
  xf = st->xf;
  execcount = st->execcount;
  return true;
}

// ---------------------------------------------------------------------------
//  Device descriptor
//
const Device::Descriptor Z80C::descriptor = {0, outdef};

const Device::OutFuncPtr Z80C::outdef[] = {
    static_cast<Device::OutFuncPtr>(&Z80C::Reset),
    static_cast<Device::OutFuncPtr>(&Z80C::IRQ),
    static_cast<Device::OutFuncPtr>(&Z80C::NMI),
};

// static
int Z80Util::ExecSingle(Z80C* first, Z80C* second, int clocks) {
  int c = first->GetCount();

  current_cpu_ = first;
  first->set_start_count(c);
  first->set_delay_count(c);

  first->SingleStep();
  first->TestIntr();

  int cbase = c;
  first->Exec0(c + clocks, c);

  c = first->GetCount();
  second->execcount = c;
  second->clockcount = 0;

  return c - cbase;
}

// Execute 2 CPUs
// static
int Z80Util::ExecDual(Z80C* first, Z80C* second, int count) {
  current_cpu_ = second;
  second->set_start_count(first->GetCount());
  second->set_delay_count(first->GetCount());

  second->SingleStep();
  second->TestIntr();

  current_cpu_ = first;
  first->set_start_count(second->GetCount());
  first->set_delay_count(second->GetCount());

  first->SingleStep();
  first->TestIntr();

  int c1 = first->GetCount();
  int c2 = second->GetCount();
  int delay = c2 - c1;
  int cbase = delay > 0 ? c1 : c2;
  int stop = cbase + count;

  while ((stop - first->GetCount() > 0) || (stop - second->GetCount() > 0)) {
    stop = first->Exec0(stop, second->GetCount());
    stop = second->Exec0(stop, first->GetCount());
  }
  return stop - cbase;
}

// Execute 2 CPUs
// static
int Z80Util::ExecDual2(Z80C* first, Z80C* second, int count) {
  current_cpu_ = second;
  second->set_start_count(first->GetCount());
  second->set_delay_count(first->GetCount());

  second->SingleStep();
  second->TestIntr();

  current_cpu_ = first;
  first->set_start_count(second->GetCount());
  first->set_delay_count(second->GetCount());

  first->SingleStep();
  first->TestIntr();

  int c1 = first->GetCount();
  int c2 = second->GetCount();
  int delay = c2 - c1;
  int cbase = delay > 0 ? c1 : c2;
  int stop = cbase + count;

  while ((stop - first->GetCount() > 0) || (stop - second->GetCount() > 0)) {
    stop = first->Exec0(stop, second->GetCount());
    stop = second->Exec1(stop, first->GetCount());
  }
  return stop - cbase;
}

Z80C* Z80Util::current_cpu_;
