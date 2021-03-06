// ---------------------------------------------------------------------------
//  Virtual Bus Implementation
//  Copyright (c) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: device.h,v 1.21 1999/12/28 10:33:53 cisc Exp $

// ---------------------------------------------------------------------------
//  Virtual Bus Implementation
//  Copyright (c) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: device_i.h,v 1.9 1999/08/26 08:05:55 cisc Exp $

#pragma once

#include <assert.h>
#include <stdint.h>

#include <memory>
#include <unordered_map>

#include "interface/ifcommon.h"

// ---------------------------------------------------------------------------
//  Device
//
class Device : public IDevice {
 public:
  explicit Device(const ID& id) : id_(id) {}
  virtual ~Device() {}

  // Overrides IDevice.
  const ID& IFCALL GetID() const final { return id_; }
  const Descriptor* IFCALL GetDesc() const override { return nullptr; }
  uint32_t IFCALL GetStatusSize() override { return 0; }
  bool IFCALL LoadStatus(const uint8_t*) override { return false; }
  bool IFCALL SaveStatus(uint8_t*) override { return false; }

 protected:
  void SetID(const ID& i) { id_ = i; }

 private:
  ID id_;
};

// ---------------------------------------------------------------------------
//  MemoryBus
//  メモリ空間とアクセス手段を提供するクラス
//  バンクごとに実メモリかアクセス関数を割り当てることができる。
//
//  アクセス関数の場合は、関数の引数に渡す識別子をバンクごとに設定できる
//  また、バンクごとにそれぞれウェイトを設定することができる
//  関数と実メモリの識別にはポインタの特定 bit を利用するため、
//  ポインタの少なくとも 1 bit は 0 になってなければならない
//
class MemoryBus final : public IMemoryAccess {
 public:
  using ReadFuncPtr = uint32_t(MEMCALL*)(void* inst, uint32_t addr);
  using WriteFuncPtr = void(MEMCALL*)(void* inst, uint32_t addr, uint32_t data);

  struct Page {
    void* read;
    void* write;
    void* inst;
    int wait;
  };

  struct Owner {
    void* read;
    void* write;
  };

  enum {
    pagebits = 10,
    pagemask = (1 << pagebits) - 1,
  };

 public:
  MemoryBus();
  virtual ~MemoryBus();

  bool Init(uint32_t npages, Page* pages = nullptr);

  void SetWriteMemory(uint32_t addr, void* ptr);
  void SetReadMemory(uint32_t addr, void* ptr);
  void SetMemory(uint32_t addr, void* ptr);

  void SetWriteMemorys(uint32_t addr, uint32_t length, uint8_t* ptr);
  void SetReadMemorys(uint32_t addr, uint32_t length, uint8_t* ptr);
  void SetMemorys(uint32_t addr, uint32_t length, uint8_t* ptr);

  void SetWriteMemorys2(uint32_t addr,
                        uint32_t length,
                        uint8_t* ptr,
                        void* inst);
  void SetReadMemorys2(uint32_t addr,
                       uint32_t length,
                       uint8_t* ptr,
                       void* inst);
  void SetMemorys2(uint32_t addr, uint32_t length, uint8_t* ptr, void* inst);

  void SetReadOwner(uint32_t addr, uint32_t length, void* inst);
  void SetWriteOwner(uint32_t addr, uint32_t length, void* inst);
  void SetOwner(uint32_t addr, uint32_t length, void* inst);

  void SetWait(uint32_t addr, uint32_t wait);
  void SetWaits(uint32_t addr, uint32_t length, uint32_t wait);

  // Overrides IMemoryAccess.
  uint32_t IFCALL Read8(uint32_t addr) override;
  void IFCALL Write8(uint32_t addr, uint32_t data) override;

  const Page* GetPageTable();

 private:
  static void MEMCALL wrdummy(void*, uint32_t, uint32_t);
  static uint32_t MEMCALL rddummy(void*, uint32_t);

  std::unique_ptr<Page[]> pages;
  std::unique_ptr<Owner[]> owners;
  bool ownpages_ = false;
};

// ---------------------------------------------------------------------------

class DeviceList final {
 public:
  using ID = IDevice::ID;

 private:
  struct Node {
    explicit Node(IDevice* d) : entry(d), count(1) {}
    ~Node() {}

    IDevice* entry;
    int count;
  };

  struct Header {
    ID id;
    uint32_t size;
  };

 public:
  DeviceList() {}
  ~DeviceList();

  void Cleanup();
  bool Add(IDevice* t);
  bool Del(IDevice* t) { return t->GetID() ? Del(t->GetID()) : false; }
  bool Del(const ID id);
  IDevice* Find(const ID id) const;

  bool LoadStatus(const uint8_t* status);
  bool SaveStatus(uint8_t* status);
  uint32_t GetStatusSize() const;

 private:
  Node* FindNode(const ID id) const;
  bool CheckStatus(const uint8_t*) const;

  std::unordered_map<ID, Node> nodes_;
};

// ---------------------------------------------------------------------------
//  IO 空間を提供するクラス
//  MemoryBus との最大の違いはひとつのバンクに複数のアクセス関数を
//  設定できること
//
class IOBus : public IIOAccess, public IIOBus {
 public:
  using InFuncPtr = Device::InFuncPtr;
  using OutFuncPtr = Device::OutFuncPtr;

