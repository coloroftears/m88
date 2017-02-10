#include "devices/adpcm.h"

#include <math.h>
#include <stdlib.h>

#include <algorithm>
#include <memory>

#include "devices/opna.h"

namespace fmgen {

class ADPCMCodec {
 public:
  void Reset() {
    x_ = 0;
    delta_ = 127;
  }

  // |data| is expected to be 4bit
  void Update(uint32_t data) {
    static int table1[16] = {
        1, 3, 5, 7, 9, 11, 13, 15, -1, -3, -5, -7, -9, -11, -13, -15,
    };
    static int table2[16] = {
        57, 57, 57, 57, 77, 102, 128, 153, 57, 57, 57, 57, 77, 102, 128, 153,
    };
    x_ = Limit16(x_ + table1[data] * delta_ / 8);
    delta_ = Limit(delta_ * table2[data] / 64, 24576, 127);
  }

  // |data| is expected to be 8bit PCM
  void Encode(uint32_t data) {
    // 8->16
    data *= 256;
    // dn
    int diff = x_ - data;
    int sign = diff < 0 ? 8 : 0;
    int l = (abs(diff) * 4 / delta_) & 7;
    int adpcm = sign | l;
    Update(adpcm);
  }

  // Return 16bit PCM
  int32_t GetPCM() const { return x_; }

 private:
  int32_t x_ = 0;
  int16_t delta_ = 127;
};

class ADPCMMemory {
 public:
  ADPCMMemory() {}
  ~ADPCMMemory() {}

  void Init(uint8_t* buf, int32_t ram_size) {
    ram_.reset(buf ? buf : new uint8_t[ram_size]);

    //  ram_mask_ = ram_size - 1;
    for (int i = 0; i <= 24; i++) {  // max 16M bytes
      if (ram_size <= (1 << i)) {
        ram_mask_ = (1 << i) - 1;
        break;
      }
    }
    limit_addr_ = ram_mask_;
  }

  void SetControl2(uint32_t data) {
    control2_ = data;
    granularity_ = data & 2 ? 1 : 4;
  }

  // This is used only by OPNB.
  void SetGranularity(int8_t granularity) { granularity_ = granularity; }

// ---------------------------------------------------------------------------
//  ADPCM データの格納方式の違い (8bit/1bit) をエミュレートしない
//  このオプションを有効にすると ADPCM メモリへのアクセス(特に 8bit モード)が
//  多少軽くなるかも
//
// #define NO_BITTYPE_EMULATION

#ifndef NO_BITTYPE_EMULATION
  void Write(uint32_t data) {
    if (!(control2_ & 2)) {
      // 1 bit mode
      ram_[(mem_addr_ >> 4) & 0x3ffff] = data;
      mem_addr_ += 16;
    } else {
      // 8 bit mode
      uint8_t* p = &ram_[(mem_addr_ >> 4) & 0x7fff];
      uint32_t bank = (mem_addr_ >> 1) & 7;
      uint8_t mask = 1 << bank;
      data <<= bank;

      p[0x00000] = (p[0x00000] & ~mask) | (uint8_t(data) & mask);
      data >>= 1;
      p[0x08000] = (p[0x08000] & ~mask) | (uint8_t(data) & mask);
      data >>= 1;
      p[0x10000] = (p[0x10000] & ~mask) | (uint8_t(data) & mask);
      data >>= 1;
      p[0x18000] = (p[0x18000] & ~mask) | (uint8_t(data) & mask);
      data >>= 1;
      p[0x20000] = (p[0x20000] & ~mask) | (uint8_t(data) & mask);
      data >>= 1;
      p[0x28000] = (p[0x28000] & ~mask) | (uint8_t(data) & mask);
      data >>= 1;
      p[0x30000] = (p[0x30000] & ~mask) | (uint8_t(data) & mask);
      data >>= 1;
      p[0x38000] = (p[0x38000] & ~mask) | (uint8_t(data) & mask);
      mem_addr_ += 2;
    }
  }

  uint32_t Read() {
    uint32_t data = 0;
    if (!(control2_ & 2)) {
      // 1 bit mode
      data = ram_[(mem_addr_ >> 4) & 0x3ffff];
      mem_addr_ += 16;
    } else {
      // 8 bit mode
      uint8_t* p = &ram_[(mem_addr_ >> 4) & 0x7fff];
      uint32_t bank = (mem_addr_ >> 1) & 7;
      uint8_t mask = 1 << bank;

      data = (p[0x38000] & mask);
      data = data * 2 + (p[0x30000] & mask);
      data = data * 2 + (p[0x28000] & mask);
      data = data * 2 + (p[0x20000] & mask);
      data = data * 2 + (p[0x18000] & mask);
      data = data * 2 + (p[0x10000] & mask);
      data = data * 2 + (p[0x08000] & mask);
      data = data * 2 + (p[0x00000] & mask);
      data >>= bank;
      mem_addr_ += 2;
    }
    return data;
  }

