// ---------------------------------------------------------------------------
//  PSG Sound Implementation
//  Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//  $Id: psg.cpp,v 1.10 2002/05/15 21:38:01 cisc Exp $

#include "devices/psg.h"

#include <math.h>

#include "common/clamp.h"

namespace fmgen {

namespace {
const int kVolumeLevels = 32;
const int kEnvelopeTypes = 16;

const int noisetablesize = 1 << 11;  // ←メモリ使用量を減らしたいなら減らして
const int toneshift = 24;
const int envshift = 22;
const int noiseshift = 14;
const int kOverSamplingBits = 2;  // ← 音質より速度が優先なら減らすといいかも

// Volume for each level.
int32_t EmitTable[kVolumeLevels];

uint32_t noisetable[noisetablesize];
int32_t envelopetable[kEnvelopeTypes][64];

}  // namespace

// ---------------------------------------------------------------------------
//  コンストラクタ・デストラクタ
//
PSG::PSG() {
  SetVolume(0);
  MakeNoiseTable();
  Reset();
  mask_ = 0x3f;
}

PSG::~PSG() {}

// ---------------------------------------------------------------------------
//  PSG を初期化する(RESET)
//
void PSG::Reset() {
  for (int i = 0; i < 14; ++i)
    SetReg(i, 0);
  SetReg(7, 0xff);
  SetReg(14, 0xff);
  SetReg(15, 0xff);

  for (int i = 0; i < 3; ++i)
    scount[i] = 0;
}

// ---------------------------------------------------------------------------
//  クロック周波数の設定
//
void PSG::SetClock(int clock, int rate) {
  // 4.0 = oversampling
  tperiodbase = static_cast<int>((1 << toneshift) / 4.0 * clock / rate);
  eperiodbase = static_cast<int>((1 << envshift) / 4.0 * clock / rate);
  nperiodbase = static_cast<int>((1 << noiseshift) / 4.0 * clock / rate);

  // 各データの更新
  for (int i = 0; i < 3; ++i) {
    uint16_t tune = GetTune(i);
    speriod[i] = tune ? tperiodbase / tune : tperiodbase;
  }

  uint16_t nf = GetNoisePeriod();
  nperiod = nf ? nperiodbase / nf / 2 : nperiodbase / 2;

  uint16_t ef = GetEnvelopePeriod();
  eperiod = ef ? eperiodbase / ef : eperiodbase * 2;
}

// ---------------------------------------------------------------------------
//  ノイズテーブルを作成する
//
void PSG::MakeNoiseTable() {
  if (!noisetable[0]) {
    int noise = 14321;
    for (int i = 0; i < noisetablesize; ++i) {
      int n = 0;
      for (int j = 0; j < 32; ++j) {
        n = n * 2 + (noise & 1);
        noise = (noise >> 1) | (((noise << 14) ^ (noise << 16)) & 0x10000);
      }
      noisetable[i] = n;
    }
  }
}

// ---------------------------------------------------------------------------
//  出力テーブルを作成
//  素直にテーブルで持ったほうが省スペース。
//
void PSG::SetVolume(int volume) {
  const int kMaxVolumeLevel = 0x4000;
  double base = kMaxVolumeLevel / 3.0 * pow(10.0, volume / 40.0);
  for (int i = kVolumeLevels - 1; i >= 2; i--) {
    EmitTable[i] = static_cast<int32_t>(base);
    // 1.1890... = 256 ** (1.0 / 32)
    base /= 1.189207115002721;
  }
  // Special case: vol = 0 & vol = 1
  EmitTable[1] = 0;
  EmitTable[0] = 0;

  MakeEnvelopeTable();
  SetChannelMask(~mask_);
}

void PSG::SetChannelMask(int c) {
  mask_ = ~c;
  for (int i = 0; i < 3; ++i)
    olevel[i] = mask_ & (1 << i) ? EmitTable[(reg_[8 + i] & 15) * 2 + 1] : 0;
}

// ---------------------------------------------------------------------------
//  エンベロープ波形テーブル
//
void PSG::MakeEnvelopeTable() {
  // 0 lo  1 up 2 down 3 hi
  static uint8_t table1[kEnvelopeTypes * 2] = {
      2, 0, 2, 0, 2, 0, 2, 0, 1, 0, 1, 0, 1, 0, 1, 0,
      2, 2, 2, 0, 2, 1, 2, 3, 1, 1, 1, 3, 1, 2, 1, 0,
  };
  static uint8_t initial_level[4] = {0, 0, 31, 31};
  static uint8_t delta_table[4] = {0, 1, static_cast<uint8_t>(-1), 0};

  int32_t* ptr = envelopetable[0];

  for (int i = 0; i < kEnvelopeTypes * 2; ++i) {
    uint8_t v = initial_level[table1[i]];

    for (int j = 0; j < 32; ++j) {
      *ptr++ = EmitTable[v];
      v += delta_table[table1[i]];
    }
  }
}

// ---------------------------------------------------------------------------
//  PSG のレジスタに値をセットする
//  regnum      レジスタの番号 (0 - 15)
//  data        セットする値
//
void PSG::SetReg(uint32_t regnum, uint8_t data) {
  if (regnum < 0x10) {
    reg_[regnum] = data;
    switch (regnum) {
      int tmp;

      case 0:  // ChA Fine Tune
      case 1:  // ChA Coarse Tune
        tmp = GetTune(0);
        speriod[0] = tmp ? tperiodbase / tmp : tperiodbase;
        break;

      case 2:  // ChB Fine Tune
      case 3:  // ChB Coarse Tune
        tmp = GetTune(1);
        speriod[1] = tmp ? tperiodbase / tmp : tperiodbase;
        break;

      case 4:  // ChC Fine Tune
      case 5:  // ChC Coarse Tune
        tmp = GetTune(2);
        speriod[2] = tmp ? tperiodbase / tmp : tperiodbase;
        break;

      case 6:  // Noise generator control
        data &= 0x1f;
        nperiod = data ? nperiodbase / data : nperiodbase;
        break;

      case 8:
        olevel[0] = mask_ & 1 ? EmitTable[GetInternalVolume(data)] : 0;
        break;

      case 9:
        olevel[1] = mask_ & 2 ? EmitTable[GetInternalVolume(data)] : 0;
        break;

      case 10:
        olevel[2] = mask_ & 4 ? EmitTable[GetInternalVolume(data)] : 0;
        break;

      case 11:  // Envelope period
      case 12:
        tmp = GetEnvelopePeriod();
        eperiod = tmp ? eperiodbase / tmp : eperiodbase * 2;
        break;

      case 13:  // Envelope shape
        ecount = 0;
        envelope_ = envelopetable[data & 15];
        break;
    }
  }
}

// ---------------------------------------------------------------------------
//  PCM データを吐き出す(2ch)
//  dest        PCM データを展開するポインタ
//  nsamples    展開する PCM のサンプル数
//
void PSG::Mix(Sample* dest, int nsamples) {
  uint8_t r7 = ~reg_[7];
  if (!((r7 & 0x3f) | ((reg_[8] | reg_[9] | reg_[10]) & 0x1f)))
    return;

  // channel enable
  uint8_t chenable[3];
  // noise enable
  uint8_t nenable[3];

  chenable[0] = (r7 & 0x01) && (speriod[0] <= (1 << toneshift));
  chenable[1] = (r7 & 0x02) && (speriod[1] <= (1 << toneshift));
  chenable[2] = (r7 & 0x04) && (speriod[2] <= (1 << toneshift));
  nenable[0] = (r7 >> 3) & 1;
  nenable[1] = (r7 >> 4) & 1;
  nenable[2] = (r7 >> 5) & 1;

  int32_t noise = 0;
  int32_t sample = 0;
  int32_t env = 0;

  int32_t* p1 = ((mask_ & 1) && (reg_[8] & 0x10)) ? &env : &olevel[0];
  int32_t* p2 = ((mask_ & 2) && (reg_[9] & 0x10)) ? &env : &olevel[1];
  int32_t* p3 = ((mask_ & 4) && (reg_[10] & 0x10)) ? &env : &olevel[2];

#define SCOUNT(ch) (scount[ch] >> (toneshift + kOverSamplingBits))

  if (p1 != &env && p2 != &env && p3 != &env) {
    // エンベロープ無し
    if ((r7 & 0x38) == 0) {
      // ノイズ無し
      for (int i = 0; i < nsamples; ++i) {
        sample = 0;
        for (int j = 0; j < (1 << kOverSamplingBits); ++j) {
          for (int k = 0; k < 3; ++k) {
            int x = (SCOUNT(k) & chenable[k]) - 1;
            sample += (olevel[k] + x) ^ x;
            scount[k] += speriod[k];
          }
        }
        sample /= (1 << kOverSamplingBits);
        StoreSample(dest[0], sample);
        StoreSample(dest[1], sample);
        dest += 2;
      }
    } else {
      // ノイズ有り
      for (int i = 0; i < nsamples; ++i) {
        sample = 0;
        for (int j = 0; j < (1 << kOverSamplingBits); ++j) {
          noise = noisetable[(ncount >> (noiseshift + kOverSamplingBits + 6)) &
                             (noisetablesize - 1)] >>
                  (ncount >> (noiseshift + kOverSamplingBits + 1) & 31);
          ncount += nperiod;
          for (int k = 0; k < 3; ++k) {
            int32_t x = ((SCOUNT(k) & chenable[k]) | (nenable[k] & noise)) -
                        1;  // 0 or -1
            sample += (olevel[k] + x) ^ x;
            scount[k] += speriod[k];
          }
        }
        sample /= (1 << kOverSamplingBits);
        StoreSample(dest[0], sample);
        StoreSample(dest[1], sample);
        dest += 2;
      }
    }

    // エンベロープの計算をさぼった帳尻あわせ
    ecount = (ecount >> 8) + (eperiod >> (8 - kOverSamplingBits)) * nsamples;
    if (ecount >= (1 << (envshift + 6 + kOverSamplingBits - 8))) {
      if ((reg_[0x0d] & 0x0b) != 0x0a)
        ecount |= (1 << (envshift + 5 + kOverSamplingBits - 8));
      ecount &= (1 << (envshift + 6 + kOverSamplingBits - 8)) - 1;
    }
    ecount <<= 8;
  } else {
    // エンベロープあり
    for (int i = 0; i < nsamples; ++i) {
      sample = 0;
      for (int j = 0; j < (1 << kOverSamplingBits); ++j) {
        env = envelope_[ecount >> (envshift + kOverSamplingBits)];
        ecount += eperiod;
        if (ecount >= (1 << (envshift + 6 + kOverSamplingBits))) {
          if ((reg_[0x0d] & 0x0b) != 0x0a)
            ecount |= (1 << (envshift + 5 + kOverSamplingBits));
          ecount &= (1 << (envshift + 6 + kOverSamplingBits)) - 1;
        }
        noise = noisetable[(ncount >> (noiseshift + kOverSamplingBits + 6)) &
                           (noisetablesize - 1)] >>
                (ncount >> (noiseshift + kOverSamplingBits + 1) & 31);
        ncount += nperiod;

        int32_t x =
            ((SCOUNT(0) & chenable[0]) | (nenable[0] & noise)) - 1;  // 0 or -1
        sample += (*p1 + x) ^ x;
        scount[0] += speriod[0];
        int32_t y = ((SCOUNT(1) & chenable[1]) | (nenable[1] & noise)) - 1;
        sample += (*p2 + y) ^ y;
        scount[1] += speriod[1];
        int32_t z = ((SCOUNT(2) & chenable[2]) | (nenable[2] & noise)) - 1;
        sample += (*p3 + z) ^ z;
        scount[2] += speriod[2];
      }
      sample /= (1 << kOverSamplingBits);
      StoreSample(dest[0], sample);
      StoreSample(dest[1], sample);
      dest += 2;
    }
  }
}
}  // namespace fmgen
