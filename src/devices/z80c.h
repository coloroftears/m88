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
//  uint Exec(uint clk)
//  �w�肵���N���b�N���������߂����s����
//  in:     clk     ���s����N���b�N��
//  out:            ���ۂɎ��s�����N���b�N��
//
//  int Stop(int clk)
//  ���s�c��N���b�N����ύX����
//  in:     clk
//
//  uint GetCount()
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
    uint execute[0x10000];

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

  void IOCALL Reset(uint = 0, uint = 0);
  void IOCALL IRQ(uint, uint d) { intr = d; }
  void IOCALL NMI(uint = 0, uint = 0);
  void Wait(bool flag);

  uint IFCALL GetStatusSize();
  bool IFCALL SaveStatus(uint8* status);
  bool IFCALL LoadStatus(const uint8* status);

  uint GetPC();
  void SetPC(uint newpc);
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
    uint8 intr;
    uint8 wait;
    uint8 xf;
    uint8 rev;
    int execcount;
  };

  void DumpLog();

  uint8* inst;      // PC �̎w���������̃|�C���^�C�܂��� PC ���̂���
  uint8* instlim;   // inst �̗L�����
  uint8* instbase;  // inst - PC        (PC = inst - instbase)
  uint8* instpage;

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
  index index_mode;  /* HL/IX/IY �ǂ���Q�Ƃ��邩 */
  uint8 uf;          /* ���v�Z�t���O */
  uint8 nfa;         /* �Ō�̉����Z�̎�� */
  uint8 xf;          /* ����`�t���O(��3,5�r�b�g) */
  uint32_t fx32, fy32; /* �t���O�v�Z�p�̃f�[�^ */
  uint fx, fy;

  uint8* ref_h[3];            /* H / XH / YH �̃e�[�u�� */
  uint8* ref_l[3];            /* L / YH / YL �̃e�[�u�� */
  Z80Reg::wordreg* ref_hl[3]; /* HL/ IX / IY �̃e�[�u�� */
  uint8* ref_byte[8];         /* BCDEHL A �̃e�[�u�� */
  FILE* dumplog;
  Z80Diag diag;

  MemoryPage rdpages[0x10000 >> MemoryManager::pagebits];
  MemoryPage wrpages[0x10000 >> MemoryManager::pagebits];

#ifdef Z80C_STATISTICS
  Statistics statistics;
#endif

  // �����C���^�[�t�F�[�X
 private:
  uint Read8(uint addr);
  uint Read16(uint a);
  uint Fetch8();
  uint Fetch16();
  void Write8(uint addr, uint data);
  void Write16(uint a, uint d);
  uint Inp(uint port);
  void Outp(uint port, uint data);
  uint Fetch8B();
  uint Fetch16B();

  void SingleStep(uint inst);
  void SingleStep();
  void Init();
  int Exec0(int stop, int d);
  int Exec1(int stop, int d);
  bool Sync();
  void OutTestIntr();

  void SetPCi(uint newpc);
  void PCInc(uint inc);
  void PCDec(uint dec);

  void Call(), Jump(uint dest), JumpR();
  uint8 GetCF(), GetZF(), GetSF();
  uint8 GetHF(), GetPF();
  void SetM(uint n);
  uint8 GetM();
  void Push(uint n);
  uint Pop();
  void ADDA(uint8), ADCA(uint8), SUBA(uint8);
  void SBCA(uint8), ANDA(uint8), ORA(uint8);
  void XORA(uint8), CPA(uint8);
  uint8 Inc8(uint8), Dec8(uint8);
  uint ADD16(uint x, uint y);
  void ADCHL(uint y), SBCHL(uint y);
  uint GetAF();
  void SetAF(uint n);
  void SetZS(uint8 a), SetZSP(uint8 a);
  void CPI(), CPD();
  void CodeCB();

  uint8 RLC(uint8), RRC(uint8), RL(uint8);
  uint8 RR(uint8), SLA(uint8), SRA(uint8);
  uint8 SLL(uint8), SRL(uint8);
};

// ---------------------------------------------------------------------------
//  �N���b�N�J�E���^�擾
//
inline int Z80C::GetCount() {
  return execcount + (clockcount << eshift);
}

inline uint Z80C::GetPC() {
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
