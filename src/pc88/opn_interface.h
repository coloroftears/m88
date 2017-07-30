// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//  $Id: opnif.h,v 1.19 2003/09/28 14:35:35 cisc Exp $

#pragma once

#include "common/device.h"
#include "devices/opna.h"

class PC88;
class Scheduler;

namespace pc88core {
class Config;
// 88 用の OPN Unit
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
    kBaseClock = 7987200,
  };

 public:
  explicit OPNInterface(const ID& id);
  ~OPNInterface();

  bool Init(IOBus* bus, int intrport, int port, Scheduler* s);
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

  void Enable(bool enable) { enable_ = enable; }
  void SetOPNMode(bool _opna) { opnamode_ = _opna; }
  const uint8_t* GetRegs() { return regs_; }
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
  class OPNUnit final : public fmgen::OPNA {
   public:
    OPNUnit() {}
    ~OPNUnit() override {}

    void Intr(bool f) override;

    void SetIntr(IOBus* b, int p) { bus_ = b, pintr_ = p; }
    void SetIntrMask(bool e);
    uint32_t IntrStat() {
      return (interrupt_enabled_ ? 1 : 0) | (interrup_pending_ ? 2 : 0);
    }

   private:
    IOBus* bus_ = nullptr;
    int pintr_;
    bool interrupt_enabled_;
    bool interrup_pending_;

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
  OPNUnit opn_;

  ISoundControl* soundcontrol_ = nullptr;
  IOBus* bus_ = nullptr;
  Scheduler* scheduler_ = nullptr;

  uint32_t imaskport_ = 0;
  int imaskbit_ = 0;
  int prevtime_ = 0;
  int portio_ = 0;
  uint32_t currentrate_ = 0;

  uint32_t clock = 0;

  bool fmmixmode_ = true;
  bool opnamode_ = false;
  bool enable_ = false;

  uint32_t index0_;
  uint32_t index1_;
  uint32_t data1_;

  uint8_t regs_[0x200];

  static int prescaler_;

  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};

inline void OPNInterface::SetChannelMask(uint32_t ch) {
  opn_.SetChannelMask(ch);
}
}  // namespace pc88core
