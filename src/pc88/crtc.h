// ---------------------------------------------------------------------------
//  M88 - PC-88 Emulator.
//  Copyright (C) cisc 1998.
// ---------------------------------------------------------------------------
//  CRTC (μPD3301) のエミュレーション
// ---------------------------------------------------------------------------
//  $Id: crtc.h,v 1.19 2002/04/07 05:40:09 cisc Exp $

#pragma once

#include "common/device.h"
#include "common/draw.h"
#include "common/scheduler.h"
#include "common/types.h"

namespace pc88core {
class PD8257;
class Config;

// ---------------------------------------------------------------------------
//  CRTC (μPD3301) 及びテキスト画面合成
//
class CRTC final : public Device {
 public:
  enum IDOut { kReset, kOut, kPCGOut, kSetKanaMode };
  enum IDIn { kIn, kGetStatus };

 public:
  explicit CRTC(const ID& id);
  ~CRTC();
  bool Init(IOBus* bus, Scheduler* s, PD8257* dmac, Draw* draw);

  void UpdateScreen(uint8_t* image,
                    int bpl,
                    Draw::Region& region,
                    bool refresh);
  void SetSize();
  void ApplyConfig(const Config* config);
  SchedTimeDelta GetFramePeriod();

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final;
  bool IFCALL SaveStatus(uint8_t* status) final;
  bool IFCALL LoadStatus(const uint8_t* status) final;

  // CRTC Control
  void IOCALL Reset(uint32_t = 0, uint32_t = 0);
  void IOCALL Out(uint32_t, uint32_t data);
  uint32_t IOCALL In(uint32_t = 0);
  uint32_t IOCALL GetStatus(uint32_t = 0);
  void IOCALL PCGOut(uint32_t, uint32_t);
  void IOCALL SetKanaMode(uint32_t, uint32_t);

  void SetTextMode(bool color);
  void SetTextSize(bool wide);

 private:
  enum Mode {
    inverse = 1 << 0,  // reverse bit と同じ
    color = 1 << 1,
    control = 1 << 2,
    skipline = 1 << 3,
    nontransparent = 1 << 4,
    attribute = 1 << 5,
    clear = 1 << 6,
    refresh = 1 << 7,
    enable = 1 << 8,
    suppressdisplay = 1 << 9,
    resize = 1 << 10,
  };

  //  ATTR BIT 配置       G  R  B  CG UL OL SE RE
  enum TextAttr {
    reverse = 1 << 0,
    secret = 1 << 1,
    overline = 1 << 2,
    underline = 1 << 3,
    graphics = 1 << 4,
  };

  enum {
    dmabank = 2,
  };

 private:
  enum {
    ssrev = 2,
  };
  struct Status {
    uint8_t rev;
    uint8_t cmdm;
    uint8_t cmdc;
    uint8_t pcount[2];
    uint8_t param0[6];
    uint8_t param1;
    uint8_t cursor_x, cursor_y;
    int8_t cursor_t;
    uint8_t mode;
    uint8_t status;
    uint8_t column;
    uint8_t attr;
    uint8_t event;
    bool color;
  };

  void HotReset();
  bool LoadFontFile();
  void CreateTFont();
  void CreateTFont(const uint8_t*, int, int);
  void CreateKanaFont();
  void CreateGFont();
  uint32_t Command(bool a0, uint32_t data);

  void IOCALL StartDisplay(uint32_t = 0);
  void IOCALL ExpandLine(uint32_t = 0);
  void IOCALL ExpandLineEnd(uint32_t = 0);
  int ExpandLineSub();

  void ClearText(uint8_t* image);
  void ExpandImage(uint8_t* image, Draw::Region& region);
  void ExpandAttributes(uint8_t* dest, const uint8_t* src, uint32_t y);
  void ChangeAttr(uint8_t code);

  const uint8_t* GetFont(uint32_t c);
  const uint8_t* GetFontW(uint32_t c);
  void ModifyFont(uint32_t off, uint32_t d);
  void EnablePCG(bool);

  void PutChar(packed* dest, uint8_t c, uint8_t a);
  void PutNormal(packed* dest, const packed* src);
  void PutReversed(packed* dest, const packed* src);
  void PutLineNormal(packed* dest, uint8_t attr);
  void PutLineReversed(packed* dest, uint8_t attr);

  void PutCharW(packed* dest, uint8_t c, uint8_t a);
  void PutNormalW(packed* dest, const packed* src);
  void PutReversedW(packed* dest, const packed* src);
  void PutLineNormalW(packed* dest, uint8_t attr);
  void PutLineReversedW(packed* dest, uint8_t attr);

  IOBus* bus_ = nullptr;
  PD8257* dmac_ = nullptr;
  Scheduler* scheduler_ = nullptr;
  SchedulerEvent* sev_ = nullptr;
  Draw* draw_ = nullptr;

  int cmdm_;
  int cmdc_;

  uint32_t cursormode_ = 0;
  uint32_t linesize_ = 0;

  bool is_15khz_ = false;

  uint8_t attr_;
  uint8_t attr_cursor_;
  uint8_t attr_blink_;
  uint32_t status_;
  uint32_t column_;
  SchedTimeDelta linetime_;
  uint32_t frametime_;
  uint32_t pcgadr_;
  uint32_t pcgdat_;

  int bpl_;
  packed pat_col_;
  packed pat_mask_;
  packed pat_rev_;
  int underlineptr_;

  uint8_t* fontrom_;
  uint8_t* hirarom_;
  uint8_t* font_;
  uint8_t* pcgram_;
  uint8_t* vram_[2];
  uint8_t* attrcache_;

  uint32_t bank_;          // VRAM Cache のバンク
  //uint32_t tvramsize;     // 1画面のテキストサイズ
  uint32_t screen_width_;   // 画面の幅
  uint32_t screen_height_;  // 画面の高さ

  uint32_t cursor_x_;  // カーソル位置
  uint32_t cursor_y_;
  uint32_t attr_per_line_;    // 1行あたりのアトリビュート数
  uint32_t linecharlimit_;  // 1行あたりのテキスト高さ
  uint32_t lines_per_char_;   // 1行のドット数
  uint32_t width_;          // テキスト画面の幅
  uint32_t height_;         // テキスト画面の高さ
  uint32_t blink_rate_;      // ブリンクの速度
  int cursor_type_;         // b0:blink, b1:underline (-1=none)
  uint32_t vretrace_;       //

  uint32_t mode_;

  bool widefont_;
  bool pcg_enable_;
  bool kana_enable_;   // ひらカナ選択有効
  uint8_t kana_mode_;  // b4 = ひらがなモード

  uint8_t pcount_[2];
  uint8_t param0_[6];
  uint8_t param1_;
  uint8_t event_;

 private:
  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];

  static const packed colorpattern[8];
};

// ---------------------------------------------------------------------------
//  1 フレーム分に相当する時間を求める
//
inline SchedTimeDelta CRTC::GetFramePeriod() {
  return linetime_ * (height_ + vretrace_);
}
}  // namespace pc88core
