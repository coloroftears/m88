// ---------------------------------------------------------------------------
//  �������Ǘ��N���X
//  Copyright (c) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: memmgr.cpp,v 1.4 1999/12/28 10:33:53 cisc Exp $

#include "win32/headers.h"
#include "common/memmgr.h"
#include "win32/diag.h"

// ---------------------------------------------------------------------------
//  �\�z�E�j��
//
MemoryManagerBase::MemoryManagerBase()
    : ownpages(false), pages(0), npages(0), priority(0) {
  lsp[0].pages = 0;
}

MemoryManagerBase::~MemoryManagerBase() {
  Cleanup();
}

// ---------------------------------------------------------------------------
//  ������
//
bool MemoryManagerBase::Init(uint32_t sas, Page* expages) {
  Cleanup();

  // pages
  npages = (sas + pagemask) >> pagebits;

  if (expages) {
    pages = expages;
    ownpages = false;
  } else {
    pages = new Page[npages];
    if (!pages)
      return false;
    ownpages = true;
  }

  // devices
  //  lsp = new LocalSpace[ndevices];
  //  if (!lsp)
  //      return false;

  lsp[0].pages = new DPage[npages * ndevices];
  for (int i = 0; i < ndevices; i++) {
    lsp[i].inst = 0;
    lsp[i].pages = lsp[0].pages + (i * npages);
  }

  // priority list
  priority = new uint8_t[npages * ndevices];
  if (!priority)
    return false;
  memset(priority, ndevices - 1, npages * ndevices);

  return true;
}

// ---------------------------------------------------------------------------
//  ��n��
//
void MemoryManagerBase::Cleanup() {
  if (ownpages) {
    delete[] pages;
    pages = 0;
  }
  delete[] priority;
  priority = 0;
  //  if (lsp)
  {
    delete[] lsp[0].pages;
    //      delete[] lsp; lsp = 0;
  }
}

// ---------------------------------------------------------------------------
//  ��������Ԃ��g�p������ device ��ǉ�����
//
int MemoryManagerBase::Connect(void* inst, bool high) {
  int pid = high ? 0 : ndevices - 1;
  int end = high ? ndevices - 1 : 0;
  int step = high ? 1 : -1;

  for (; pid != end; pid += step) {
    LocalSpace& ls = lsp[pid];

    // ��� lsp ��T��
    if (!ls.inst) {
      ls.inst = inst;
      for (uint32_t i = 0; i < npages; i++) {
        ls.pages[i].ptr = 0;
      }
      return pid;
    }
  }
  return -1;
}

// ---------------------------------------------------------------------------
//  �f�o�C�X�����O��
//
bool MemoryManagerBase::Disconnect(uint32_t pid) {
  Release(pid, 0, npages);
  lsp[pid].inst = 0;
  return true;
}

// ---------------------------------------------------------------------------
//  �f�o�C�X�����O��
//
bool MemoryManagerBase::Disconnect(void* inst) {
  for (int i = 0; i < ndevices - 1; i++) {
    if (lsp[i].inst == inst)
      return Disconnect(i);
  }
  return false;
}

// ---------------------------------------------------------------------------
//  ������
//
bool ReadMemManager::Init(uint32_t sas, Page* _pages) {
  if (!MemoryManagerBase::Init(sas, _pages))
    return false;

  for (uint32_t i = 0; i < npages; i++) {
#ifdef PTR_IDBIT
    pages[i].ptr = (intpointer(UndefinedRead) | idbit);
#else
    pages[i].ptr = intpointer(UndefinedRead);
    pages[i].func = true;
#endif
  }
  return true;
}

// ---------------------------------------------------------------------------
//  �w�肳�ꂽ pid �̒���̃�������Ԃ̓ǂݍ���
//
uint32_t ReadMemManager::Read8P(uint32_t pid, uint32_t addr) {
  assert(pid < ndevices - 1);

  int page = addr >> pagebits;
  LocalSpace& ls = lsp[priority[page * ndevices + pid + 1]];

#ifdef PTR_IDBIT
  if (!(ls.pages[page].ptr & idbit))
    return ((uint8_t*)ls.pages[page].ptr)[addr & pagemask];
  else
    return (*RdFunc(ls.pages[page].ptr & ~idbit))(ls.inst, addr);
#else
  if (!ls.pages[page].func)
    return ((uint8_t*)ls.pages[page].ptr)[addr & pagemask];
  else
    return (*RdFunc(ls.pages[page].ptr))(ls.inst, addr);
#endif
}

// ---------------------------------------------------------------------------
//  ����[
//
uint32_t ReadMemManager::UndefinedRead(void*, uint32_t addr) {
  LOG2("bus: Read on undefined memory page 0x%x. (addr:0x%.4x)\n",
       addr >> pagebits, addr);
  return 0xff;
}

// ---------------------------------------------------------------------------
//  ������
//
bool WriteMemManager::Init(uint32_t sas, Page* _pages) {
  if (!MemoryManagerBase::Init(sas, _pages))
    return false;

  for (uint32_t i = 0; i < npages; i++) {
#ifdef PTR_IDBIT
    pages[i].ptr = (intpointer(UndefinedWrite) | idbit);
#else
    pages[i].ptr = intpointer(UndefinedWrite);
    pages[i].func = true;
#endif
  }
  return true;
}

// ---------------------------------------------------------------------------
//  �w�肳�ꂽ pid �̒���̃�������Ԃɑ΂��鏑����
//
void WriteMemManager::Write8P(uint32_t pid, uint32_t addr, uint32_t data) {
  assert(pid < ndevices - 1);

  int page = addr >> pagebits;
  LocalSpace& ls = lsp[priority[page * ndevices + pid + 1]];

#ifdef PTR_IDBIT
  if (!(ls.pages[page].ptr & idbit))
    ((uint8_t*)ls.pages[page].ptr)[addr & pagemask] = data;
  else
    (*WrFunc(ls.pages[page].ptr & ~idbit))(ls.inst, addr, data);
#else
  if (!ls.pages[page].func)
    ((uint8_t*)ls.pages[page].ptr)[addr & pagemask] = data;
  else
    (*WrFunc(ls.pages[page].ptr))(ls.inst, addr, data);
#endif
}

// ---------------------------------------------------------------------------
//  ����[
//
void WriteMemManager::UndefinedWrite(void*, uint32_t addr, uint32_t) {
  LOG2("bus: Write on undefined memory page 0x%x. (addr:0x%.4x)\n",
       addr >> pagebits, addr);
}
