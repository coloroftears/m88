// ---------------------------------------------------------------------------
//  Z80 emulator in C++
//  Copyright (C) cisc 1997, 1999.
// ----------------------------------------------------------------------------
//  $Id: Z80c.h,v 1.26 2001/02/21 11:57:16 cisc Exp $

#ifndef Z80C_h
#define Z80C_h

#include "win32/types.h"
#include "common/device.h"
#include "common/memmgr.h"
#include "devices/z80.h"
#include "devices/z80diag.h"

class IOBus;

#define Z80C_STATISTICS

// ----------------------------------------------------------------------------
//  Z80 Emulator
//
//  �g�p�\�ȋ@�\
//  Reset
//  INT
//  NMI
//
//  bool Init(MemoryManager* mem, IOBus* bus)
//  Z80 �G�~�����[�^������������
//  in:     bus     CPU ���Ȃ� Bus
//  out:            ���Ȃ���� true
//
//  uint32_t Exec(uint32_t clk)
//  �w�肵���N���b�N���������߂����s����
//  in:     clk     ���s����N���b�N��
//  out:            ���ۂɎ��s�����N���b�N��
//
//  int Stop(int clk)
//  ���s�c��N���b�N����ύX����
//  in:     clk
//
//  uint32_t GetCount()
//  �ʎZ���s�N���b�N�J�E���g���擾
//  out:
//
//  void Reset()
//  Z80 CPU �����Z�b�g����
//
//  void INT(int flag)
//  Z80 CPU �� INT ���荞�ݗv�����o��
//  in:     flag    true: ���荞�ݔ���
//                  false: ������
//
//  void NMI()
//  Z80 CPU �� NMI ���荞�ݗv�����o��
//
//  void Wait(bool wait)
//  Z80 CPU �̓�����~������
//  in:     wait    �~�߂�ꍇ true
//                  wait ��Ԃ̏ꍇ Exec �����߂����s���Ȃ��悤�ɂȂ�
//
class Z80C : public Device {
 public:
  enum {
    reset = 0,
    irq,
    nmi,
  };

  struct Statistics {
    uint32_t execute[0x10000];

    void Clear() { memset(execute, 0, sizeof(execute)); }
  };

 public:
  Z80C(const ID& id);
  ~Z80C();

  const Descriptor* IFCALL GetDesc() const { return &descriptor; }

  bool Init(MemoryManager* mem, IOBus* bus, int iack);

  int Exec(int count);
  int ExecOne();
  static int ExecSingle(Z80C* first, Z80C* second, int count);
  static int ExecDual(Z80C* first, Z80C* second, int count);
  static int ExecDual2(Z80C* first, Z80C* second, int count);

  void Stop(int count);
  static void StopDual(int count) {
    if (currentcpu)
      currentcpu->Stop(count);
  }
  int GetCount();
  static int GetCCount() {
    return currentcpu ? currentcpu->GetCount() - currentcpu->startcount : 0;
  }

  void IOCALL Reset(uint32_t = 0, uint32_t = 0);
  void IOCALL IRQ(uint32_t, uint32_t d) { intr = d; }
  void IOCALL NMI(uint32_t = 0, uint32_t = 0);
  void Wait(bool flag);

  uint32_t IFCALL GetStatusSize();
  bool IFCALL SaveStatus(uint8_t* status);
  bool IFCALL LoadStatus(const uint8_t* status);

  uint32_t GetPC();
  void SetPC(uint32_t newpc);
  const Z80Reg& GetReg() { return reg; }

  bool GetPages(MemoryPage** rd, MemoryPage** wr) {
    *rd = rdpages, *wr = wrpages;
    return true;
  }
  int* GetWaits() { return 0; }

  void TestIntr();
  bool IsIntr() { return !!intr; }
  bool EnableDump(bool dump);
  int GetDumpState() { return !!dumplog; }

  Statistics* GetStatistics();

 private:
  enum {
    pagebits = MemoryManagerBase::pagebits,
    pagemask = MemoryManagerBase::pagemask,
    idbit = MemoryManagerBase::idbit
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

