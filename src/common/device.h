// ---------------------------------------------------------------------------
//  Virtual Bus Implementation
//  Copyright (c) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: device.h,v 1.21 1999/12/28 10:33:53 cisc Exp $

#ifndef common_device_h
#define common_device_h

#include "win32/types.h"
#include "if/ifcommon.h"

// ---------------------------------------------------------------------------
//  Device
//
class Device : public IDevice {
 public:
  Device(const ID& _id) : id(_id) {}
  virtual ~Device() {}

  const ID& IFCALL GetID() const { return id; }
  const Descriptor* IFCALL GetDesc() const { return 0; }
  uint IFCALL GetStatusSize() { return 0; }
  bool IFCALL LoadStatus(const uint8* status) { return false; }
  bool IFCALL SaveStatus(uint8* status) { return false; }

 protected:
  void SetID(const ID& i) { id = i; }

 private:
  ID id;
};

// ---------------------------------------------------------------------------
//  MemoryBus
//  ��������ԂƃA�N�Z�X��i��񋟂���N���X
//  �o���N���ƂɎ����������A�N�Z�X�֐������蓖�Ă邱�Ƃ��ł���B
//
//  �A�N�Z�X�֐��̏ꍇ�́A�֐��̈����ɓn�����ʎq���o���N���Ƃɐݒ�ł���
//  �܂��A�o���N���Ƃɂ��ꂼ��E�F�C�g��ݒ肷�邱�Ƃ��ł���
//  �֐��Ǝ��������̎��ʂɂ̓|�C���^�̓��� bit �𗘗p���邽�߁A
//  �|�C���^�̏��Ȃ��Ƃ� 1 bit �� 0 �ɂȂ��ĂȂ���΂Ȃ�Ȃ�
//
class MemoryBus : public IMemoryAccess {
 public:
  typedef uint(MEMCALL* ReadFuncPtr)(void* inst, uint addr);
  typedef void(MEMCALL* WriteFuncPtr)(void* inst, uint addr, uint data);

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
#ifdef PTR_IDBIT
    idbit = PTR_IDBIT,
#else
    idbit = 0,
#endif
  };

 public:
  MemoryBus();
  ~MemoryBus();

  bool Init(uint npages, Page* pages = 0);

  void SetWriteMemory(uint addr, void* ptr);
  void SetReadMemory(uint addr, void* ptr);
  void SetMemory(uint addr, void* ptr);
  void SetFunc(uint addr, void* inst, ReadFuncPtr rd, WriteFuncPtr wr);

  void SetWriteMemorys(uint addr, uint length, uint8* ptr);
  void SetReadMemorys(uint addr, uint length, uint8* ptr);
  void SetMemorys(uint addr, uint length, uint8* ptr);
  void SetFuncs(uint addr,
                uint length,
                void* inst,
                ReadFuncPtr rd,
                WriteFuncPtr wr);

  void SetWriteMemorys2(uint addr, uint length, uint8* ptr, void* inst);
  void SetReadMemorys2(uint addr, uint length, uint8* ptr, void* inst);
  void SetMemorys2(uint addr, uint length, uint8* ptr, void* inst);
  void SetFuncs2(uint addr,
                 uint length,
                 void* inst,
                 ReadFuncPtr rd,
                 WriteFuncPtr wr);

  void SetReadOwner(uint addr, uint length, void* inst);
  void SetWriteOwner(uint addr, uint length, void* inst);
  void SetOwner(uint addr, uint length, void* inst);

  void SetWait(uint addr, uint wait);
  void SetWaits(uint addr, uint length, uint wait);

  uint IFCALL Read8(uint addr);
  void IFCALL Write8(uint addr, uint data);

  const Page* GetPageTable();

 private:
  static void MEMCALL wrdummy(void*, uint, uint);
  static uint MEMCALL rddummy(void*, uint);

  Page* pages;
  Owner* owners;
  bool ownpages;
};

// ---------------------------------------------------------------------------

class DeviceList {
 public:
  typedef IDevice::ID ID;

 private:
  struct Node {
    IDevice* entry;
    Node* next;
    int count;
  };
  struct Header {
    ID id;
    uint size;
  };

 public:
  DeviceList() { node = 0; }
  ~DeviceList();

  void Cleanup();
  bool Add(IDevice* t);
  bool Del(IDevice* t) { return t->GetID() ? Del(t->GetID()) : false; }
  bool Del(const ID id);
  IDevice* Find(const ID id);

  bool LoadStatus(const uint8*);
  bool SaveStatus(uint8*);
  uint GetStatusSize();

 private:
  Node* FindNode(const ID id);
  bool CheckStatus(const uint8*);

  Node* node;
};

// ---------------------------------------------------------------------------
//  IO ��Ԃ�񋟂���N���X
//  MemoryBus �Ƃ̍ő�̈Ⴂ�͂ЂƂ̃o���N�ɕ����̃A�N�Z�X�֐���
//  �ݒ�ł��邱��
//
class IOBus : public IIOAccess, public IIOBus {
 public:
  typedef Device::InFuncPtr InFuncPtr;
  typedef Device::OutFuncPtr OutFuncPtr;

  enum {
    iobankbits = 0,  // 1 �o���N�̃T�C�Y(�r�b�g��)
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
  ~IOBus();

  bool Init(uint nports, DeviceList* devlist = 0);

  bool ConnectIn(uint bank, IDevice* device, InFuncPtr func);
  bool ConnectOut(uint bank, IDevice* device, OutFuncPtr func);

  InBank* GetIns() { return ins; }
  OutBank* GetOuts() { return outs; }
  uint8* GetFlags() { return flags; }

  bool IsSyncPort(uint port);

  bool IFCALL Connect(IDevice* device, const Connector* connector);
  bool IFCALL Disconnect(IDevice* device);
  uint IFCALL In(uint port);
  void IFCALL Out(uint port, uint data);

  // inactive line is high
  static uint Active(uint data, uint bits) { return data | ~bits; }

 private:
  class DummyIO : public Device {
   public:
    DummyIO() : Device(0) {}
    ~DummyIO() {}

    uint IOCALL dummyin(uint);
    void IOCALL dummyout(uint, uint);
  };
  struct StatusTag {
    Device::ID id;
    uint32 size;
  };

 private:
  InBank* ins;
  OutBank* outs;
  uint8* flags;
  DeviceList* devlist;

  uint banksize;
  static DummyIO dummyio;
};

// ---------------------------------------------------------------------------
//  Bus
//
/*
class Bus : public MemoryBus, public IOBus
{
public:
    Bus() {}
    ~Bus() {}

    bool Init(uint nports, uint npages, Page* pages = 0);
};
*/
// ---------------------------------------------------------------------------

#ifdef ENDIAN_IS_SMALL
#define DEV_ID(a, b, c, d) \
  (Device::ID(a + (uint32(b) << 8) + (uint32(c) << 16) + (uint32(d) << 24)))
#else
#define DEV_ID(a, b, c, d) \
  (Device::ID(d + (uint32(c) << 8) + (uint32(b) << 16) + (uint32(a) << 24)))
#endif

// ---------------------------------------------------------------------------

inline bool IOBus::IsSyncPort(uint port) {
  return (flags[port >> iobankbits] & 1) != 0;
}

#endif  // common_device_h
