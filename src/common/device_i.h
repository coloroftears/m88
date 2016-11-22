// ---------------------------------------------------------------------------
//  Virtual Bus Implementation
//  Copyright (c) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: device_i.h,v 1.9 1999/08/26 08:05:55 cisc Exp $

#pragma once

#include <assert.h>

// ---------------------------------------------------------------------------
//  MemoryBus inline funcitions

// ---------------------------------------------------------------------------
//  バンク書き込みにメモリを割り当てる
//
inline void MemoryBus::SetWriteMemory(uint32_t addr, void* ptr) {
  assert((uint32_t(ptr) & idbit) == 0 && (addr & pagemask) == 0);
  pages[addr >> pagebits].write = ptr;
}

// ---------------------------------------------------------------------------
//  バンク読み込みにメモリを割り当てる
//
inline void MemoryBus::SetReadMemory(uint32_t addr, void* ptr) {
  assert((uint32_t(ptr) & idbit) == 0 && (addr & pagemask) == 0);
  pages[addr >> pagebits].read = ptr;
}

// ---------------------------------------------------------------------------
//  バンク読み書きにメモリを割り当てる
//
inline void MemoryBus::SetMemory(uint32_t addr, void* ptr) {
  assert((uint32_t(ptr) & idbit) == 0 && (addr & pagemask) == 0);
  Page* page = &pages[addr >> pagebits];
  page->read = ptr;
  page->write = ptr;
}

// ---------------------------------------------------------------------------
//  バンク読み書きに関数を割り当てる
//
inline void MemoryBus::SetFunc(uint32_t addr,
                               void* inst,
                               ReadFuncPtr rd,
                               WriteFuncPtr wr) {
  assert((addr & pagemask) == 0);
  assert((intptr_t(rd) & idbit) == 0 && (intptr_t(wr) & idbit) == 0);
  Page* page = &pages[addr >> pagebits];
  page->read = (void*)(intptr_t(rd) | idbit);
  page->write = (void*)(intptr_t(wr) | idbit);
  page->inst = inst;
}

// ---------------------------------------------------------------------------
//  複数のバンク書き込みに連続したメモリを割り当てる
//  npages は固定の方が好ましいかも
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
//  複数のバンク書き込みに連続したメモリを割り当てる
//  所有者チェック付き
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
//  複数のバンク読み込みに連続したメモリを割り当てる
//  npages は固定の方が好ましいかも
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
//  複数のバンク読み込みに連続したメモリを割り当てる
//  npages は固定の方が好ましいかも
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
//  複数のバンク読み書きに連続したメモリを割り当てる
//  npages は固定の方が好ましいかも
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
//  複数のバンク読み書きに連続したメモリを割り当てる
//  npages は固定の方が好ましいかも
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
//  複数のバンク読み書きに関数を割り当てる
//  npages は固定の方が好ましいかも
//
inline void MemoryBus::SetFuncs(uint32_t addr,
                                uint32_t length,
                                void* inst,
                                ReadFuncPtr rd,
                                WriteFuncPtr wr) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);
  assert((intptr_t(rd) & idbit) == 0 && (intptr_t(wr) & idbit) == 0);

  void* r = (void*)(intptr_t(rd) | idbit);
  void* w = (void*)(intptr_t(wr) | idbit);

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
//  複数のバンク読み書きに関数を割り当てる
//
inline void MemoryBus::SetFuncs2(uint32_t addr,
                                 uint32_t length,
                                 void* inst,
                                 ReadFuncPtr rd,
                                 WriteFuncPtr wr) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);
  assert((intptr_t(rd) & idbit) == 0 && (intptr_t(wr) & idbit) == 0);

  void* r = (void*)(intptr_t(rd) | idbit);
  void* w = (void*)(intptr_t(wr) | idbit);

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
//  バンクアクセスのウェイトを設定する
//
inline void MemoryBus::SetWait(uint32_t addr, uint32_t wait) {
  pages[addr >> pagebits].wait = wait;
}

// ---------------------------------------------------------------------------
//  複数のバンクに対するウェイトを設定する
//  npages は固定の方が好ましいかも
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
//  ページに対して所有権を設定する
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
//  ページに対して所有権を設定する
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
//  ページに対して所有権を設定する
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
//  メモリに対する書き込み
//
inline void MemoryBus::Write8(uint32_t addr, uint32_t data) {
  Page* page = &pages[addr >> pagebits];
  if (!(intptr_t(page->write) & idbit))
    ((uint8_t*)page->write)[addr & pagemask] = data;
  else
    (*WriteFuncPtr(intptr_t(page->write) & ~idbit))(page->inst, addr, data);
}

// ---------------------------------------------------------------------------
//  メモリからの読み込み
//
inline uint32_t MemoryBus::Read8(uint32_t addr) {
  Page* page = &pages[addr >> pagebits];
  if (!(intptr_t(page->read) & idbit))
    return ((uint8_t*)page->read)[addr & pagemask];
  else
    return (*ReadFuncPtr(intptr_t(page->read) & ~idbit))(page->inst, addr);
}

// ---------------------------------------------------------------------------
//  ページテーブルの取得
//
inline const MemoryBus::Page* MemoryBus::GetPageTable() {
  return pages;
}
