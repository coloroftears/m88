#ifndef DEVICES_OPNB_H_
#define DEVICES_OPNB_H_

#include "devices/adpcma.h"
#include "devices/fmgen.h"
#include "devices/opna.h"

namespace fmgen {

//  YM2610/B(OPNB) ---------------------------------------------------
class OPNB : public OPNABase {
 public:
  OPNB();
  ~OPNB() override;

  bool Init(uint32_t c,
            uint32_t r,
            bool = false,
            uint8_t* _adpcma = 0,
            int _adpcma_size = 0,
            uint8_t* _adpcmb = 0,
            int _adpcmb_size = 0);

  bool SetRate(uint32_t c, uint32_t r, bool = false);
  void Mix(FMSample* buffer, int nsamples);

  void Reset() override;
  void SetReg(uint32_t addr, uint32_t data);
  uint32_t GetReg(uint32_t addr);
  uint32_t ReadStatusEx();

  ADPCM* GetADPCM() override { return &adpcm_; }

  void SetVolumeADPCMATotal(int db);
  void SetVolumeADPCMA(int index, int db);
  void SetVolumeADPCMB(int db);

  //      void    SetChannelMask(uint32_t mask);

 private:
  void ADPCMAMix(FMSample* buffer, uint32_t count);

  // ADPCM 関係
  const uint32_t kADPCMNoticeBit = 0x8000;
  ADPCM adpcm_;

  // ADPCMA 関係
  ADPCMA adpcma[6];
  ADPCMAConfig config;

  Channel4 ch[6];
};

}  // namespace fmgen

#endif  // DEVICES_OPNB_H_
