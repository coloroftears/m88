// ---------------------------------------------------------------------------
//  Virtual Bus Implementation
//  Copyright (c) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: device_i.h,v 1.9 1999/08/26 08:05:55 cisc Exp $

#ifndef core_device_i_h
#define core_device_i_h

#include <assert.h>

// ---------------------------------------------------------------------------
//  MemoryBus inline funcitions

// ---------------------------------------------------------------------------
//  �o���N�������݂Ƀ����������蓖�Ă�
//
inline void MemoryBus::SetWriteMemory(uint32_t addr, void* ptr) {
  assert((uint32_t(ptr) & idbit) == 0 && (addr & pagemask) == 0);
  pages[addr >> pagebits].write = ptr;
}

// ---------------------------------------------------------------------------
//  �o���N�ǂݍ��݂Ƀ����������蓖�Ă�
//
inline void MemoryBus::SetReadMemory(uint32_t addr, void* ptr) {
  assert((uint32_t(ptr) & idbit) == 0 && (addr & pagemask) == 0);
  pages[addr >> pagebits].read = ptr;
}

// ---------------------------------------------------------------------------
//  �o���N�ǂݏ����Ƀ����������蓖�Ă�
//
inline void MemoryBus::SetMemory(uint32_t addr, void* ptr) {
  assert((uint32_t(ptr) & idbit) == 0 && (addr & pagemask) == 0);
  Page* page = &pages[addr >> pagebits];
  page->read = ptr;
  page->write = ptr;
}

// ---------------------------------------------------------------------------
//  �o���N�ǂݏ����Ɋ֐������蓖�Ă�
//
inline void MemoryBus::SetFunc(uint32_t addr,
                               void* inst,
                               ReadFuncPtr rd,
                               WriteFuncPtr wr) {
  assert((addr & pagemask) == 0);
  assert((intpointer(rd) & idbit) == 0 && (intpointer(wr) & idbit) == 0);
  Page* page = &pages[addr >> pagebits];
  page->read = (void*)(intpointer(rd) | idbit);
  page->write = (void*)(intpointer(wr) | idbit);
  page->inst = inst;
}

// ---------------------------------------------------------------------------
//  �����̃o���N�������݂ɘA�����������������蓖�Ă�
//  npages �͌Œ�̕����D�܂�������
//
inline void MemoryBus::SetWriteMemorys(uint32_t addr,
                                       uint32_t length,
                                       uint8_t* ptr) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);
  assert((uint32_t(ptr) & idbit) == 0);

  Page* page = pages + (addr >> pagebits);
  int npages = length >> pagebits;

  if (!(npages & 3) || npages >= 16) {
    for (int i = npages & 3; i > 0; i--) {
      (page++)->write = ptr;
      ptr += 1 << pagebits;
    }
    for (npages >>= 2; npages > 0; npages--) {
      page[0].write = ptr;
      page[1].write = ptr + (1 << pagebits);
      page[2].write = ptr + (2 << pagebits);
      page[3].write = ptr + (3 << pagebits);
      page += 4;
      ptr += 4 << pagebits;
    }
  } else {
    for (; npages > 0; npages--) {
      (page++)->write = ptr;
      ptr += 1 << pagebits;
    }
  }
}

// ---------------------------------------------------------------------------
//  �����̃o���N�������݂ɘA�����������������蓖�Ă�
//  ���L�҃`�F�b�N�t��
//
inline void MemoryBus::SetWriteMemorys2(uint32_t addr,
                                        uint32_t length,
                                        uint8_t* ptr,
                                        void* inst) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);
  assert((uint32_t(ptr) & idbit) == 0);

  Page* page = pages + (addr >> pagebits);
  Owner* owner = owners + (addr >> pagebits);
  int npages = length >> pagebits;

  for (; npages > 0; npages--) {
    if (owner->write == inst)
      page->write = ptr;
    owner++, page++;
    ptr += 1 << pagebits;
  }
}

// ---------------------------------------------------------------------------
//  �����̃o���N�ǂݍ��݂ɘA�����������������蓖�Ă�
//  npages �͌Œ�̕����D�܂�������
//
inline void MemoryBus::SetReadMemorys(uint32_t addr,
                                      uint32_t length,
                                      uint8_t* ptr) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);
  assert((uint32_t(ptr) & idbit) == 0);

  Page* page = pages + (addr >> pagebits);
  uint32_t npages = length >> pagebits;

  if (!(npages & 3) || npages >= 16) {
    for (int i = npages & 3; i > 0; i--) {
      (page++)->read = ptr;
      ptr += 1 << pagebits;
    }
    for (npages >>= 2; npages > 0; npages--) {
      page[0].read = ptr;
      page[1].read = ptr + (1 << pagebits);
      page[2].read = ptr + (2 << pagebits);
      page[3].read = ptr + (3 << pagebits);
      page += 4;
      ptr += 4 << pagebits;
    }
  } else {
    for (; npages > 0; npages--) {
      (page++)->read = ptr;
      ptr += 1 << pagebits;
    }
  }
}

