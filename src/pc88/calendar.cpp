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

#include "common/bcd.h"

//#define LOGNAME "calender"
#include "common/diag.h"

namespace pc88core {

// ---------------------------------------------------------------------------
//  Construct/Destruct
//
Calendar::Calendar(const ID& id) : Device(id) {
  diff = 0;
  Reset();
}

Calendar::~Calendar() {}

// ---------------------------------------------------------------------------
//  入・出力
//
void IOCALL Calendar::Reset(uint32_t, uint32_t) {
  datain = 0;
  dataoutmode = 0;
  strobe = 0;
  cmd = 0x80;
  scmd = 0;
  for (int i = 0; i < 6; i++)
    reg[i] = 0;
}

uint32_t IOCALL Calendar::In40(uint32_t) {
  if (dataoutmode)
    return IOBus::Active((reg[0] & 1) << 4, 0x10);
  else {
    //      SYSTEMTIME st;
    //      GetLocalTime(&st);
    //      return (st.wSecond & 1) << 4;
    return IOBus::Active(0, 0x10);
  }
}

void IOCALL Calendar::Out10(uint32_t, uint32_t data) {
  pcmd = data & 7;
  datain = (data >> 3) & 1;
}

void IOCALL Calendar::Out40(uint32_t, uint32_t data) {
  uint32_t modified;
  modified = strobe ^ data;
  strobe = data;
  if (modified & data & 2)
    Command();
  if (modified & data & 4)
    ShiftData();
}

// ---------------------------------------------------------------------------
//  制御
//
void Calendar::Command() {
  if (pcmd == 7)
    cmd = scmd | 0x80;
  else
    cmd = pcmd;

  Log("Command = %.2x\n", cmd);
  switch (cmd & 15) {
    case 0x00:  // register hold
      hold = true;
      dataoutmode = false;
      break;

    case 0x01:  // register shift
      hold = false;
      dataoutmode = true;
      break;

    case 0x02:  // time set
      SetTime();
      hold = true;
      dataoutmode = true;
      break;

    case 0x03:  // time read
      GetTime();
      hold = true;
      dataoutmode = false;
      break;
  }
}

// ---------------------------------------------------------------------------
//  データシフト
//
void Calendar::ShiftData() {
  if (hold) {
    if (cmd & 0x80) {
      // shift sreg only
      Log("Shift HS %d\n", datain);
      scmd = (scmd >> 1) | (datain << 3);
    } else {
      Log("Shift HP -\n", datain);
    }
  } else {
    if (cmd & 0x80) {
      reg[0] = (reg[0] >> 1) | (reg[1] << 7);
      reg[1] = (reg[1] >> 1) | (reg[2] << 7);
      reg[2] = (reg[2] >> 1) | (reg[3] << 7);
      reg[3] = (reg[3] >> 1) | (reg[4] << 7);
      reg[4] = (reg[4] >> 1) | (reg[5] << 7);
      reg[5] = (reg[5] >> 1) | (scmd << 7);
      scmd = (scmd >> 1) | (datain << 3);
      Log("Shift -S %d\n", datain);
    } else {
      reg[0] = (reg[0] >> 1) | (reg[1] << 7);
      reg[1] = (reg[1] >> 1) | (reg[2] << 7);
      reg[2] = (reg[2] >> 1) | (reg[3] << 7);
      reg[3] = (reg[3] >> 1) | (reg[4] << 7);
      reg[4] = (reg[4] >> 1) | (datain << 7);
      Log("Shift -P %d\n", datain);
    }
  }
}

// ---------------------------------------------------------------------------
//  時間取得
//
void Calendar::GetTime() {
  time_t ct;

  ct = time(&ct) + diff;

  tm* lt = localtime(&ct);

  reg[5] = NtoBCD(lt->tm_year % 100);
  reg[4] = (lt->tm_mon + 1) * 16 + lt->tm_wday;
  reg[3] = NtoBCD(lt->tm_mday);
  reg[2] = NtoBCD(lt->tm_hour);
  reg[1] = NtoBCD(lt->tm_min);
  reg[0] = NtoBCD(lt->tm_sec);
}

// ---------------------------------------------------------------------------
//  時間設定
//
void Calendar::SetTime() {
  time_t ct;
  time(&ct);
  tm* lt = localtime(&ct);

  tm nt;
  nt.tm_year = (cmd & 0x80) ? BCDtoN(reg[5]) : lt->tm_year;
  if (nt.tm_year < 70)
    nt.tm_year += 100;
  nt.tm_mon = (reg[4] - 1) >> 4;
  nt.tm_mday = BCDtoN(reg[3]);
  nt.tm_hour = BCDtoN(reg[2]);
  nt.tm_min = BCDtoN(reg[1]);
  nt.tm_sec = BCDtoN(reg[0]);
  nt.tm_isdst = 0;

  time_t at = mktime(&nt);
  diff = at - ct;
}

// ---------------------------------------------------------------------------
//  状態保存
//
uint32_t IFCALL Calendar::GetStatusSize() {
  return sizeof(Status);
}

bool IFCALL Calendar::SaveStatus(uint8_t* s) {
  Status* st = (Status*)s;
  st->rev = ssrev;
  st->t = time(&st->t) + diff;
  st->dataoutmode = dataoutmode;
  st->hold = hold;
  st->datain = datain;
  st->strobe = strobe;
  st->cmd = cmd;
  st->scmd = scmd;
  st->pcmd = pcmd;
  memcpy(st->reg, reg, 6);
  return true;
}

bool IFCALL Calendar::LoadStatus(const uint8_t* s) {
  const Status* st = (const Status*)s;
  if (st->rev != ssrev)
    return false;
  time_t ct;
  diff = st->t - time(&ct);
  dataoutmode = st->dataoutmode;
  hold = st->hold;
  datain = st->datain;
  strobe = st->strobe;
  cmd = st->cmd;
  scmd = st->scmd;
  pcmd = st->pcmd;
  memcpy(reg, st->reg, 6);
  return true;
}

// ---------------------------------------------------------------------------
//  device description
//
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