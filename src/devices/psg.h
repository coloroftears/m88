// ---------------------------------------------------------------------------
//  PSG-like sound generator
//  Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//  $Id: psg.h,v 1.8 2003/04/22 13:12:53 cisc Exp $

#pragma once

#include <stdint.h>

#define PSG_SAMPLETYPE int32_t  // int32_t or int16_t

namespace fmgen {

// ---------------------------------------------------------------------------
//  class PSG
//  PSG に良く似た音を生成する音源ユニット
//
//  interface:
//  bool SetClock(uint32_t clock, uint32_t rate)
//      初期化．このクラスを使用する前にかならず呼んでおくこと．
//      PSG のクロックや PCM レートを設定する
//
//      clock:  PSG の動作クロック
//      rate:   生成する PCM のレート
//      retval  初期化に成功すれば true
//
//  void Mix(Sample* dest, int nsamples)
//      PCM を nsamples 分合成し， dest で始まる配列に加える(加算する)
//      あくまで加算なので，最初に配列をゼロクリアする必要がある
//
//  void Reset()
//      リセットする
//
//  void SetReg(uint32_t reg, uint8_t data)
//      レジスタ reg に data を書き込む
//
//  uint32_t GetReg(uint32_t reg)
//      レジスタ reg の内容を読み出す
//
//  void SetVolume(int db)
//      各音源の音量を調節する
//      単位は約 1/2 dB
//
class PSG {
 public:
  using Sample = PSG_SAMPLETYPE;
  PSG();
  ~PSG();

  void Mix(Sample* dest, int nsamples);
  void SetClock(int clock, int rate);

  void SetVolume(int vol);
  void SetChannelMask(int c);

  void Reset();
  void SetReg(uint32_t regnum, uint8_t data);
  uint32_t GetReg(uint32_t regnum) const { return reg_[regnum & 0x0f]; }

 private:
  void MakeNoiseTable();
  void MakeEnvelopeTable();

  uint16_t GetTune(int ch) const {
    return (reg_[ch * 2] + reg_[ch * 2 + 1] * 256) & 0xfff;
  }
  uint16_t GetNoisePeriod() const { return reg_[6] & 0x1f; }
  uint16_t GetEnvelopePeriod() const {
    return (reg_[11] + reg_[12] * 256) & 0xffff;
  }
  int GetInternalVolume(int vol) const { return (vol & 15) * 2 + 1; }
  uint8_t reg_[16];

  const int32_t* envelope_;

  int32_t olevel[3];
  uint32_t scount[3];
  uint32_t speriod[3];

  uint32_t ecount;
  uint32_t eperiod;

  uint32_t ncount;
  uint32_t nperiod;

  uint32_t tperiodbase;
  uint32_t eperiodbase;
  uint32_t nperiodbase;

  int mask_;
};
}  // namespace fmgen