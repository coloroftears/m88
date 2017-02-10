// ---------------------------------------------------------------------------
//  OPN/A/B interface with ADPCM support
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: opna.cpp,v 1.70 2004/02/06 13:13:39 cisc Exp $

#include "devices/opna.h"

#include "devices/opn_build_config.h"

#include <algorithm>
#include <assert.h>
#include <math.h>
#include <string.h>

//  TOFIX:
//   OPN ch3 が常にPrepareの対象となってしまう障害

namespace fmgen {

// ---------------------------------------------------------------------------
//  OPNBase

#if defined(BUILD_OPN) || defined(BUILD_OPNA) || defined(BUILD_OPNB)

uint32_t OPNBase::lfotable[8];  // OPNA/B 用

OPNBase::OPNBase() {
  timer_.set_timera_sink(this);
  prescale = 0;
}

//  パラメータセット
void OPNBase::SetParameter(Channel4* ch, uint32_t addr, uint32_t data) {
  const static uint32_t slottable[4] = {0, 2, 1, 3};
  const static uint8_t sltable[16] = {
      0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 124,
  };

  if ((addr & 3) < 3) {
    uint32_t slot = slottable[(addr >> 2) & 3];
    Operator* op = &ch->op[slot];

    switch ((addr >> 4) & 15) {
      case 3:  // 30-3E DT/MULTI
        op->SetDT((data >> 4) & 0x07);
        op->SetMULTI(data & 0x0f);
        break;

      case 4:  // 40-4E TL
        op->SetTL(data & 0x7f,
                  ((timer_.regtc() & 0xc0) == 0x80) && (csmch == ch));
        break;

      case 5:  // 50-5E KS/AR
        op->SetKS((data >> 6) & 3);
        op->SetAR((data & 0x1f) * 2);
        break;

      case 6:  // 60-6E DR/AMON
        op->SetDR((data & 0x1f) * 2);
        op->SetAMON((data & 0x80) != 0);
        break;

      case 7:  // 70-7E SR
        op->SetSR((data & 0x1f) * 2);
        break;

      case 8:  // 80-8E SL/RR
        op->SetSL(sltable[(data >> 4) & 15]);
        op->SetRR((data & 0x0f) * 4 + 2);
        break;

      case 9:  // 90-9E SSG-EC
        op->SetSSGEC(data & 0x0f);
        break;
    }
  }
}

//  リセット
void OPNBase::Reset() {
  status_ = 0;
  SetPrescaler(0);
  timer_.Reset();
  psg.Reset();
}

//  プリスケーラ設定
void OPNBase::SetPrescaler(uint32_t p) {
  static const char table[3][2] = {{6, 4}, {3, 2}, {2, 1}};
  static const uint8_t table2[8] = {108, 77, 71, 67, 62, 44, 8, 5};
  // 512
  if (prescale != p) {
    prescale = p;
    assert(prescale < 3);

    uint32_t fmclock = clock() / table[p][0] / 12;

    set_rate(psgrate);

    // 合成周波数と出力周波数の比
    assert(fmclock < (0x80000000 >> FM_RATIOBITS));
    uint32_t ratio = ((fmclock << FM_RATIOBITS) + rate() / 2) / rate();

    chip.SetRatio(ratio);
    psg.SetClock(clock() / table[p][1], psgrate);

    for (int i = 0; i < 8; i++) {
      lfotable[i] = (ratio << (2 + FM_LFOCBITS - FM_RATIOBITS)) / table2[i];
    }
  }
}

//  初期化
bool OPNBase::Init(uint32_t clk, uint32_t samplerate) {
  set_clock(clk);
  timer_.SetTimerBase(clk);
  psgrate = samplerate;

  return true;
}

//  音量設定
void OPNBase::SetVolumeFM(int db) {
  db = std::min(db, 20);
  if (db > -192)
    fmvolume = int(16384.0 * pow(10.0, db / 40.0));
  else
    fmvolume = 0;
}

//  タイマー時間処理
void OPNBase::TimerA() {
  if ((timer_.regtc() & 0xc0) == 0x80) {
    csmch->KeyControl(0x00);
    csmch->KeyControl(0x0f);
  }
}

#endif  // defined(BUILD_OPN) || defined(BUILD_OPNA) || defined (BUILD_OPNB)

// ---------------------------------------------------------------------------
//  YM2203
//
#ifdef BUILD_OPN

OPN::OPN() {
  timer_.set_status_sink(this);

  SetVolumeFM(0);
  SetVolumePSG(0);

  csmch = &ch[2];

  for (int i = 0; i < 3; i++) {
    ch[i].SetChip(&chip);
    ch[i].SetType(OpType::kOPN);
  }
}

//  初期化
bool OPN::Init(uint32_t clk, uint32_t samplerate, bool ip, const char*) {
  if (!SetRate(clk, samplerate, ip))
    return false;

  Reset();

  SetVolumeFM(0);
  SetVolumePSG(0);
  SetChannelMask(0);
  return true;
}

//  サンプリングレート変更
bool OPN::SetRate(uint32_t clk, uint32_t samplerate, bool) {
  OPNBase::Init(clk, samplerate);
  RebuildTimeTable();
  return true;
}

//  リセット
void OPN::Reset() {
  int i;
  for (i = 0x20; i < 0x28; i++)
    SetReg(i, 0);
  for (i = 0x30; i < 0xc0; i++)
    SetReg(i, 0);
  OPNBase::Reset();
  ch[0].Reset();
  ch[1].Reset();
  ch[2].Reset();
}

//  レジスタ読み込み
uint32_t OPN::GetReg(uint32_t addr) {
  if (addr < 0x10)
    return psg.GetReg(addr);
  else
    return 0;
}

//  レジスタアレイにデータを設定
void OPN::SetReg(uint32_t addr, uint32_t data) {
  //  Log("reg[%.2x] <- %.2x\n", addr, data);
  if (addr >= 0x100)
    return;

  int c = addr & 3;
  switch (addr) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      psg.SetReg(addr, data);
      break;

    case 0x24:
    case 0x25:
      timer_.SetTimerA(addr, data);
      break;

    case 0x26:
      timer_.SetTimerB(data);
      break;

    case 0x27:
      timer_.SetTimerControl(data);
      break;

    case 0x28:  // Key On/Off
      if ((data & 3) < 3)
        ch[data & 3].KeyControl(data >> 4);
      break;

    case 0x2d:
    case 0x2e:
    case 0x2f:
      SetPrescaler(addr - 0x2d);
      break;

    // F-Number
    case 0xa0:
    case 0xa1:
    case 0xa2:
      fnum[c] = data + fnum2[c] * 0x100;
      break;

    case 0xa4:
    case 0xa5:
    case 0xa6:
      fnum2[c] = uint8_t(data);
      break;

    case 0xa8:
    case 0xa9:
    case 0xaa:
      fnum3[c] = data + fnum2[c + 3] * 0x100;
      break;

    case 0xac:
    case 0xad:
    case 0xae:
      fnum2[c + 3] = uint8_t(data);
      break;

    case 0xb0:
    case 0xb1:
    case 0xb2:
      ch[c].SetFB((data >> 3) & 7);
      ch[c].SetAlgorithm(data & 7);
      break;

    default:
      if (c < 3) {
        if ((addr & 0xf0) == 0x60)
          data &= 0x1f;
        OPNBase::SetParameter(&ch[c], addr, data);
      }
      break;
  }
}

