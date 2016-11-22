//  $Id: piccolo.h,v 1.2 2002/05/31 09:45:22 cisc Exp $

#ifndef incl_romeo_piccolo_h
#define incl_romeo_piccolo_h

#include "win32/types.h"
#include "win32/timekeep.h"
#include "win32/critsect.h"

//  �x�����M�Ή� ROMEO �h���C�o
//
class PiccoloChip {
 public:
  virtual int Init(uint32_t c) = 0;
  virtual void Reset() = 0;
  virtual bool SetReg(uint32_t at, uint32_t addr, uint32_t data) = 0;
  virtual void SetChannelMask(uint32_t mask) = 0;
  virtual void SetVolume(int ch, int value) = 0;
};

enum PICCOLO_CHIPTYPE {
  PICCOLO_INVALID = 0,
  PICCOLO_YMF288,
};

enum PICCOLO_ERROR {
  PICCOLO_SUCCESS = 0,
  PICCOLOE_UNKNOWN = -32768,
  PICCOLOE_DLL_NOT_FOUND,
  PICCOLOE_ROMEO_NOT_FOUND,
  PICCOLOE_HARDWARE_NOT_AVAILABLE,
  PICCOLOE_HARDWARE_IN_USE,
  PICCOLOE_TIME_OUT_OF_RANGE,
  PICCOLOE_THREAD_ERROR,
};

class Piccolo {
 public:
  ~Piccolo();

  static Piccolo* GetInstance();

  // �x���o�b�t�@�̃T�C�Y��ݒ�
  bool SetLatencyBufferSize(uint32_t entry);

  // �x�����Ԃ̍ő�l��ݒ�
  // SetReg ���Ăяo���ꂽ�Ƃ��Ananosec ��ȍ~�̃��W�X�^�������݂��w������ at
  // �̒l���w�肵���ꍇ
  // �Ăяo���͋p������邩������Ȃ��B
  bool SetMaximumLatency(uint32_t nanosec);

  // ���\�b�h�Ăяo�����_�ł̎��Ԃ�n��(�P�ʂ� nanosec)
  uint32_t GetCurrentTime();

  //
  int GetChip(PICCOLO_CHIPTYPE type, PiccoloChip** pc);

  bool IsDriverBased() { return !islegacy; }

 private:
  struct Driver {
    virtual bool IsAvailable() = 0;
    virtual void Reserve(bool r) = 0;
    virtual void Reset() = 0;
    virtual void Set(uint32_t a, uint32_t d) = 0;
  };

  class ChipIF : public PiccoloChip {
   public:
    ChipIF(Piccolo* p, Driver* d) { pic = p, drv = d; }
    ~ChipIF() { pic->DrvRelease(drv); }

    int Init(uint32_t param) { return pic->DrvInit(drv, param); }
    void Reset() { pic->DrvReset(drv); }
    bool SetReg(uint32_t at, uint32_t addr, uint32_t data) {
      return pic->DrvSetReg(drv, at, addr, data);
    }
    void SetChannelMask(uint32_t mask) {}
    void SetVolume(int ch, int value) {}

   private:
    Piccolo* pic;
    Driver* drv;
  };
  friend class ChipIF;

  class YMF288 : public Driver {
   public:
    bool Init(Piccolo* p, uint32_t _addr) {
      piccolo = p;
      addr = _addr;
      used = false;
      return true;
    }
    bool IsAvailable() { return !used; }

    void Reserve(bool r) { used = r; }
    void Reset();
    void Set(uint32_t a, uint32_t d);
    void Mute();

   private:
    enum {
      ADDR0 = 0x00,
      DATA0 = 0x04,
      ADDR1 = 0x08,
      DATA1 = 0x0c,
      CTRL = 0x1c,
    };
    bool IsBusy();
    Piccolo* piccolo;
    uint32_t addr;
    bool used;
  };
  friend class YMF288;

  struct Event {
    Driver* drv;
    uint32_t at;
    uint32_t addr;
    uint32_t data;
  };

  static Piccolo piccolo;

  Piccolo();
  int Init();
  void Cleanup();
  static uint32_t CALLBACK ThreadEntry(void* arg);
  uint32_t ThreadMain();

  TimeKeeper timekeeper;
  CriticalSection cs;

  bool Push(Event&);
  Event* Top();
  void Pop();

  bool DrvSetReg(Driver* drv, uint32_t at, uint32_t addr, uint32_t data);
  int DrvInit(Driver* drv, uint32_t c);
  void DrvReset(Driver* drv);
  void DrvRelease(Driver* drv);

  YMF288 ymf288;

  Event* events;
  int evread;
  int evwrite;
  int eventries;

  int maxlatency;

  uint32_t addr;
  uint32_t irq;
  int avail;

  volatile bool shouldterminate;
  volatile bool active;
  HANDLE hthread;
  uint32_t idthread;

  HANDLE hfile;
  bool islegacy;
};

#endif  // incl_romeo_piccolo_h
