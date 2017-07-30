// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  カレンダ時計(μPD4990A) のエミュレーション
// ---------------------------------------------------------------------------
//  $Id: calender.cpp,v 1.4 1999/10/10 01:47:04 cisc Exp $
//  ・TP, 1Hz 機能が未実装

#include "pc88/calendar.h"

#include <stdlib.h>
#include <string.h>

#include "common/bcd.h"

//#define LOGNAME "calender"
#include "common/diag.h"

namespace pc88core {

Calendar::Calendar(const ID& id) : Device(id) {
  Reset();
}

Calendar::~Calendar() {}

void IOCALL Calendar::Reset(uint32_t, uint32_t) {
  datain_ = 0;
  dataoutmode_ = 0;
  strobe_ = 0;
  cmd_ = 0x80;
  scmd_ = 0;
  for (int i = 0; i < 6; i++)
    reg[i] = 0;
}

uint32_t IOCALL Calendar::In40(uint32_t) {
  if (dataoutmode_)
    return IOBus::Active((reg[0] & 1) << 4, 0x10);
  else {
    //      SYSTEMTIME st;
    //      GetLocalTime(&st);
    //      return (st.wSecond & 1) << 4;
    return IOBus::Active(0, 0x10);
  }
}

void IOCALL Calendar::Out10(uint32_t, uint32_t data) {
  pcmd_ = data & 7;
  datain_ = (data >> 3) & 1;
}

void IOCALL Calendar::Out40(uint32_t, uint32_t data) {
  uint32_t modified;
  modified = strobe_ ^ data;
  strobe_ = data;
  if (modified & data & 2)
    Command();
  if (modified & data & 4)
    ShiftData();
}

void Calendar::Command() {
  if (pcmd_ == 7)
    cmd_ = scmd_ | 0x80;
  else
    cmd_ = pcmd_;

  Log("Command = %.2x\n", cmd_);
  switch (cmd_ & 15) {
    case 0x00:  // register hold
      hold_ = true;
      dataoutmode_ = false;
      break;

    case 0x01:  // register shift
      hold_ = false;
      dataoutmode_ = true;
      break;

    case 0x02:  // time set
      SetTime();
      hold_ = true;
      dataoutmode_ = true;
      break;

    case 0x03:  // time read
      GetTime();
      hold_ = true;
      dataoutmode_ = false;
      break;
  }
}

void Calendar::ShiftData() {
  if (hold_) {
    if (cmd_ & 0x80) {
      // shift sreg only
      Log("Shift HS %d\n", datain_);
      scmd_ = (scmd_ >> 1) | (datain_ << 3);
    } else {
      Log("Shift HP -\n", datain_);
    }
  } else {
    if (cmd_ & 0x80) {
      reg[0] = (reg[0] >> 1) | (reg[1] << 7);
      reg[1] = (reg[1] >> 1) | (reg[2] << 7);
      reg[2] = (reg[2] >> 1) | (reg[3] << 7);
      reg[3] = (reg[3] >> 1) | (reg[4] << 7);
      reg[4] = (reg[4] >> 1) | (reg[5] << 7);
      reg[5] = (reg[5] >> 1) | (scmd_ << 7);
      scmd_ = (scmd_ >> 1) | (datain_ << 3);
      Log("Shift -S %d\n", datain_);
    } else {
      reg[0] = (reg[0] >> 1) | (reg[1] << 7);
      reg[1] = (reg[1] >> 1) | (reg[2] << 7);
      reg[2] = (reg[2] >> 1) | (reg[3] << 7);
      reg[3] = (reg[3] >> 1) | (reg[4] << 7);
      reg[4] = (reg[4] >> 1) | (datain_ << 7);
      Log("Shift -P %d\n", datain_);
    }
  }
}

void Calendar::GetTime() {
  time_t ct;

  ct = time(&ct) + diff_;

  // Note: localtime() is not thread-safe.
  tm* lt = localtime(&ct);

  reg[5] = NtoBCD(lt->tm_year % 100);
  reg[4] = (lt->tm_mon + 1) * 16 + lt->tm_wday;
  reg[3] = NtoBCD(lt->tm_mday);
  reg[2] = NtoBCD(lt->tm_hour);
  reg[1] = NtoBCD(lt->tm_min);
  reg[0] = NtoBCD(lt->tm_sec);
}

void Calendar::SetTime() {
  time_t ct;
  time(&ct);
  tm* lt = localtime(&ct);

  tm nt;
  nt.tm_year = (cmd_ & 0x80) ? BCDtoN(reg[5]) : lt->tm_year;
  if (nt.tm_year < 70)
    nt.tm_year += 100;
  nt.tm_mon = (reg[4] - 1) >> 4;
  nt.tm_mday = BCDtoN(reg[3]);
  nt.tm_hour = BCDtoN(reg[2]);
  nt.tm_min = BCDtoN(reg[1]);
  nt.tm_sec = BCDtoN(reg[0]);
  nt.tm_isdst = 0;

  time_t at = mktime(&nt);
  diff_ = at - ct;
}

uint32_t IFCALL Calendar::GetStatusSize() {
  return sizeof(Status);
}

bool IFCALL Calendar::SaveStatus(uint8_t* s) {
  Status* st = (Status*)s;
  st->rev = ssrev;
  st->t = time(&st->t) + diff_;
  st->dataoutmode = dataoutmode_;
  st->hold = hold_;
  st->datain = datain_;
  st->strobe = strobe_;
  st->cmd = cmd_;
  st->scmd = scmd_;
  st->pcmd = pcmd_;
  memcpy(st->reg, reg, 6);
  return true;
}

bool IFCALL Calendar::LoadStatus(const uint8_t* s) {
  const Status* st = (const Status*)s;
  if (st->rev != ssrev)
    return false;
  time_t ct;
  diff_ = st->t - time(&ct);
  dataoutmode_ = st->dataoutmode;
  hold_ = st->hold;
  datain_ = st->datain;
  strobe_ = st->strobe;
  cmd_ = st->cmd;
  scmd_ = st->scmd;
  pcmd_ = st->pcmd;
  memcpy(reg, st->reg, 6);
  return true;
}

// device description
const Device::Descriptor Calendar::descriptor = {indef, outdef};

const Device::OutFuncPtr Calendar::outdef[] = {
    static_cast<Device::OutFuncPtr>(&Calendar::Reset),
    static_cast<Device::OutFuncPtr>(&Calendar::Out10),
    static_cast<Device::OutFuncPtr>(&Calendar::Out40),
};

const Device::InFuncPtr Calendar::indef[] = {
    static_cast<Device::InFuncPtr>(&Calendar::In40),
};
}  // namespace pc88core