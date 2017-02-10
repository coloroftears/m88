// ---------------------------------------------------------------------------
//  OPN/A/B interface with ADPCM support
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: opna.cpp,v 1.70 2004/02/06 13:13:39 cisc Exp $

#include "devices/opnb.h"

#include "devices/opn_build_config.h"

#include <algorithm>
#include <math.h>

namespace fmgen {

// ---------------------------------------------------------------------------
//  YM2610(OPNB)
// ---------------------------------------------------------------------------

#ifdef BUILD_OPNB

// ---------------------------------------------------------------------------
//  構築
//
OPNB::OPNB() : adpcm_(this, kADPCMNoticeBit) {
  adpcm_.SetGranularity(-1);
  csmch = &ch[2];
  ADPCMA::InitTable();
}

OPNB::~OPNB() {}

// ---------------------------------------------------------------------------
//  初期化
//
bool OPNB::Init(uint32_t c,
                uint32_t r,
                bool ipflag,
                uint8_t* _adpcma,
                int _adpcma_size,
                uint8_t* _adpcmb,
                int _adpcmb_size) {
  if (!SetRate(c, r, ipflag))
    return false;
  if (!OPNABase::Init(c, r, ipflag))
    return false;

  // ADPCMA
  config.adpcmabuf = _adpcma;
  config.adpcmasize = _adpcma_size;
  for (int i = 0; i < 6; ++i)
    if (!adpcma[i].Init(&config, this, i))
      return false;

  // ADPCMB
  adpcm_.Init(_adpcmb, _adpcmb_size);

  Reset();

  SetVolumeFM(0);
  SetVolumePSG(0);
  SetVolumeADPCMB(0);
  SetVolumeADPCMATotal(0);
  for (int i = 0; i < 6; i++)
    SetVolumeADPCMA(i, 0);
  SetChannelMask(0);
  return true;
}

// ---------------------------------------------------------------------------
//  リセット
//
void OPNB::Reset() {
  OPNABase::Reset();

  adpcm_.Reset();

  stmask = ~0;
  config.adpcmakey = 0;
  reg29 = ~0;

  for (int i = 0; i < 6; i++)
    adpcma[i].Reset();
}

// ---------------------------------------------------------------------------
//  サンプリングレート変更
//
bool OPNB::SetRate(uint32_t c, uint32_t r, bool ipflag) {
  if (!OPNABase::SetRate(c, r, ipflag))
    return false;

  // 3993600   54 / 4 = 18488
  config.adpcmastep = static_cast<int>(double(c) / 54 * 8192 / r);

  adpcm_.SetRate(clock(), r);

  return true;
}

// ---------------------------------------------------------------------------
//  レジスタアレイにデータを設定
//
void OPNB::SetReg(uint32_t addr, uint32_t data) {
  addr &= 0x1ff;

  switch (addr) {
    // omitted registers
    case 0x29:
    case 0x2d:
    case 0x2e:
    case 0x2f:
      break;

    // ADPCM A ---------------------------------------------------------------
    case 0x100:            // DM/KEYON
      if (!(data & 0x80))  // KEY ON
      {
        config.adpcmakey |= data & 0x3f;
        for (int c = 0; c < 6; c++) {
          if (data & (1 << c)) {
            ResetStatus(0x100 << c);
            adpcma[c].pos = adpcma[c].start;
            //                  adpcma[c].step = 0x10000 - adpcma[c].step;
            adpcma[c].step = 0;
            adpcma[c].ResetDecoder();
          }
        }
      } else {  // DUMP
        config.adpcmakey &= ~data;
      }
      break;

    case 0x101:
      config.adpcmatl = ~data & 63;
      break;

    case 0x108:
    case 0x109:
    case 0x10a:
    case 0x10b:
    case 0x10c:
    case 0x10d:
      adpcma[addr & 7].pan = (data >> 6) & 3;
      adpcma[addr & 7].level = ~data & 31;
      break;

    case 0x110:
    case 0x111:
    case 0x112:  // START ADDRESS (L)
    case 0x113:
    case 0x114:
    case 0x115:
    case 0x118:
    case 0x119:
    case 0x11a:  // START ADDRESS (H)
    case 0x11b:
    case 0x11c:
    case 0x11d:
      config.adpcmareg[addr - 0x110] = data;
      adpcma[addr & 7].pos = adpcma[addr & 7].start =
          (config.adpcmareg[(addr & 7) + 8] * 256 + config.adpcmareg[addr & 7])
          << 9;
      break;

    case 0x120:
    case 0x121:
    case 0x122:  // END ADDRESS (L)
    case 0x123:
    case 0x124:
    case 0x125:
    case 0x128:
    case 0x129:
    case 0x12a:  // END ADDRESS (H)
    case 0x12b:
    case 0x12c:
    case 0x12d:
      config.adpcmareg[addr - 0x110] = data;
      adpcma[addr & 7].stop = (config.adpcmareg[(addr & 7) + 24] * 256 +
                               config.adpcmareg[(addr & 7) + 16] + 1)
                              << 9;
      break;

    // ADPCMB -----------------------------------------------------------------
    case 0x10:
    case 0x11:  // Control Register 2
    case 0x12:  // Start Address L
    case 0x13:  // Start Address H
    case 0x14:  // Stop Address L
    case 0x15:  // Stop Address H
    case 0x19:  // delta-N L
    case 0x1a:  // delta-N H
    case 0x1b:  // Level Control
      adpcm_.SetRegForOPNB(addr, data);
      break;

    case 0x1c:  // Flag Control
      stmask = ~((data & 0xbf) << 8);
      status_ &= stmask;
      UpdateStatus();
      break;

    default:
      OPNABase::SetReg(addr, data);
      break;
  }
  //  Log("\n");
}

// ---------------------------------------------------------------------------
//  レジスタ取得
//
uint32_t OPNB::GetReg(uint32_t addr) {
  if (addr < 0x10)
    return psg.GetReg(addr);

  return 0;
}

// ---------------------------------------------------------------------------
//  拡張ステータスを読みこむ
//
uint32_t OPNB::ReadStatusEx() {
  return (status_ & stmask) >> 8;
}

// ---------------------------------------------------------------------------
//  YM2610
//

// ---------------------------------------------------------------------------
//  ADPCMA 合成
//
void OPNB::ADPCMAMix(FMSample* buffer, uint32_t count) {
  if (config.adpcmatvol < 128 && (config.adpcmakey & 0x3f)) {
    for (int i = 0; i < 6; i++) {
      ADPCMA& r = adpcma[i];
      if ((config.adpcmakey & (1 << i)) && r.level < 128) {
        int db = Limit(config.adpcmatl + config.adpcmatvol + r.level + r.volume,
                       127, -31);
        int vol = tltable[FM_TLPOS + (db << (FM_TLBITS - 7))] >> 4;
        bool mute = !!(rhythmmask_ & (1 << i));
        r.Mix(buffer, count, vol, mute);
      }
    }
  }
}

// ---------------------------------------------------------------------------
//  音量設定
//
void OPNB::SetVolumeADPCMATotal(int db) {
  db = std::min(db, 20);
  config.adpcmatvol = -(db * 2 / 3);
}

void OPNB::SetVolumeADPCMA(int index, int db) {
  db = std::min(db, 20);
  adpcma[index].volume = -(db * 2 / 3);
}

void OPNB::SetVolumeADPCMB(int db) {
  adpcm_.SetVolume(db);
}

// ---------------------------------------------------------------------------
//  合成
//  in:     buffer      合成先
//          nsamples    合成サンプル数
//
void OPNB::Mix(FMSample* buffer, int nsamples) {
  FMMix(buffer, nsamples);
  psg.Mix(buffer, nsamples);
  adpcm_.Mix(buffer, nsamples);
  ADPCMAMix(buffer, nsamples);
}

#endif  // BUILD_OPNB

}  // namespace fmgen