  enum {
    iobankbits = 0,  // 1 バンクのサイズ(ビット数)
  };
  struct InBank {
    IDevice* device;
    InFuncPtr func;
    InBank* next;
  };

  struct OutBank {
    IDevice* device;
    OutFuncPtr func;
    OutBank* next;
  };

 public:
  IOBus();
  virtual ~IOBus();

  bool Init(uint32_t nports, DeviceList* devlist = nullptr);

  bool ConnectIn(uint32_t bank, IDevice* device, InFuncPtr func);
  bool ConnectOut(uint32_t bank, IDevice* device, OutFuncPtr func);

  InBank* GetIns() { return ins_.get(); }
  OutBank* GetOuts() { return outs_.get(); }
  uint8_t* GetFlags() { return flags_.get(); }

  bool IsSyncPort(uint32_t port);

  // Overrides IIOBus.
  bool IFCALL Connect(IDevice* device, const Connector* connector) override;
  bool IFCALL Disconnect(IDevice* device) override;
  // Overrides IIOAccess.
  uint32_t IFCALL In(uint32_t port) override;
  void IFCALL Out(uint32_t port, uint32_t data) override;

  // inactive line is high
  static uint32_t Active(uint32_t data, uint32_t bits) { return data | ~bits; }

 private:
  class DummyIO final : public Device {
   public:
    DummyIO() : Device(0) {}
    ~DummyIO() {}

    uint32_t IOCALL dummyin(uint32_t);
    void IOCALL dummyout(uint32_t, uint32_t);
  };

  struct StatusTag {
    Device::ID id;
    uint32_t size;
  };

 private:
  std::unique_ptr<InBank[]> ins_;
  std::unique_ptr<OutBank[]> outs_;
  std::unique_ptr<uint8_t[]> flags_;
  DeviceList* devlist_ = nullptr;

  uint32_t banksize_ = 0;
  static DummyIO dummyio;
};

#define DEV_ID(a, b, c, d)                                   \
  (Device::ID(a + (uint32_t(b) << 8) + (uint32_t(c) << 16) + \
              (uint32_t(d) << 24)))

// ---------------------------------------------------------------------------

inline bool IOBus::IsSyncPort(uint32_t port) {
  return (flags_[port >> iobankbits] & 1) != 0;
}

// ---------------------------------------------------------------------------
//  MemoryBus inline funcitions

// ---------------------------------------------------------------------------
//  バンク書き込みにメモリを割り当てる
//
inline void MemoryBus::SetWriteMemory(uint32_t addr, void* ptr) {
  assert((addr & pagemask) == 0);
  pages[addr >> pagebits].write = ptr;
}

// ---------------------------------------------------------------------------
//  バンク読み込みにメモリを割り当てる
//
inline void MemoryBus::SetReadMemory(uint32_t addr, void* ptr) {
  assert((addr & pagemask) == 0);
  pages[addr >> pagebits].read = ptr;
}

// ---------------------------------------------------------------------------
//  バンク読み書きにメモリを割り当てる
//
inline void MemoryBus::SetMemory(uint32_t addr, void* ptr) {
  assert((addr & pagemask) == 0);
  Page* page = &pages[addr >> pagebits];
  page->read = ptr;
  page->write = ptr;
}

// ---------------------------------------------------------------------------
//  複数のバンク書き込みに連続したメモリを割り当てる
//  npages は固定の方が好ましいかも
//
inline void MemoryBus::SetWriteMemorys(uint32_t addr,
                                       uint32_t length,
                                       uint8_t* ptr) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);

  Page* page = pages.get() + (addr >> pagebits);
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

  Page* page = pages.get() + (addr >> pagebits);
  Owner* owner = owners.get() + (addr >> pagebits);
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

  Page* page = pages.get() + (addr >> pagebits);
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

  Page* page = pages.get() + (addr >> pagebits);
  Owner* owner = owners.get() + (addr >> pagebits);
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

  Page* page = pages.get() + (addr >> pagebits);
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

  Page* page = pages.get() + (addr >> pagebits);
  Owner* owner = owners.get() + (addr >> pagebits);
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

  Page* page = pages.get() + (addr >> pagebits);
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

  Owner* owner = owners.get() + (addr >> pagebits);
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

  Owner* owner = owners.get() + (addr >> pagebits);
  int npages = length >> pagebits;

  for (; npages > 0; npages--)
    (owner++)->write = inst;
}

// ---------------------------------------------------------------------------
//  ページに対して所有権を設定する
//
inline void MemoryBus::SetOwner(uint32_t addr, uint32_t length, void* inst) {
  assert((addr & pagemask) == 0 && (length & pagemask) == 0);

  Owner* owner = owners.get() + (addr >> pagebits);
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
  ((uint8_t*)page->write)[addr & pagemask] = data;
}

// ---------------------------------------------------------------------------
//  メモリからの読み込み
//
inline uint32_t MemoryBus::Read8(uint32_t addr) {
  Page* page = &pages[addr >> pagebits];
  return ((uint8_t*)page->read)[addr & pagemask];
}

// ---------------------------------------------------------------------------
//  ページテーブルの取得
//
inline const MemoryBus::Page* MemoryBus::GetPageTable() {
  return pages.get();
}