//  ステータスフラグ設定
void OPN::SetStatus(uint32_t bits) {
  if (!status_ & bits) {
    status_ |= bits;
    Intr(true);
  }
}

void OPN::ResetStatus(uint32_t bit) {
  status_ &= ~bit;
  if (!status_)
    Intr(false);
}

//  マスク設定
void OPN::SetChannelMask(uint32_t mask) {
  for (int i = 0; i < 3; i++)
    ch[i].Mute(!!(mask & (1 << i)));
  psg.SetChannelMask(mask >> 6);
}

//  合成(2ch)
void OPN::Mix(FMSample* buffer, int nsamples) {
#define IStoSample(s) ((Limit(s, 0x7fff, -0x8000) * fmvolume) >> 14)

  psg.Mix(buffer, nsamples);

  // Set F-Number
  ch[0].SetFNum(fnum[0]);
  ch[1].SetFNum(fnum[1]);
  if (!(timer_.regtc() & 0xc0))
    ch[2].SetFNum(fnum[2]);
  else {  // 効果音
    ch[2].op[0].SetFNum(fnum3[1]);
    ch[2].op[1].SetFNum(fnum3[2]);
    ch[2].op[2].SetFNum(fnum3[0]);
    ch[2].op[3].SetFNum(fnum[2]);
  }

  int actch =
      (((ch[2].Prepare() << 2) | ch[1].Prepare()) << 2) | ch[0].Prepare();
  if (actch & 0x15) {
    FMSample* limit = buffer + nsamples * 2;
    for (FMSample* dest = buffer; dest < limit; dest += 2) {
      ISample s = 0;
      if (actch & 0x01)
        s = ch[0].Calc();
      if (actch & 0x04)
        s += ch[1].Calc();
      if (actch & 0x10)
        s += ch[2].Calc();
      s = IStoSample(s);
      StoreSample(dest[0], s);
      StoreSample(dest[1], s);
    }
  }
#undef IStoSample
}

#endif  // BUILD_OPN

}  // namespace fmgen