  uint32_t Read4() {
    uint32_t data = 0;
    if (granularity_ > 0) {
      if (!(control2_ & 2)) {
        data = ram_[(mem_addr_ >> 4) & 0x3ffff];
        mem_addr_ += 8;
        if (mem_addr_ & 8)
          return data >> 4;
        data &= 0x0f;
      } else {
        uint8_t* p =
            &ram_[(mem_addr_ >> 4) & 0x7fff] + ((~mem_addr_ & 1) << 17);
        uint32_t bank = (mem_addr_ >> 1) & 7;
        uint8_t mask = 1 << bank;

        data = (p[0x18000] & mask);
        data = data * 2 + (p[0x10000] & mask);
        data = data * 2 + (p[0x08000] & mask);
        data = data * 2 + (p[0x00000] & mask);
        data >>= bank;
        mem_addr_++;
        if (mem_addr_ & 1)
          return data;
      }
    } else {
      data = ram_[(mem_addr_ >> 1) & ram_mask_];
      ++mem_addr_;
      if (mem_addr_ & 1)
        return data >> 4;
      data &= 0x0f;
    }
    return data;
  }
#else
  void Write(uint32_t data) {
    ram_[(mem_addr_ >> granularity_) & 0x3ffff] = data;
    mem_addr_ += 1 << granularity_;
  }

  uint32_t Read() {
    uint32_t data = ram_[(mem_addr_ >> granularity_) & 0x3ffff];
    mem_addr_ += 1 << granularity_;
    return data;
  }

  uint32_t Read4() {
    uint32_t data = 0;
    if (granularity_ > 0) {
      data = ram_[(mem_addr_ >> granularity_) & ram_mask_];
      mem_addr_ += 1 << (granularity_ - 1);
      if (mem_addr_ & (1 << (granularity_ - 1)))
        return data >> 4;
      data &= 0x0f;
    } else {
      data = ram_[(mem_addr_ >> 1) & ram_mask_];
      ++mem_addr_;
      if (mem_addr_ & 1)
        return data >> 4;
      data &= 0x0f;
    }
    return data;
  }
#endif

  uint8_t* GetBuffer() const { return ram_.get(); }

  bool IsLessThanStopAddress() const { return mem_addr_ < stop_addr_; }
  bool IsStopAddress() const { return mem_addr_ == stop_addr_; }
  void CheckRamLimit() {
    if (mem_addr_ == limit_addr_)
      mem_addr_ = 0;
  }
  // ??? This smells a bug.
  void WrapMemAddress() { mem_addr_ &= 0x3fffff; }

  void reset_mem_addr() { mem_addr_ = start_addr_; }
  void wrap_mem_addr() { mem_addr_ &= ram_mask_; }
  void set_start_addr(uint32_t addr) { mem_addr_ = start_addr_ = addr; }
  void set_stop_addr(uint32_t addr) { stop_addr_ = addr; }
  void set_limit_addr(uint32_t addr) { limit_addr_ = addr; }

 private:
  std::unique_ptr<uint8_t[]> ram_;

  uint32_t mem_addr_ = 0;  // 再生中アドレス

  uint32_t start_addr_ = 0;  // Start address
  uint32_t stop_addr_ = 0;   // Stop address

  uint32_t ram_mask_ = 0;  // メモリアドレスに対するビットマスク
  uint32_t limit_addr_ = 0;

