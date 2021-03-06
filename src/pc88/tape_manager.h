// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1997, 2000.
// ---------------------------------------------------------------------------
//  $Id: tapemgr.h,v 1.2 2000/06/26 14:05:30 cisc Exp $

#pragma once

#include "common/device.h"

class Scheduler;
class SchedulerEvent;

namespace pc88core {

class TapeManager final : public Device {
 public:
  enum {
    requestdata = 0,
    out30,
    in40 = 0,
  };

  TapeManager();
  ~TapeManager();

  bool Init(Scheduler* s, IOBus* bus, int pin);

  bool Open(const char* file);
  bool Close();
  bool Rewind(bool timer = true);

  bool IsOpen() { return !!tags_; }

  bool Motor(bool on);
  bool Carrier();

  bool Seek(uint32_t pos, uint32_t offset);
  uint32_t GetPos();

  uint32_t ReadByte();
  void IOCALL RequestData(uint32_t = 0, uint32_t = 0);

  void IOCALL Out30(uint32_t, uint32_t en);
  uint32_t IOCALL In40(uint32_t);

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final;
  bool IFCALL SaveStatus(uint8_t* status) final;
  bool IFCALL LoadStatus(const uint8_t* status) final;

 private:
  enum {
    T88VER = 0x100,
    SSREV = 1,
  };
  enum Mode {
    T_END = 0,
    T_VERSION = 1,
    T_BLANK = 0x100,
    T_DATA = 0x101,
    T_SPACE = 0x102,
    T_MARK = 0x103,
  };
  struct TagHdr {
    uint16_t id;
    uint16_t length;
  };
  struct Tag {
    Tag* next;
    Tag* prev;
    uint16_t id;
    uint16_t length;
    uint8_t data[1];
  };
  struct BlankTag {
    uint32_t pos;
    uint32_t tick;
  };
  struct DataTag {
    uint32_t pos;
    uint32_t tick;
    uint16_t length;
    uint16_t type;
    uint8_t data[1];
  };
  struct Status {
    uint8_t rev;
    bool motor;
    uint32_t pos;
    uint32_t offset;
  };

  void Proceed(bool timer = true);
  void IOCALL Timer(uint32_t = 0);
  void Send(uint32_t);
  void SetTimer(int t);

  Scheduler* scheduler_;
  SchedulerEvent* event_ = nullptr;
  Tag* tags_;
  Tag* pos_;
  int offset_;
  uint32_t tick_;  // per 4800bps
  Mode mode_;
  SchedTime time_;        // motor on: タイマー開始時間
  uint32_t timercount_;   // per 4800bps
  uint32_t timerremain_;  // タイマー残り (per 4800bps)
  bool motor_;

  IOBus* bus_;
  int pinput_;

  uint8_t* data_;
  int datasize_;
  int datatype_;

 private:
  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};

}  // namespace pc88core