// ---------------------------------------------------------------------------
//  �����̃o���N�ǂݍ��݂ɘA�����������������蓖�Ă�
//  npages �͌Œ�̕����D�܂�������
//
inline void MemoryBus::SetReadMemorys2(uint32_t addr,
                                       uint32_t length,
                                       uint8_t* ptr,
                                       void* inst) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);
  assert((uint32_t(ptr) & idbit) == 0);

  Page* page = pages + (addr >> pagebits);
  Owner* owner = owners + (addr >> pagebits);
  uint32_t npages = length >> pagebits;

  for (; npages > 0; npages--) {
    if (owner->read == inst)
      page->read = ptr;
    owner++, page++;
    ptr += 1 << pagebits;
  }
}

// ---------------------------------------------------------------------------
//  �����̃o���N�ǂݏ����ɘA�����������������蓖�Ă�
//  npages �͌Œ�̕����D�܂�������
//
inline void MemoryBus::SetMemorys(uint32_t addr,
                                  uint32_t length,
                                  uint8_t* ptr) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);
  assert((uint32_t(ptr) & idbit) == 0);

  Page* page = pages + (addr >> pagebits);
  uint32_t npages = length >> pagebits;

  if (!(npages & 3) || npages >= 16) {
    for (int i = npages & 3; i > 0; i--) {
      page->read = ptr;
      page->write = ptr;
      page++;
      ptr += 1 << pagebits;
    }
    for (npages >>= 2; npages > 0; npages--) {
      page[0].read = page[0].write = ptr;
      page[1].read = page[1].write = ptr + (1 << pagebits);
      page[2].read = page[2].write = ptr + (2 << pagebits);
      page[3].read = page[3].write = ptr + (3 << pagebits);
      page += 4;
      ptr += 4 << pagebits;
    }
  } else {
    for (; npages > 0; npages--) {
      page->read = ptr;
      page->write = ptr;
      page++;
      ptr += 1 << pagebits;
    }
  }
}

// ---------------------------------------------------------------------------
//  �����̃o���N�ǂݏ����ɘA�����������������蓖�Ă�
//  npages �͌Œ�̕����D�܂�������
//
inline void MemoryBus::SetMemorys2(uint32_t addr,
                                   uint32_t length,
                                   uint8_t* ptr,
                                   void* inst) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);
  assert((uint32_t(ptr) & idbit) == 0);

  Page* page = pages + (addr >> pagebits);
  Owner* owner = owners + (addr >> pagebits);
  uint32_t npages = length >> pagebits;

  for (; npages > 0; npages--) {
    if (owner->read == inst)
      page->read = ptr;
    if (owner->write == inst)
      page->write = ptr;
    owner++;
    page++;
    ptr += 1 << pagebits;
  }
}

// ---------------------------------------------------------------------------
//  �����̃o���N�ǂݏ����Ɋ֐������蓖�Ă�
//  npages �͌Œ�̕����D�܂�������
//
inline void MemoryBus::SetFuncs(uint32_t addr,
                                uint32_t length,
                                void* inst,
                                ReadFuncPtr rd,
                                WriteFuncPtr wr) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);
  assert((intpointer(rd) & idbit) == 0 && (intpointer(wr) & idbit) == 0);

  void* r = (void*)(intpointer(rd) | idbit);
  void* w = (void*)(intpointer(wr) | idbit);

  Page* page = pages + (addr >> pagebits);
  uint32_t npages = length >> pagebits;

  if (!(npages & 3) || npages >= 16) {
    for (int i = npages & 3; i > 0; i--) {
      page->read = r;
      page->write = w;
      page->inst = inst;
      page++;
    }
    for (npages >>= 2; npages > 0; npages--) {
      page[0].read = r;
      page[0].write = w;
      page[0].inst = inst;
      page[1].read = r;
      page[1].write = w;
      page[1].inst = inst;
      page[2].read = r;
      page[2].write = w;
      page[2].inst = inst;
      page[3].read = r;
      page[3].write = w;
      page[3].inst = inst;
      page += 4;
    }
  } else {
    for (; npages > 0; npages--) {
      page->read = r;
      page->write = w;
      page->inst = inst;
      page++;
    }
  }
}