  uint8_t control2_ = 0;
  int8_t granularity_ = 0;
};

// Used for sampling rate conversion.
const int kADPCMRateMultiplier = 8192;
const int kADPCMRateShift = 13;

ADPCM::ADPCM(StatusSink* sink, uint32_t notice_bit)
    : decoder_(new ADPCMCodec),
      memory_(new ADPCMMemory),
      sink_(sink),
      notice_bit_(notice_bit) {
  for (int i = 0; i < 8; ++i)
    reg_[i] = 0;
}

ADPCM::~ADPCM() {
  delete memory_;
  delete decoder_;
}

bool ADPCM::Init(uint8_t* buf, uint32_t ram_size) {
  memory_->Init(buf, ram_size);
  return true;
}

void ADPCM::Reset() {
  memory_->set_start_addr(0);
  decoder_->Reset();

  statusnext_ = 0;
  is_playing_ = false;
  level_ = 0;

  out_ = out0_ = out1_ = 0;
  lc_ = 0;
  ld_ = 0x100;
}

void ADPCM::SetRate(uint32_t clock, uint32_t rate) {
  lbase_ = static_cast<int>(kADPCMRateMultiplier * (clock / 72.0) / rate);
  ld_ = delta_n_ * lbase_ >> 16;
}

void ADPCM::SetVolume(int db) {
  db = std::min(db, 20);
  if (db > -192)
    vol_ = static_cast<int>(65536.0 * pow(10.0, db / 40.0));
  else
    vol_ = 0;

  volume_ = (vol_ * level_) >> 12;
}

void ADPCM::SetGranularity(int8_t granularity) {
  memory_->SetGranularity(granularity);
}

void ADPCM::SetReg(uint32_t addr, uint32_t data) {
  switch (addr) {
    case 0x00:  // Control Register 1
      // d0: reset
      // d1/d2: N/A
      // d3: spoff
      // d4: repeat
      // d5: memdata (external memory: 1, cpu: 0)
      // d6: rec
      // d7: start
      if ((data & 0x80) && !is_playing_) {
        is_playing_ = true;
        memory_->reset_mem_addr();
        decoder_->Reset();
        lc_ = 0;
      }
      if (data & 1)
        is_playing_ = false;
      control1 = data;
      break;

    case 0x01:  // Control Register 2
      // d0: rom (1: rom 0: dram)
      // d1: ram type (1: x8bit, 0: x1bit)
      // d2: DA/AD (1: DA = output $0E reg, 0: AD = signed 8bitPCM)
      // d3: SAMPLE (1: start)
      // d4-d5: N/A
      // d6: Rch (1: on)
      // d7: Lch (1: on)
      control2 = data;
      memory_->SetControl2(data);
      break;

    case 0x02:  // Start Address L
    case 0x03:  // Start Address H
      reg_[addr - 0x02 + 0] = data;
      memory_->set_start_addr((reg_[1] * 256 + reg_[0]) << 6);
      //      Log("  start_addr_ %.6x", start_addr_);
      break;

    case 0x04:  // Stop Address L
    case 0x05:  // Stop Address H
      reg_[addr - 0x04 + 2] = data;
      memory_->set_stop_addr((reg_[3] * 256 + reg_[2] + 1) << 6);
      //      Log("  stop_addr_ %.6x", stop_addr_);
      break;

    case 0x08:  // ADPCM data
      if ((control1 & 0x60) == 0x60) {
        //          Log("  Wr [0x%.5x] = %.2x", mem_addr_, data);
        WriteRAM(data);
      }
      break;

    case 0x09:  // delta-N L
    case 0x0a:  // delta-N H
      reg_[addr - 0x09 + 4] = data;
      delta_n_ = reg_[5] * 256 + reg_[4];
      delta_n_ = std::max(256U, delta_n_);
      ld_ = delta_n_ * lbase_ >> 16;
      break;

    case 0x0b:  // Level Control
      level_ = data;
      volume_ = (vol_ * level_) >> 12;
      break;

    case 0x0c:  // Limit Address L
    case 0x0d:  // Limit Address H
      reg_[addr - 0x0c + 6] = data;
      memory_->set_limit_addr((reg_[7] * 256 + reg_[6] + 1) << 6);
      //      Log("  limitaddr %.6x", ram_limit_);
      break;

    case 0x10:  // Flag Control
      // Handled in OPNABase::SetADPCMBReg().
      break;
    default:
      // NOTREACHED();
      break;
  }
}

void ADPCM::SetRegForOPNB(uint32_t addr, uint32_t data) {
  switch (addr) {
    case 0x10:
      if ((data & 0x80) && !is_playing_) {
        is_playing_ = true;
        memory_->reset_mem_addr();
        decoder_->Reset();
        lc_ = 0;
      }
      if (data & 1)
        is_playing_ = false;
      control1 = data & 0x91;
      break;

    case 0x11:  // Control Register 2
      control2 = data & 0xc0;
      break;

    case 0x12:  // Start Address L
    case 0x13:  // Start Address H
      reg_[addr - 0x12 + 0] = data;
      memory_->set_start_addr((reg_[1] * 256 + reg_[0]) << 9);
      break;

    case 0x14:  // Stop Address L
    case 0x15:  // Stop Address H
      reg_[addr - 0x14 + 2] = data;
      memory_->set_stop_addr((reg_[3] * 256 + reg_[2] + 1) << 9);
      //      Log("  stop_addr_ %.6x", stop_addr_);
      break;

    case 0x19:  // delta-N L
    case 0x1a:  // delta-N H
      reg_[addr - 0x19 + 4] = data;
      delta_n_ = reg_[5] * 256 + reg_[4];
      delta_n_ = std::max(256U, delta_n_);
      ld_ = delta_n_ * lbase_ >> 16;
      break;

    case 0x1b:  // Level Control
      level_ = data;
      volume_ = (vol_ * level_) >> 12;
      break;

    default:
      // NOTREACHED();
      break;
  }
}

void ADPCM::WriteRAM(uint32_t data) {
  memory_->Write(data);

  if (memory_->IsStopAddress()) {
    sink_->SetStatus(4);
    statusnext_ = 0x04;  // EOS
    // ???
    memory_->WrapMemAddress();
  }
  memory_->CheckRamLimit();
  sink_->SetStatus(8);
}

uint32_t ADPCM::ReadRAM() {
  uint32_t data = memory_->Read();

  if (memory_->IsStopAddress()) {
    sink_->SetStatus(4);
    statusnext_ = 0x04;  // EOS
    // ???
    memory_->WrapMemAddress();
  }
  memory_->CheckRamLimit();
  if (memory_->IsLessThanStopAddress())
    sink_->SetStatus(8);

  return data;
}

uint8_t* ADPCM::GetRAM() const {
  return memory_->GetBuffer();
}

int ADPCM::ReadRAMN() {
  uint32_t data = memory_->Read4();
  decoder_->Update(data);

  // check
  if (memory_->IsStopAddress()) {
    if (control1 & 0x10) {
      memory_->reset_mem_addr();
      data = decoder_->GetPCM();
      decoder_->Reset();
      return data;
    } else {
      memory_->wrap_mem_addr();
      sink_->SetStatus(notice_bit_);
      is_playing_ = false;
    }
  }

  memory_->CheckRamLimit();

  return decoder_->GetPCM();
}

inline void ADPCM::DecodeADPCMB() {
  out0_ = out1_;
  int n = (ReadRAMN() * volume_) >> kADPCMRateShift;
  out1_ = out_ + n;
  out_ = n;
}

void ADPCM::Mix(FMSample* dest, uint32_t count) {
  uint32_t maskl = control2 & 0x80 ? -1 : 0;
  uint32_t maskr = control2 & 0x40 ? -1 : 0;
  if (mask_)
    maskl = maskr = 0;

  if (is_playing_) {
    //      Log("ADPCM Play: %d   Delta_N_: %d\n", ld_, delta_n_);
    if (ld_ <= kADPCMRateMultiplier) {  // fplay < fsamp
      for (; count > 0; count--) {
        if (lc_ < 0) {
          lc_ += kADPCMRateMultiplier;
          DecodeADPCMB();
          if (!is_playing_)
            break;
        }
        int s = (lc_ * out0_ + (kADPCMRateMultiplier - lc_) * out1_) >>
                kADPCMRateShift;
        StoreSample(dest[0], s & maskl);
        StoreSample(dest[1], s & maskr);
        dest += 2;
        lc_ -= ld_;
      }
      for (; count > 0 && out0_; count--) {
        if (lc_ < 0) {
          out0_ = out1_;
          out1_ = 0;
          lc_ += kADPCMRateMultiplier;
        }
        int s = (lc_ * out1_) >> kADPCMRateShift;
        StoreSample(dest[0], s & maskl);
        StoreSample(dest[1], s & maskr);
        dest += 2;
        lc_ -= ld_;
      }
    } else {  // fplay > fsamp    (ld_ = fplay/famp*kADPCMRateMultiplier)
      int t = (-kADPCMRateMultiplier * kADPCMRateMultiplier) / ld_;
      for (; count > 0; count--) {
        int s = out0_ * (kADPCMRateMultiplier + lc_);
        while (lc_ < 0) {
          DecodeADPCMB();
          if (!is_playing_)
            goto stop;
          s -= out0_ * std::max(lc_, t);
          lc_ -= t;
        }
        lc_ -= kADPCMRateMultiplier;
        s >>= kADPCMRateShift;
        StoreSample(dest[0], s & maskl);
        StoreSample(dest[1], s & maskr);
        dest += 2;
      }
    stop:;
    }
  }
  if (!is_playing_) {
    out_ = out0_ = out1_ = 0;
    lc_ = 0;
  }
}

}  // namespace fmgen
