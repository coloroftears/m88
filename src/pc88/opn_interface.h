// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//  $Id: opnif.h,v 1.19 2003/09/28 14:35:35 cisc Exp $

#pragma once

#include "common/device.h"
#include "devices/opna.h"

// ---------------------------------------------------------------------------

class PC88;
class Scheduler;

//#define USE_OPN

namespace pc88core {
class Config;
// ---------------------------------------------------------------------------
//  88 用の OPN Unit
//
class OPNInterface final : public Device, public ISoundSource {
 public:
  enum IDFunc {
    reset = 0,
    setindex0,
    setindex1,
    writedata0,
    writedata1,
    setintrmask,
    sync,
    readstatus = 0,
    readstatusex,
    readdata0,
    readdata1,
  };
  enum {
#ifdef USE_OPN
    baseclock = 3993600,
#else
    baseclock = 7987200,
#endif
  };

 public:
  explicit OPNInterface(const ID& id);
  ~OPNInterface();

  bool Init(IOBus* bus, int intrport, int io, Scheduler* s);
  void SetIMask(uint32_t port, uint32_t bit);

  // Overrides ISoundSource.
  bool IFCALL Connect(ISoundControl* c) final;
  bool IFCALL SetRate(uint32_t rate) final;
  void IFCALL Mix(int32_t* buffer, int nsamples) final;

  void SetVolume(const Config* config);
  void SetFMMixMode(bool);

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final;
  bool IFCALL SaveStatus(uint8_t* status) final;
  bool IFCALL LoadStatus(const uint8_t* status) final;

  void Enable(bool en) { enable = en; }
  void SetOPNMode(bool _opna) { opnamode = _opna; }
  const uint8_t* GetRegs() { return regs; }
  void SetChannelMask(uint32_t ch);

  void IOCALL SetIntrMask(uint32_t, uint32_t intrmask);
  void IOCALL Reset(uint32_t = 0, uint32_t = 0);
  void IOCALL SetIndex0(uint32_t, uint32_t data);
  void IOCALL SetIndex1(uint32_t, uint32_t data);
  void IOCALL WriteData0(uint32_t, uint32_t data);
  void IOCALL WriteData1(uint32_t, uint32_t data);
  uint32_t IOCALL ReadData0(uint32_t);
  uint32_t IOCALL ReadData1(uint32_t);
  uint32_t IOCALL ReadStatus(uint32_t);
  uint32_t IOCALL ReadStatusEx(uint32_t);
  void IOCALL Sync(uint32_t, uint32_t);

 private:
  class OPNUnit final :
#ifndef USE_OPN
      public fmgen::OPNA
#else
      public fmgen::OPN
#endif
  {
   public:
    OPNUnit() : bus(nullptr) {}
    ~OPNUnit() override {}

    void Intr(bool f) override;

    void SetIntr(IOBus* b, int p) { bus = b, pintr = p; }
    void SetIntrMask(bool e);
    uint32_t IntrStat() {
      return (intrenabled ? 1 : 0) | (intrpending ? 2 : 0);
    }

   private:
    IOBus* bus;
    int pintr;
    bool intrenabled;
    bool intrpending;

    friend class OPNInterface;
  };

  enum {
    ssrev = 3,
  };
  struct Status {
    uint8_t rev;
    uint8_t i0, i1, d0, d1;
    uint8_t is;
    uint8_t regs[0x200];
  };

 private:
  void UpdateTimer();
  void IOCALL TimeEvent(uint32_t);
  OPNUnit opn;
  ISoundControl* soundcontrol;
  IOBus* bus;
  Scheduler* scheduler;
  uint32_t imaskport;
  int imaskbit;
  int prevtime;
  int portio;
  uint32_t currentrate;
  bool fmmixmode;

  uint32_t clock;

  bool opnamode;
  bool enable;

  uint32_t index0;
  uint32_t index1;
  uint32_t data1;

  int delay;

  uint8_t regs[0x200];

  static int prescaler;

  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};

inline void OPNInterface::SetChannelMask(uint32_t ch) {
  opn.SetChannelMask(ch);
}
}  // namespace pc88core
