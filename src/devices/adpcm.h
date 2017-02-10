#pragma once

#include <inttypes.h>

#include "devices/fmgen.h"

namespace fmgen {

class ADPCMCodec;
class ADPCMMemory;
class StatusSink;

class ADPCM {
 public:
  ADPCM(StatusSink* sink, uint32_t notice_bit);
  ~ADPCM();

  bool Init(uint8_t* buf, uint32_t ram_size);
  void Reset();
  void SetReg(uint32_t addr, uint32_t data);
  void SetRegForOPNB(uint32_t addr, uint32_t data);
  uint32_t ReadRAM();
  uint8_t* GetRAM() const;
  void Mix(FMSample* dest, uint32_t count);
  void SetRate(uint32_t clock, uint32_t rate);
  void SetVolume(int db);
  bool IsPlaying() const { return is_playing_; }

  // TODO: Remove this.
  void SetGranularity(int8_t granularity);

  void set_statusnext(uint32_t status) { statusnext_ = status; }
  uint32_t statusnext() const { return statusnext_; }
  void set_mask(bool mask) { mask_ = mask; }

  uint8_t control1 = 0;  // ADPCM コントロールレジスタ１
  uint8_t control2 = 0;  // ADPCM コントロールレジスタ２

 private:
  void WriteRAM(uint32_t data);
  int ReadRAMN();
  void DecodeADPCMB();

  ADPCMCodec* decoder_;
  ADPCMMemory* memory_;
  StatusSink* sink_;
  uint32_t statusnext_ = 0;
  bool mask_ = false;

  const uint32_t notice_bit_;  // ADPCM 再生終了時にたつStatusビット

  uint8_t reg_[8];  // ADPCM レジスタの一部分

  bool is_playing_ = false;  // ADPCM 再生中

  int volume_ = 0;
  int level_ = 0;  // ADPCM 音量
  int vol_ = 0;

  uint32_t delta_n_ = 256;  // ⊿N

  int out_ = 0;   // ADPCM 合成後の出力
  int out0_ = 0;  // out(t-2)+out(t-1)
  int out1_ = 0;  // out(t-1)+out(t)

  uint32_t lbase_ = 0;  // ld_ の元
  int lc_ = 0;          // 周波数変換用変数
  int ld_ = 0x100;      // 周波数変換用変数差分値
};
}  // namespace fmgen