  uint8_t* inst;      // PC �̎w���������̃|�C���^�C�܂��� PC ���̂���
  uint8_t* instlim;   // inst �̗L�����
  uint8_t* instbase;  // inst - PC        (PC = inst - instbase)
  uint8_t* instpage;

  Z80Reg reg;
  IOBus* bus;
  static const Descriptor descriptor;
  static const OutFuncPtr outdef[];
  static Z80C* currentcpu;
  static int cbase;

  int execcount;
  int clockcount;
  int stopcount;
  int delaycount;
  int intack;
  int intr;
  int waitstate;  // b0:HALT b1:WAIT
  int eshift;
  int startcount;

  enum index { USEHL, USEIX, USEIY };
  index index_mode;    /* HL/IX/IY �ǂ���Q�Ƃ��邩 */
  uint8_t uf;          /* ���v�Z�t���O */
  uint8_t nfa;         /* �Ō�̉����Z�̎�� */
  uint8_t xf;          /* ����`�t���O(��3,5�r�b�g) */
  uint32_t fx32, fy32; /* �t���O�v�Z�p�̃f�[�^ */
  uint32_t fx, fy;

  uint8_t* ref_h[3];          /* H / XH / YH �̃e�[�u�� */
  uint8_t* ref_l[3];          /* L / YH / YL �̃e�[�u�� */
  Z80Reg::wordreg* ref_hl[3]; /* HL/ IX / IY �̃e�[�u�� */
  uint8_t* ref_byte[8];       /* BCDEHL A �̃e�[�u�� */
  FILE* dumplog;
  Z80Diag diag;

  MemoryPage rdpages[0x10000 >> MemoryManager::pagebits];
  MemoryPage wrpages[0x10000 >> MemoryManager::pagebits];

#ifdef Z80C_STATISTICS
  Statistics statistics;
#endif

  // �����C���^�[�t�F�[�X
 private:
  uint32_t Read8(uint32_t addr);
  uint32_t Read16(uint32_t a);
  uint32_t Fetch8();
  uint32_t Fetch16();
  void Write8(uint32_t addr, uint32_t data);
  void Write16(uint32_t a, uint32_t d);
  uint32_t Inp(uint32_t port);
  void Outp(uint32_t port, uint32_t data);
  uint32_t Fetch8B();
  uint32_t Fetch16B();

  void SingleStep(uint32_t inst);
  void SingleStep();
  void Init();
  int Exec0(int stop, int d);
  int Exec1(int stop, int d);
  bool Sync();
  void OutTestIntr();

  void SetPCi(uint32_t newpc);
  void PCInc(uint32_t inc);
  void PCDec(uint32_t dec);

  void Call(), Jump(uint32_t dest), JumpR();
  uint8_t GetCF(), GetZF(), GetSF();
  uint8_t GetHF(), GetPF();
  void SetM(uint32_t n);
  uint8_t GetM();
  void Push(uint32_t n);
  uint32_t Pop();
  void ADDA(uint8_t), ADCA(uint8_t), SUBA(uint8_t);
  void SBCA(uint8_t), ANDA(uint8_t), ORA(uint8_t);
  void XORA(uint8_t), CPA(uint8_t);
  uint8_t Inc8(uint8_t), Dec8(uint8_t);
  uint32_t ADD16(uint32_t x, uint32_t y);
  void ADCHL(uint32_t y), SBCHL(uint32_t y);
  uint32_t GetAF();
  void SetAF(uint32_t n);
  void SetZS(uint8_t a), SetZSP(uint8_t a);
  void CPI(), CPD();
  void CodeCB();

  uint8_t RLC(uint8_t), RRC(uint8_t), RL(uint8_t);
  uint8_t RR(uint8_t), SLA(uint8_t), SRA(uint8_t);
  uint8_t SLL(uint8_t), SRL(uint8_t);
};

// ---------------------------------------------------------------------------
//  �N���b�N�J�E���^�擾
//
inline int Z80C::GetCount() {
  return execcount + (clockcount << eshift);
}

inline uint32_t Z80C::GetPC() {
  return inst - instbase;
}

inline Z80C::Statistics* Z80C::GetStatistics() {
#ifdef Z80C_STATISTICS
  return &statistics;
#else
  return 0;
#endif
}

#endif  // Z80C.h
