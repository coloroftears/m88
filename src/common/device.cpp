// ---------------------------------------------------------------------------
//  Virtual Bus Implementation
//  Copyright (c) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: device.cpp,v 1.21 2000/06/20 23:53:03 cisc Exp $

#include "common/device.h"

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
    ownpages = false;
  } else {
    pages.reset(new Page[npages]);
    if (!pages)
      return false;
    ownpages = true;
  }
  owners.reset(new Owner[npages]);
  if (!owners)
    return false;

  memset(owners.get(), 0, sizeof(Owner) * npages);

  for (Page *b = pages.get(); npages > 0; npages--, b++) {
    b->read = (void*)(intptr_t(rddummy) | idbit);
    b->write = (void*)(intptr_t(wrdummy) | idbit);
    b->inst = 0;
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
  for (uint32_t i = 0; i < banksize; i++) {
    for (InBank* ib = ins[i].next; ib;) {
      InBank* nxt = ib->next;
      delete ib;
      ib = nxt;
    }
    for (OutBank* ob = outs[i].next; ob;) {
      OutBank* nxt = ob->next;
      delete ob;
      ob = nxt;
    }
  }
}

//  初期化
bool IOBus::Init(uint32_t nbanks, DeviceList* dl) {
  devlist = dl;

  banksize = 0;
  ins.reset(new InBank[nbanks]);
  outs.reset(new OutBank[nbanks]);
  flags.reset(new uint8_t[nbanks]);
  if (!ins || !outs || !flags)
    return false;
  banksize = nbanks;

  memset(flags.get(), 0, nbanks);

  for (uint32_t i = 0; i < nbanks; i++) {
    ins[i].device = &dummyio;
    ins[i].func = static_cast<InFuncPtr>(&DummyIO::dummyin);
    ins[i].next = nullptr;
    outs[i].device = &dummyio;
    outs[i].func = static_cast<OutFuncPtr>(&DummyIO::dummyout);
    outs[i].next = nullptr;
  }

  return true;
}

//  デバイス接続
bool IOBus::Connect(IDevice* device, const Connector* connector) {
  if (devlist)
    devlist->Add(device);

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
      flags[connector->bank] = 1;
  }
  return true;
}

bool IOBus::ConnectIn(uint32_t bank, IDevice* device, InFuncPtr func) {
  InBank* i = &ins[bank];
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
  OutBank* i = &outs[bank];
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
  if (devlist)
    devlist->Del(device);

  uint32_t i;
  for (i = 0; i < banksize; i++) {
    InBank* current = &ins[i];
    InBank* referer = 0;
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
            referer = 0;
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

  for (i = 0; i < banksize; i++) {
    OutBank* current = &outs[i];
    OutBank* referer = 0;
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
            referer = 0;
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
  InBank* list = &ins[port >> iobankbits];

  uint32_t data = 0xff;
  do {
    data &= (list->device->*list->func)(port);
    list = list->next;
  } while (list);
  return data;
}

void IOBus::Out(uint32_t port, uint32_t data) {
  OutBank* list = &outs[port >> iobankbits];
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
  Node* n = node;
  while (n) {
    Node* nx = n->next;
    delete n;
    n = nx;
  }
  node = 0;
}

// ---------------------------------------------------------------------------
//  リストにデバイスを登録
//
bool DeviceList::Add(IDevice* t) {
  ID id = t->GetID();
  if (!id)
    return false;

  Node* n = FindNode(id);
  if (n) {
    n->count++;
    return true;
  } else {
    n = new Node;
    if (n) {
      n->entry = t, n->next = node, n->count = 1;
      node = n;
      return true;
    }
    return false;
  }
}

// ---------------------------------------------------------------------------
//  リストからデバイスを削除
//
bool DeviceList::Del(const ID id) {
  for (Node** r = &node; *r; r = &((*r)->next)) {
    if ((*r)->entry->GetID() == id) {
      Node* d = *r;
      if (!--d->count) {
        *r = d->next;
        delete d;
      }
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
//  指定された識別子を持つデバイスをリスト中から探す
//
IDevice* DeviceList::Find(const ID id) {
  Node* n = FindNode(id);
  return n ? n->entry : 0;
}

// ---------------------------------------------------------------------------
//  指定された識別子を持つデバイスノードを探す
//
DeviceList::Node* DeviceList::FindNode(const ID id) {
  for (Node* n = node; n; n = n->next) {
    if (n->entry->GetID() == id)
      return n;
  }
  return 0;
}

// ---------------------------------------------------------------------------
//  状態保存に必要なデータサイズを求める
//
uint32_t DeviceList::GetStatusSize() {
  uint32_t size = sizeof(Header);
  for (Node* n = node; n; n = n->next) {
    int ds = n->entry->GetStatusSize();
    if (ds)
      size += sizeof(Header) + ((ds + 3) & ~3);
  }
  return size;
}

// ---------------------------------------------------------------------------
//  状態保存を行う
//  data にはあらかじめ GetStatusSize() で取得したサイズのバッファが必要
//
bool DeviceList::SaveStatus(uint8_t* data) {
  for (Node* n = node; n; n = n->next) {
    int s = n->entry->GetStatusSize();
    if (s) {
      ((Header*)data)->id = n->entry->GetID();
      ((Header*)data)->size = s;
      data += sizeof(Header);
      n->entry->SaveStatus(data);
      data += (s + 3) & ~3;
    }
  }
  ((Header*)data)->id = 0;
  ((Header*)data)->size = 0;
  return true;
}

// ---------------------------------------------------------------------------
//  SaveStatus で保存した状態から復帰する．
//
bool DeviceList::LoadStatus(const uint8_t* data) {
  if (!CheckStatus(data))
    return false;
  while (1) {
    const Header* hdr = (const Header*)data;
    data += sizeof(Header);
    if (!hdr->id)
      break;

    IDevice* dev = Find(hdr->id);
    if (dev) {
      if (!dev->LoadStatus(data))
        return false;
    }
    data += (hdr->size + 3) & ~3;
  }
  return true;
}

// ---------------------------------------------------------------------------
//  状態データが現在の構成で適応可能か調べる
//  具体的にはサイズチェックだけ．
//
bool DeviceList::CheckStatus(const uint8_t* data) {
  while (1) {
    const Header* hdr = (const Header*)data;
    data += sizeof(Header);
    if (!hdr->id)
      break;

    IDevice* dev = Find(hdr->id);
    if (dev && dev->GetStatusSize() != hdr->size)
      return false;
    data += (hdr->size + 3) & ~3;
  }
  return true;
}
