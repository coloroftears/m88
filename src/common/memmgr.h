// ---------------------------------------------------------------------------
//  Memory Manager class
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: memmgr.h,v 1.4 1999/12/28 10:33:54 cisc Exp $

#ifndef incl_memmgr_h
#define incl_memmgr_h

#include "if/ifcommon.h"

// ---------------------------------------------------------------------------
//  �������Ǘ��N���X
//
struct MemoryPage {
  intpointer ptr;
  void* inst;
#ifndef PTR_IDBIT
  bool func;
#endif
};

class MemoryManagerBase {
 public:
  typedef MemoryPage Page;
  enum {
    ndevices = 8,
    pagebits = 10,
    pagemask = (1 << pagebits) - 1,
#ifdef PTR_IDBIT
    idbit = PTR_IDBIT,
#else
    idbit = 0,
#endif
  };

 public:
  MemoryManagerBase();
  ~MemoryManagerBase();

  bool Init(uint sas, Page* pages = 0);
  void Cleanup();
  int Connect(void* inst, bool high = false);
  bool Disconnect(uint pid);
  bool Disconnect(void* inst);
  bool Release(uint pid, uint page, uint top);

 protected:
  bool Alloc(uint pid,
             uint page,
             uint top,
             intpointer ptr,
             int incr,
             bool func);

  struct DPage {
    intpointer ptr;
#ifndef PTR_IDBIT
    bool func;
#endif
  };
  struct LocalSpace {
    void* inst;
    DPage* pages;
  };

  Page* pages;
  uint npages;
  bool ownpages;

  uint8_t* priority;
  LocalSpace lsp[ndevices];
};

// ---------------------------------------------------------------------------

class ReadMemManager : public MemoryManagerBase {
 public:
  typedef uint(MEMCALL* RdFunc)(void* inst, uint addr);

  bool Init(uint sas, Page* pages = 0);
  bool AllocR(uint pid, uint addr, uint length, uint8_t* ptr);
  bool AllocR(uint pid, uint addr, uint length, RdFunc ptr);
  bool ReleaseR(uint pid, uint addr, uint length);
  uint Read8(uint addr);
  uint Read8P(uint pid, uint addr);

 private:
  static uint MEMCALL UndefinedRead(void*, uint);
};

// ---------------------------------------------------------------------------

class WriteMemManager : public MemoryManagerBase {
 public:
  typedef void(MEMCALL* WrFunc)(void* inst, uint addr, uint data);

 public:
  bool Init(uint sas, Page* pages = 0);
  bool AllocW(uint pid, uint addr, uint length, uint8_t* ptr);
  bool AllocW(uint pid, uint addr, uint length, WrFunc ptr);
  bool ReleaseW(uint pid, uint addr, uint length);
  void Write8(uint addr, uint data);
  void Write8P(uint pid, uint addr, uint data);

 private:
  static void MEMCALL UndefinedWrite(void*, uint, uint);
};

// ---------------------------------------------------------------------------

