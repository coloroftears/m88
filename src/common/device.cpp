// ---------------------------------------------------------------------------
//  Virtual Bus Implementation
//  Copyright (c) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: device.cpp,v 1.21 2000/06/20 23:53:03 cisc Exp $

#include "common/device.h"

#include <string.h>

//#define LOGNAME "membus"
#include "common/diag.h"

// ---------------------------------------------------------------------------
//  Memory Bus
//  構築・廃棄
//
MemoryBus::MemoryBus() {}

MemoryBus::~MemoryBus() {}

// ---------------------------------------------------------------------------
//  初期化
//  arg:    npages  バンク数
//          _pages  Page 構造体の array (外部で用意する場合)
//                  省略時は MemoryBus で用意
//
bool MemoryBus::Init(uint32_t npages, Page* _pages) {
  owners.reset();

  if (_pages) {
    pages.reset(_pages);
    ownpages_ = false;
  } else {
    pages.reset(new Page[npages]);
    if (!pages)
      return false;
    ownpages_ = true;
  }
  owners.reset(new Owner[npages]);
  if (!owners)
    return false;

  memset(owners.get(), 0, sizeof(Owner) * npages);

  for (Page *b = pages.get(); npages > 0; npages--, b++) {
    b->read = (void*)(intptr_t(rddummy));
    b->write = (void*)(intptr_t(wrdummy));
    b->inst = nullptr;
    b->wait = 0;
  }
  return true;
}

// ---------------------------------------------------------------------------
//  ダミー入出力関数
//
uint32_t MEMCALL MemoryBus::rddummy(void*, uint32_t addr) {
  LOG2("bus: Read on undefined memory page 0x%x. (addr:0x%.4x)\n",
       addr >> pagebits, addr);
  return 0xff;
}

void MEMCALL MemoryBus::wrdummy(void*, uint32_t addr, uint32_t data) {
  LOG3("bus: Write on undefined memory page 0x%x, (addr:0x%.4x data:0x%.2x)\n",
       addr >> pagebits, addr, data);
}

// ---------------------------------------------------------------------------
//  IO Bus
//
IOBus::DummyIO IOBus::dummyio;

IOBus::IOBus() {}

IOBus::~IOBus() {
  for (uint32_t i = 0; i < banksize_; i++) {
    for (InBank* ib = ins_[i].next; ib;) {
      InBank* nxt = ib->next;
      delete ib;
      ib = nxt;
    }
    for (OutBank* ob = outs_[i].next; ob;) {
      OutBank* nxt = ob->next;
      delete ob;
      ob = nxt;
    }
  }
}

//  初期化
bool IOBus::Init(uint32_t nbanks, DeviceList* dl) {
  devlist_ = dl;

  banksize_ = 0;
  ins_.reset(new InBank[nbanks]);
  outs_.reset(new OutBank[nbanks]);
  flags_.reset(new uint8_t[nbanks]);
  if (!ins_ || !outs_ || !flags_)
    return false;
  banksize_ = nbanks;

  memset(flags_.get(), 0, nbanks);

  for (uint32_t i = 0; i < nbanks; i++) {
    ins_[i].device = &dummyio;
    ins_[i].func = static_cast<InFuncPtr>(&DummyIO::dummyin);
    ins_[i].next = nullptr;
    outs_[i].device = &dummyio;
    outs_[i].func = static_cast<OutFuncPtr>(&DummyIO::dummyout);
    outs_[i].next = nullptr;
  }

  return true;
}

//  デバイス接続
bool IOBus::Connect(IDevice* device, const Connector* connector) {
  if (devlist_)
    devlist_->Add(device);

  const IDevice::Descriptor* desc = device->GetDesc();

  for (; connector->rule; connector++) {
    switch (connector->rule & 3) {
      case portin:
        if (!ConnectIn(connector->bank, device, desc->indef[connector->id]))
          return false;
        break;

      case portout:
        if (!ConnectOut(connector->bank, device, desc->outdef[connector->id]))
          return false;
        break;
    }
    if (connector->rule & sync)
      flags_[connector->bank] = 1;
  }
  return true;
}

bool IOBus::ConnectIn(uint32_t bank, IDevice* device, InFuncPtr func) {
  InBank* i = &ins_[bank];
  if (i->func == &DummyIO::dummyin) {
    // 最初の接続
    i->device = device;
    i->func = func;
  } else {
    // 2回目以降の接続
    InBank* j = new InBank;
    if (!j)
      return false;
    j->device = device;
    j->func = func;
    j->next = i->next;
    i->next = j;
  }
  return true;
}

bool IOBus::ConnectOut(uint32_t bank, IDevice* device, OutFuncPtr func) {
  OutBank* i = &outs_[bank];
  if (i->func == &DummyIO::dummyout) {
    // 最初の接続
    i->device = device;
    i->func = func;
  } else {
    // 2回目以降の接続
    OutBank* j = new OutBank;
    if (!j)
      return false;
    j->device = device;
    j->func = func;
    j->next = i->next;
    i->next = j;
  }
  return true;
}