// ---------------------------------------------------------------------------
//  �����̃o���N�ǂݏ����Ɋ֐������蓖�Ă�
//
inline void MemoryBus::SetFuncs2(uint32_t addr,
                                 uint32_t length,
                                 void* inst,
                                 ReadFuncPtr rd,
                                 WriteFuncPtr wr) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);
  assert((intpointer(rd) & idbit) == 0 && (intpointer(wr) & idbit) == 0);

  void* r = (void*)(intpointer(rd) | idbit);
  void* w = (void*)(intpointer(wr) | idbit);

  Page* page = pages + (addr >> pagebits);
  Owner* owner = owners + (addr >> pagebits);
  uint32_t npages = length >> pagebits;

  for (; npages > 0; npages--) {
    if (owner->read == inst) {
      page->read = r;
      if (owner->write == inst)
        page->write = w;
      page->inst = inst;
    } else if (owner->write == inst) {
      page->write = w;
      page->inst = inst;
    }
    page++;
    owner++;
  }
}

// ---------------------------------------------------------------------------
//  �o���N�A�N�Z�X�̃E�F�C�g��ݒ肷��
//
inline void MemoryBus::SetWait(uint32_t addr, uint32_t wait) {
  pages[addr >> pagebits].wait = wait;
}

// ---------------------------------------------------------------------------
//  �����̃o���N�ɑ΂���E�F�C�g��ݒ肷��
//  npages �͌Œ�̕����D�܂�������
//
inline void MemoryBus::SetWaits(uint32_t addr, uint32_t length, uint32_t wait) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);

  Page* page = pages + (addr >> pagebits);
  int npages = length >> pagebits;

  if (!(npages & 3) || npages >= 16) {
    for (int i = npages & 3; i > 0; i--)
      (page++)->wait = wait;

    for (npages >>= 2; npages > 0; npages--) {
      page[0].wait = wait;
      page[1].wait = wait;
      page[2].wait = wait;
      page[3].wait = wait;
      page += 4;
    }
  } else {
    for (; npages > 0; npages--)
      (page++)->wait = wait;
  }
}

// ---------------------------------------------------------------------------
//  �y�[�W�ɑ΂��ď��L����ݒ肷��
//
inline void MemoryBus::SetReadOwner(uint32_t addr,
                                    uint32_t length,
                                    void* inst) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);

  Owner* owner = owners + (addr >> pagebits);
  int npages = length >> pagebits;

  for (; npages > 0; npages--)
    (owner++)->read = inst;
}

// ---------------------------------------------------------------------------
//  �y�[�W�ɑ΂��ď��L����ݒ肷��
//
inline void MemoryBus::SetWriteOwner(uint32_t addr,
                                     uint32_t length,
                                     void* inst) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);

  Owner* owner = owners + (addr >> pagebits);
  int npages = length >> pagebits;

  for (; npages > 0; npages--)
    (owner++)->write = inst;
}

// ---------------------------------------------------------------------------
//  �y�[�W�ɑ΂��ď��L����ݒ肷��
//
inline void MemoryBus::SetOwner(uint32_t addr, uint32_t length, void* inst) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);

  Owner* owner = owners + (addr >> pagebits);
  int npages = length >> pagebits;

  for (; npages > 0; npages--) {
    owner->read = owner->write = inst;
    owner++;
  }
}

// ---------------------------------------------------------------------------
//  �������ɑ΂��鏑������
//
inline void MemoryBus::Write8(uint32_t addr, uint32_t data) {
  Page* page = &pages[addr >> pagebits];
  if (!(intpointer(page->write) & idbit))
    ((uint8_t*)page->write)[addr & pagemask] = data;
  else
    (*WriteFuncPtr(intpointer(page->write) & ~idbit))(page->inst, addr, data);
}

// ---------------------------------------------------------------------------
//  ����������̓ǂݍ���
//
inline uint32_t MemoryBus::Read8(uint32_t addr) {
  Page* page = &pages[addr >> pagebits];
  if (!(intpointer(page->read) & idbit))
    return ((uint8_t*)page->read)[addr & pagemask];
  else
    return (*ReadFuncPtr(intpointer(page->read) & ~idbit))(page->inst, addr);
}

// ---------------------------------------------------------------------------
//  �y�[�W�e�[�u���̎擾
//
inline const MemoryBus::Page* MemoryBus::GetPageTable() {
  return pages;
}

#endif  // core_device_i_h