class MemoryManager : public IMemoryManager,
                      public IMemoryAccess,
                      private ReadMemManager,
                      private WriteMemManager {
 public:
  enum {
    pagebits = ::MemoryManagerBase::pagebits,
    pagemask = ::MemoryManagerBase::pagemask,
    idbit = ::MemoryManagerBase::idbit,
  };
  typedef ReadMemManager::RdFunc RdFunc;
  typedef WriteMemManager::WrFunc WrFunc;

  bool Init(uint sas, Page* read = 0, Page* write = 0);
  int IFCALL Connect(void* inst, bool highpriority = false);
  bool IFCALL Disconnect(uint pid);
  bool Disconnect(void* inst);

  bool IFCALL AllocR(uint pid, uint addr, uint length, uint8_t* ptr) {
    return ReadMemManager::AllocR(pid, addr, length, ptr);
  }
  bool IFCALL AllocR(uint pid, uint addr, uint length, RdFunc ptr) {
    return ReadMemManager::AllocR(pid, addr, length, ptr);
  }
  bool IFCALL ReleaseR(uint pid, uint addr, uint length) {
    return ReadMemManager::ReleaseR(pid, addr, length);
  }
  uint IFCALL Read8(uint addr) { return ReadMemManager::Read8(addr); }
  uint IFCALL Read8P(uint pid, uint addr) {
    return ReadMemManager::Read8P(pid, addr);
  }
  bool IFCALL AllocW(uint pid, uint addr, uint length, uint8_t* ptr) {
    return WriteMemManager::AllocW(pid, addr, length, ptr);
  }
  bool IFCALL AllocW(uint pid, uint addr, uint length, WrFunc ptr) {
    return WriteMemManager::AllocW(pid, addr, length, ptr);
  }
  bool IFCALL ReleaseW(uint pid, uint addr, uint length) {
    return WriteMemManager::ReleaseW(pid, addr, length);
  }
  void IFCALL Write8(uint addr, uint data) {
    WriteMemManager::Write8(addr, data);
  }
  void IFCALL Write8P(uint pid, uint addr, uint data) {
    WriteMemManager::Write8P(pid, addr, data);
  }
};

// ---------------------------------------------------------------------------

inline bool MemoryManager::Init(uint sas, Page* read, Page* write) {
  if (!read ^ !write)
    return false;
  return ReadMemManager::Init(sas, read) && WriteMemManager::Init(sas, write);
}

inline int IFCALL MemoryManager::Connect(void* inst, bool high) {
  int r = ReadMemManager::Connect(inst, high);
  int w = WriteMemManager::Connect(inst, high);
  assert(r == w);
  return r;
}

inline bool IFCALL MemoryManager::Disconnect(uint pid) {
  return ReadMemManager::Disconnect(pid) & WriteMemManager::Disconnect(pid);
}

inline bool MemoryManager::Disconnect(void* inst) {
  return ReadMemManager::Disconnect(inst) & WriteMemManager::Disconnect(inst);
}

// ---------------------------------------------------------------------------
//  ��������Ԃ̎擾
//
inline bool MemoryManagerBase::Alloc(uint pid,
                                     uint page,
                                     uint top,
                                     intpointer ptr,
                                     int incr,
                                     bool func) {
  LocalSpace& ls = lsp[pid];
  assert(ls.inst);
  assert(page < top);

  uint8_t* pri = priority + page * ndevices;
  for (; page < top; page++, pri += ndevices) {
    // ���݂̃y�[�W�� owner �����������Ⴂ�D��x�����ꍇ
    // priority �̏����������s��
    for (int i = pid; pri[i] > pid && i >= 0; i--) {
      pri[i] = pid;
    }
    if (pri[0] == pid) {
      // �������y�[�W�̗D�挠�����Ȃ� Page �̏�������
      pages[page].inst = ls.inst;
      pages[page].ptr = ptr;
#ifndef PTR_IDBIT
      pages[page].func = func;
#endif
    }
    // ���[�J���y�[�W�̑������X�V
    ls.pages[page].ptr = ptr;
#ifndef PTR_IDBIT
    ls.pages[page].func = func;
#endif
    ptr += incr;
  }
  return true;
}