bool IOBus::Disconnect(IDevice* device) {
  if (devlist_)
    devlist_->Del(device);

  uint32_t i;
  for (i = 0; i < banksize_; i++) {
    InBank* current = &ins_[i];
    InBank* referer = nullptr;
    while (current) {
      InBank* next = current->next;
      if (current->device == device) {
        if (referer) {
          referer->next = next;
          delete current;
        } else {
          // 削除するべきアイテムが最初にあった場合
          if (next) {
            // 次のアイテムの内容を複写して削除
            *current = *next;
            referer = nullptr;
            delete next;
            continue;
          } else {
            // このアイテムが唯一のアイテムだった場合
            current->func = static_cast<InFuncPtr>(&DummyIO::dummyin);
          }
        }
      }
      current = next;
    }
  }

  for (i = 0; i < banksize_; i++) {
    OutBank* current = &outs_[i];
    OutBank* referer = nullptr;
    while (current) {
      OutBank* next = current->next;
      if (current->device == device) {
        if (referer) {
          referer->next = next;
          delete current;
        } else {
          // 削除するべきアイテムが最初にあった場合
          if (next) {
            // 次のアイテムの内容を複写して削除
            *current = *next;
            referer = nullptr;
            delete next;
            continue;
          } else {
            // このアイテムが唯一のアイテムだった場合
            current->func = static_cast<OutFuncPtr>(&DummyIO::dummyout);
          }
        }
      }
      current = next;
    }
  }
  return true;
}

uint32_t IOBus::In(uint32_t port) {
  InBank* list = &ins_[port >> iobankbits];

  uint32_t data = 0xff;
  do {
    data &= (list->device->*list->func)(port);
    list = list->next;
  } while (list);
  return data;
}

void IOBus::Out(uint32_t port, uint32_t data) {
  OutBank* list = &outs_[port >> iobankbits];
  do {
    (list->device->*list->func)(port, data);
    list = list->next;
  } while (list);
}

uint32_t IOCALL IOBus::DummyIO::dummyin(uint32_t) {
  return IOBus::Active(0xff, 0xff);
}

void IOCALL IOBus::DummyIO::dummyout(uint32_t, uint32_t) {
  return;
}

// ---------------------------------------------------------------------------
//  DeviceList
//  状態保存・復帰の対象となるデバイスのリストを管理する．
//
DeviceList::~DeviceList() {
  Cleanup();
}

// ---------------------------------------------------------------------------
//  リストをすべて破棄
//
void DeviceList::Cleanup() {
  nodes_.clear();
}

// ---------------------------------------------------------------------------
//  リストにデバイスを登録
//
bool DeviceList::Add(IDevice* device) {
  ID id = device->GetID();
  if (!id)
    return false;

  if (Node* n = FindNode(id))
    ++n->count;
  else
    nodes_.emplace(id, Node(device));
  return true;
}

// ---------------------------------------------------------------------------
//  リストからデバイスを削除
//
bool DeviceList::Del(const ID id) {
  if (FindNode(id)) {
    nodes_.erase(id);
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
//  指定された識別子を持つデバイスをリスト中から探す
//
IDevice* DeviceList::Find(const ID id) const {
  Node* n = FindNode(id);
  return n ? n->entry : nullptr;
}

// ---------------------------------------------------------------------------
//  指定された識別子を持つデバイスノードを探す
//
DeviceList::Node* DeviceList::FindNode(const ID id) const {
  auto pos = nodes_.find(id);
  if (pos == nodes_.end())
    return nullptr;

  return const_cast<Node*>(&pos->second);
}

// ---------------------------------------------------------------------------
//  状態保存に必要なデータサイズを求める
//
uint32_t DeviceList::GetStatusSize() const {
  uint32_t size = sizeof(Header);
  for (auto& n : nodes_) {
    if (int ds = n.second.entry->GetStatusSize())
      size += sizeof(Header) + ((ds + 3) & ~3);
  }
  return size;
}

// ---------------------------------------------------------------------------
//  状態保存を行う
//  data にはあらかじめ GetStatusSize() で取得したサイズのバッファが必要
//
bool DeviceList::SaveStatus(uint8_t* data) {
  for (auto& n : nodes_) {
    if (uint32_t s = n.second.entry->GetStatusSize()) {
      Header* header = reinterpret_cast<Header*>(data);
      header->id = n.second.entry->GetID();
      header->size = s;
      data += sizeof(Header);
      n.second.entry->SaveStatus(data);
      data += (s + 3) & ~3;
    }
  }
  Header* header = reinterpret_cast<Header*>(data);
  header->id = 0;
  header->size = 0;
  return true;
}

// ---------------------------------------------------------------------------
//  SaveStatus で保存した状態から復帰する．
//
bool DeviceList::LoadStatus(const uint8_t* data) {
  if (!CheckStatus(data))
    return false;

  while (true) {
    const Header* header = reinterpret_cast<const Header*>(data);
    data += sizeof(Header);
    if (!header->id)
      break;

    IDevice* dev = Find(header->id);
    if (dev) {
      if (!dev->LoadStatus(data))
        return false;
    }
    data += (header->size + 3) & ~3;
  }
  return true;
}

// ---------------------------------------------------------------------------
//  状態データが現在の構成で適応可能か調べる
//  具体的にはサイズチェックだけ．
//
bool DeviceList::CheckStatus(const uint8_t* data) const {
  while (true) {
    const Header* header = reinterpret_cast<const Header*>(data);
    data += sizeof(Header);
    if (!header->id)
      break;

    IDevice* dev = Find(header->id);
    if (dev && dev->GetStatusSize() != header->size)
      return false;
    data += (header->size + 3) & ~3;
  }
  return true;
}