// ---------------------------------------------------------------------------
//  ��������Ԃ̊J��
//
inline bool MemoryManagerBase::Release(uint pid, uint page, uint top) {
  if (pid < ndevices - 1)  // �ŉ��ʂ̃f�o�C�X�� Release �ł��Ȃ�
  {
    LocalSpace& ls = lsp[pid];
    assert(ls.inst);

    uint8_t* pri = priority + page * ndevices;
    for (; page < top; page++, pri += ndevices) {
      // �������������������]����y�[�W�Ȃ��
      if (pri[pid] == pid) {
        int npid = pri[pid + 1];
        // priority �̏�������
        for (int i = pid; i >= 1 && pri[i] >= pid; i--) {
          pri[i] = npid;
        }
        if (pri[0] == pid) {
          pri[0] = npid;
          pages[page].inst = lsp[npid].inst;
          pages[page].ptr = lsp[npid].pages[page].ptr;
        }
      }
      ls.pages[page].ptr = 0;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------

inline bool ReadMemManager::AllocR(uint pid,
                                   uint addr,
                                   uint length,
                                   uint8_t* ptr) {
  assert((intpointer(ptr) & idbit) == 0);
  uint page = addr >> pagebits;
  uint top = (addr + length + pagemask) >> pagebits;
  return Alloc(pid, page, top, intpointer(ptr), 1 << pagebits, false);
}

// ---------------------------------------------------------------------------

inline bool ReadMemManager::AllocR(uint pid,
                                   uint addr,
                                   uint length,
                                   RdFunc ptr) {
  assert((intpointer(ptr) & idbit) == 0);
  uint page = addr >> pagebits;
  uint top = (addr + length + pagemask) >> pagebits;

  return Alloc(pid, page, top, intpointer(ptr) | idbit, 0, true);
}

// ---------------------------------------------------------------------------

inline bool ReadMemManager::ReleaseR(uint pid, uint addr, uint length) {
  uint page = addr >> pagebits;
  uint top = (addr + length + pagemask) >> pagebits;
  return Release(pid, page, top);
}

// ---------------------------------------------------------------------------

inline bool WriteMemManager::AllocW(uint pid,
                                    uint addr,
                                    uint length,
                                    uint8_t* ptr) {
  assert((intpointer(ptr) & idbit) == 0);
  uint page = addr >> pagebits;
  uint top = (addr + length + pagemask) >> pagebits;
  return Alloc(pid, page, top, intpointer(ptr), 1 << pagebits, false);
}

// ---------------------------------------------------------------------------

inline bool WriteMemManager::AllocW(uint pid,
                                    uint addr,
                                    uint length,
                                    WrFunc ptr) {
  assert((intpointer(ptr) & idbit) == 0);
  uint page = addr >> pagebits;
  uint top = (addr + length + pagemask) >> pagebits;
  return MemoryManagerBase::Alloc(pid, page, top, intpointer(ptr) | idbit, 0,
                                  true);
}

// ---------------------------------------------------------------------------

inline bool WriteMemManager::ReleaseW(uint pid, uint addr, uint length) {
  uint page = addr >> pagebits;
  uint top = (addr + length + pagemask) >> pagebits;
  return Release(pid, page, top);
}

// ---------------------------------------------------------------------------
//  ����������̓ǂݍ���
//
inline uint ReadMemManager::Read8(uint addr) {
  Page& page = pages[addr >> pagebits];
#ifdef PTR_IDBIT
  if (!(page.ptr & idbit))
    return ((uint8_t*)page.ptr)[addr & pagemask];
  else
    return (*RdFunc(page.ptr & ~idbit))(page.inst, addr);
#else
  if (!page.func)
    return ((uint8_t*)page.ptr)[addr & pagemask];
  else
    return (*RdFunc(page.ptr))(page.inst, addr);
#endif
}

// ---------------------------------------------------------------------------
//  �������ւ̏�����
//
inline void WriteMemManager::Write8(uint addr, uint data) {
  Page& page = pages[addr >> pagebits];
#ifdef PTR_IDBIT
  if (!(page.ptr & idbit))
    ((uint8_t*)page.ptr)[addr & pagemask] = data;
  else
    (*WrFunc(page.ptr & ~idbit))(page.inst, addr, data);
#else
  if (!page.func)
    ((uint8_t*)page.ptr)[addr & pagemask] = data;
  else
    (*WrFunc(page.ptr))(page.inst, addr, data);
#endif
}

#endif  // incl_memmgr_h
