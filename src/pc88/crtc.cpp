// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  CRTC (μPD3301) のエミュレーション
// ---------------------------------------------------------------------------
//  $Id: crtc.cpp,v 1.34 2004/02/05 11:57:49 cisc Exp $

#include "pc88/crtc.h"

#include <algorithm>

#include "common/draw.h"
#include "common/error.h"
#include "common/file.h"
#include "common/scheduler.h"
// #include "common/toast.h"
#include "pc88/config.h"
#include "pc88/pc88.h"
#include "pc88/pd8257.h"

//#define LOGNAME "crtc"
#include "common/diag.h"

namespace pc88core {

// ---------------------------------------------------------------------------
//  CRTC 部の機能
//  ・VSYNC 割り込み管理
//  ・画面位置・サイズ計算
//  ・テキスト画面生成
//  ・CGROM
//
//  カーソルブリンク間隔 16n フレーム
//
//  Status Bit
//      b0      Light Pen
//      b1      E(?)
//      b2      N(?)
//      b3      DMA under run
//      b4      Video Enable
//
//  画面イメージのビット配分
//      GMode   b4 b3 b2 b1 b0
//      カラー  -- TE TG TR TB
//      白黒    Rv TE TG TR TB
//
//  リバースの方法(XOR)
//      カラー  -- TE -- -- --
//      白黒    Rv -- -- -- --
//
//  24kHz   440 lines(25)
//          448 lines(20)
//  15kHz   256 lines(25)
//          260 lines(20)
//
#define TEXT_BIT 0x0f
#define TEXT_SET 0x08
#define TEXT_RES 0x00
#define COLOR_BIT 0x07

#define TEXT_BITP PACK(TEXT_BIT)
#define TEXT_SETP PACK(TEXT_SET)
#define TEXT_RESP PACK(TEXT_RES)

// ---------------------------------------------------------------------------
// 構築/消滅
//
CRTC::CRTC(const ID& id) : Device(id) {
  font_ = 0;
  fontrom_ = 0;
  hirarom_ = 0;
  vram_[0] = 0;
  pcgram_ = 0;
  pcgadr_ = 0;
  pcgdat_ = 0;
  pcg_enable_ = 0;
  kana_enable_ = false;
}

CRTC::~CRTC() {
  delete[] font_;
  delete[] fontrom_;
  delete[] vram_[0];
  delete[] pcgram_;
  delete[] hirarom_;
}

// ---------------------------------------------------------------------------
//  初期化
//
bool CRTC::Init(IOBus* b, Scheduler* s, PD8257* d, Draw* _draw) {
  bus_ = b, scheduler_ = s, dmac_ = d, draw_ = _draw;

  delete[] font_;
  delete[] fontrom_;
  delete[] vram_[0];
  delete[] pcgram_;

  font_ = new uint8_t[0x8000 + 0x10000];
  fontrom_ = new uint8_t[0x800];
  vram_[0] = new uint8_t[0x1e00 + 0x1e00 + 0x1400];
  pcgram_ = new uint8_t[0x400];

  if (!font_ || !fontrom_ || !vram_[0] || !pcgram_) {
    Error::SetError(Errno::OutOfMemory);
    return false;
  }
  if (!LoadFontFile()) {
    Error::SetError(Errno::LoadFontFailed);
    return false;
  }
  CreateTFont();
  CreateGFont();

  vram_[1] = vram_[0] + 0x1e00;
  attrcache_ = vram_[1] + 0x1e00;

  bank_ = 0;
  mode_ = 0;
  column_ = 0;
  SetTextMode(true);
  EnablePCG(true);

  sev_ = 0;
  return true;
}

// ---------------------------------------------------------------------------
//  IO
//
void IOCALL CRTC::Out(uint32_t port, uint32_t data) {
  Command((port & 1) != 0, data);
}

uint32_t IOCALL CRTC::In(uint32_t) {
  return Command(false, 0);
}

uint32_t IOCALL CRTC::GetStatus(uint32_t) {
  return status_;
}

// ---------------------------------------------------------------------------
//  Reset
//
void IOCALL CRTC::Reset(uint32_t, uint32_t) {
  is_15khz_ = (bus_->In(0x40) & 2) != 0;
  memcpy(pcgram_, fontrom_ + 0x400, 0x400);
  kana_mode_ = 0;
  CreateTFont();
  HotReset();
}

// ---------------------------------------------------------------------------
//  パラメータリセット
//
void CRTC::HotReset() {
  status_ = 0;  // 1

  cursor_type_ = cursormode_ = -1;
  // tvramsize = 0;
  linesize_ = 0;
  screen_width_ = 640;
  screen_height_ = 400;

  linetime_ =
      is_15khz_ ? static_cast<int>(6.258 * 8) : static_cast<int>(4.028 * 16);
  height_ = 25;
  vretrace_ = is_15khz_ ? 7 : 3;
  mode_ = clear | resize;

  pcount_[0] = 0;
  pcount_[1] = 0;

  if (sev_)
    scheduler_->DelEvent(sev_);
  StartDisplay();
}

// ---------------------------------------------------------------------------
//  グラフィックモードの変更
//
void CRTC::SetTextMode(bool color) {
  if (color) {
    pat_rev_ = PACK(0x08);
    pat_mask_ = ~PACK(0x0f);
  } else {
    pat_rev_ = PACK(0x10);
    pat_mask_ = ~PACK(0x1f);
  }
  mode_ |= refresh;
}

// ---------------------------------------------------------------------------
//  文字サイズの変更
//
void CRTC::SetTextSize(bool wide) {
  widefont_ = wide;
  memset(attrcache_, secret, 0x1400);
}

// ---------------------------------------------------------------------------
//  コマンド処理
//
uint32_t CRTC::Command(bool a0, uint32_t data) {
  static const uint32_t modetbl[8] = {
      enable | control | attribute,          // transparent b/w
      enable,                                // no attribute
      enable | color | control | attribute,  // transparent color
      0,                                     // invalid
      enable | control | nontransparent,     // non transparent b/w
      enable | nontransparent,               // non transparent b/w
      0,                                     // invalid
      0,                                     // invalid
  };

  uint32_t result = 0xff;

  Log(a0 ? "\ncmd:%.2x " : "%.2x ", data);

  if (a0)
    cmdc_ = 0, cmdm_ = data >> 5;

  switch (cmdm_) {
    case 0:  // RESET
      if (cmdc_ < 6)
        pcount_[0] = cmdc_ + 1, param0_[cmdc_] = data;
      switch (cmdc_) {
        case 0:
          status_ = 0;  // 1
          attr_ = 7 << 5;
          mode_ |= clear;
          pcount_[1] = 0;
          break;

        //  b0-b6   width-2 (char)
        //  b7      ???
        case 1:
          width_ = (data & 0x7f) + 2;
          break;

        //  b0-b5   height-1 (char)
        //  b6-b7   カーソル点滅速度 (0:16 - 3:64 frame)
        case 2:
          blink_rate_ = 32 * (1 + (data >> 6));
          height_ = (data & 0x3f) + 1;
          break;

        //  b0-b4   文字のライン数
        //  b5-b6   カーソルの種別 (b5:点滅 b6:ボックス/~アンダーライン)
        //  b7      1 行置きモード
        case 3:
          cursormode_ = (data >> 5) & 3;
          lines_per_char_ = (data & 0x1f) + 1;

          linetime_ = (is_15khz_ ? static_cast<int>(6.258 * 1024)
                                 : static_cast<int>(4.028 * 1024)) *
                      lines_per_char_ / 1024;
          if (data & 0x80)
            mode_ |= skipline;
          if (is_15khz_)
            lines_per_char_ *= 2;

          linecharlimit_ = std::min(lines_per_char_, 16U);
          break;

        //  b0-b4   Horizontal Retrace-2 (char)
        //  b5-b7   Vertical Retrace-1 (char)
        case 4:
          //          hretrace = (data & 0x1f) + 2;
          vretrace_ = ((data >> 5) & 7) + 1;
          //          linetime_ = 1667 / (height_+vretrace_-1);
          break;

        //  b0-b4   １行あたりのアトリビュート数 - 1
        //  b5-b7   テキスト画面モード
        case 5:
          mode_ &= ~(enable | color | control | attribute | nontransparent);
          mode_ |= modetbl[(data >> 5) & 7];
          attr_per_line_ = mode_ & attribute ? (data & 0x1f) + 1 : 0;
          if (attr_per_line_ + width_ > 120)
            mode_ &= ~enable;

          screen_width_ = 640;
          screen_height_ = std::min(400U, lines_per_char_ * height_);
          Log("\nscrn=(%d, %d), vrtc = %d, linetime = %d0 us, frametime0 = %d "
              "us\n",
              screen_width_, screen_height_, vretrace_, linetime_,
              linetime_ * (height_ + vretrace_));
          mode_ |= resize;
          break;
      }
      break;

    // START DISPLAY
    // b0   invert
    case 1:
      if (cmdc_ == 0) {
        pcount_[1] = 1, param1_ = data;

        linesize_ = width_ + attr_per_line_ * 2;
        int tvramsize =
            (mode_ & skipline ? (height_ + 1) / 2 : height_) * linesize_;

        Log("[%.2x]", status_);
        mode_ = (mode_ & ~inverse) | (data & 1 ? inverse : 0);
        if (mode_ & enable) {
          if (!(status_ & 0x10)) {
            status_ |= 0x10;
            scheduler_->DelEvent(sev_);
            event_ = -1;
            sev_ = scheduler_->AddEvent(
                linetime_ * vretrace_, this,
                static_cast<TimeFunc>(&CRTC::StartDisplay), 0);
          }
        } else
          status_ &= ~0x10;

        Log(" Start Display [%.2x;%.3x;%2d] vrtc %d  tvram size = %.4x ",
            status_, mode_, width_, vretrace_, tvramsize);
      }
      break;

    case 2:  // SET INTERRUPT MASK
      if (!(data & 1)) {
        mode_ |= clear;
        status_ = 0;
      }
      break;

    case 3:  // READ LIGHT PEN
      status_ &= ~1;
      break;

    case 4:  // LOAD CURSOR POSITION
      switch (cmdc_) {
        // b0   display cursor
        case 0:
          cursor_type_ = data & 1 ? cursormode_ : -1;
          break;
        case 1:
          cursor_x_ = data;
          break;
        case 2:
          cursor_y_ = data;
          break;
      }
      break;

    case 5:  // RESET INTERRUPT
      break;

    case 6:           // RESET COUNTERS
      mode_ |= clear;  // タイミングによっては
      status_ = 0;     // 消えないこともあるかも？
      break;

    default:
      break;
  }
  cmdc_++;
  return result;
}

// ---------------------------------------------------------------------------
//  フォントファイル読み込み
//
bool CRTC::LoadFontFile() {
  FileIO file;

  if (file.Open("HIRAFONT.ROM", FileIO::readonly)) {
    delete[] hirarom_;
    hirarom_ = new uint8_t[0x200];
    file.Seek(0, FileIO::begin);
    file.Read(hirarom_, 0x200);
  }

  if (file.Open("FONT.ROM", FileIO::readonly)) {
    file.Seek(0, FileIO::begin);
    file.Read(fontrom_, 0x800);
    return true;
  }
  if (file.Open("KANJI1.ROM", FileIO::readonly)) {
    file.Seek(0x1000, FileIO::begin);
    file.Read(fontrom_, 0x800);
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
//  テキストフォントから表示用フォントイメージを作成する
//  src     フォント ROM
//
void CRTC::CreateTFont() {
  CreateTFont(fontrom_, 0, 0xa0);
  CreateKanaFont();
  CreateTFont(fontrom_ + 8 * 0xe0, 0xe0, 0x20);
}

void CRTC::CreateKanaFont() {
  CreateTFont((kana_mode_ && hirarom_) ? hirarom_ : fontrom_ + 8 * 0xa0, 0xa0,
              0x40);
}

void CRTC::CreateTFont(const uint8_t* src, int idx, int num) {
  uint8_t* dest = font_ + 64 * idx;
  uint8_t* destw = font_ + 0x8000 + 128 * idx;

  for (uint32_t i = 0; i < num * 8; i++) {
    uint8_t d = *src++;
    for (uint32_t j = 0; j < 8; j++, d *= 2) {
      uint8_t b = d & 0x80 ? TEXT_SET : TEXT_RES;
      *dest++ = b;
      *destw++ = b;
      *destw++ = b;
    }
  }
}

void CRTC::ModifyFont(uint32_t off, uint32_t d) {
  uint8_t* dest = font_ + 8 * off;
  uint8_t* destw = font_ + 0x8000 + 16 * off;

  for (uint32_t j = 0; j < 8; j++, d *= 2) {
    uint8_t b = d & 0x80 ? TEXT_SET : TEXT_RES;
    *dest++ = b;
    *destw++ = b;
    *destw++ = b;
  }
  mode_ |= refresh;
}

// ---------------------------------------------------------------------------
//  セミグラフィックス用フォントを作成する
//
void CRTC::CreateGFont() {
  uint8_t* dest = font_ + 0x4000;
  uint8_t* destw = font_ + 0x10000;
  const uint8_t order[8] = {0x01, 0x10, 0x02, 0x20, 0x04, 0x40, 0x08, 0x80};

  for (int i = 0; i < 256; i++) {
    for (uint32_t j = 0; j < 8; j += 2) {
      dest[0] = dest[1] = dest[2] = dest[3] = dest[8] = dest[9] = dest[10] =
          dest[11] = destw[0] = destw[1] = destw[2] = destw[3] = destw[4] =
              destw[5] = destw[6] = destw[7] = destw[16] = destw[17] =
                  destw[18] = destw[19] = destw[20] = destw[21] = destw[22] =
                      destw[23] = i & order[j] ? TEXT_SET : TEXT_RES;

      dest[4] = dest[5] = dest[6] = dest[7] = dest[12] = dest[13] = dest[14] =
          dest[15] = destw[8] = destw[9] = destw[10] = destw[11] = destw[12] =
              destw[13] = destw[14] = destw[15] = destw[24] = destw[25] =
                  destw[26] = destw[27] = destw[28] = destw[29] = destw[30] =
                      destw[31] = i & order[j + 1] ? TEXT_SET : TEXT_RES;

      dest += 16;
      destw += 32;
    }
  }
}

// ---------------------------------------------------------------------------
//  画面表示開始のタイミング処理
//
void IOCALL CRTC::StartDisplay(uint32_t) {
  sev_ = 0;
  column_ = 0;
  mode_ &= ~suppressdisplay;
  //  Log("DisplayStart\n");
  bus_->Out(PC88::kVRTC, 0);
  if (++frametime_ > blink_rate_)
    frametime_ = 0;
  ExpandLine();
}

// ---------------------------------------------------------------------------
//  １行分取得
//
void IOCALL CRTC::ExpandLine(uint32_t) {
  int e = ExpandLineSub();
  if (e) {
    event_ = e + 1;
    sev_ = scheduler_->AddEvent(linetime_ * e, this,
                                static_cast<TimeFunc>(&CRTC::ExpandLineEnd));
  } else {
    if (++column_ < height_) {
      event_ = 1;
      sev_ = scheduler_->AddEvent(linetime_, this,
                                  static_cast<TimeFunc>(&CRTC::ExpandLine));
    } else
      ExpandLineEnd();
  }
}

int CRTC::ExpandLineSub() {
  uint8_t* dest;
  dest = vram_[bank_] + linesize_ * column_;
  if (!(mode_ & skipline) || !(column_ & 1)) {
    if (status_ & 0x10) {
      if (linesize_ > dmac_->RequestRead(dmabank, dest, linesize_)) {
        // DMA アンダーラン
        mode_ = (mode_ & ~(enable)) | clear;
        status_ = (status_ & ~0x10) | 0x08;
        memset(dest, 0, linesize_);
        Log("DMA underrun\n");
      } else {
        if (mode_ & suppressdisplay)
          memset(dest, 0, linesize_);

        if (mode_ & control) {
          bool docontrol = false;
#if 0  // XXX: 要検証
                    for (int i=1; i<=attr_per_line_; i++)
                    {
                        if ((dest[linesize_-i*2] & 0x7f) == 0x60)
                        {
                            docontrol = true;
                            break;
                        }
                    }
#else
          docontrol = (dest[linesize_ - 2] & 0x7f) == 0x60;
#endif
          if (docontrol) {
            // 特殊制御文字
            int sc = dest[linesize_ - 1];
            if (sc & 1) {
              int skip = height_ - column_ - 1;
              if (skip) {
                memset(dest + linesize_, 0, linesize_ * skip);
                return skip;
              }
            }
            if (sc & 2)
              mode_ |= suppressdisplay;
          }
        }
      }
    } else
      memset(dest, 0, linesize_);
  }
  return 0;
}

inline void IOCALL CRTC::ExpandLineEnd(uint32_t) {
  //  Log("Vertical Retrace\n");
  bus_->Out(PC88::kVRTC, 1);
  event_ = -1;
  sev_ = scheduler_->AddEvent(linetime_ * vretrace_, this,
                              static_cast<TimeFunc>(&CRTC::StartDisplay), 0);
}

// ---------------------------------------------------------------------------
//  画面サイズ変更の必要があれば変更
//
void CRTC::SetSize() {}

// ---------------------------------------------------------------------------
//  画面をイメージに展開する
//  region  更新領域
//
void CRTC::UpdateScreen(uint8_t* image,
                        int _bpl,
                        Draw::Region& region,
                        bool ref) {
  bpl_ = _bpl;
  Log("UpdateScreen:");
  if (mode_ & clear) {
    Log(" clear\n");
    mode_ &= ~(clear | refresh);
    ClearText(image);
    region.Update(0, screen_height_);
    return;
  }
  if (mode_ & resize) {
    Log(" resize");
    // 仮想画面自体の大きさを変えてしまうのが理想的だが，
    // 色々面倒なので実際はテキストマスクを貼る
    mode_ &= ~resize;
    //      draw_->Resize(screen_width_, screen_height_);
    ref = true;
  }
  if ((mode_ & refresh) || ref) {
    Log(" refresh");
    mode_ &= ~refresh;
    ClearText(image);
  }

  // Toast::Show(10, 0, "CRTC: %.2x %.2x %.2x", status_, mode_, attr_);
  if (status_ & 0x10) {
    static const uint8_t ctype[5] = {0, underline, underline, reverse, reverse};

    if ((cursor_type_ & 1) &&
        ((frametime_ <= blink_rate_ / 4) ||
         (blink_rate_ / 2 <= frametime_ && frametime_ <= 3 * blink_rate_ / 4)))
      attr_cursor_ = 0;
    else
      attr_cursor_ = ctype[1 + cursor_type_];

    attr_blink_ = frametime_ < blink_rate_ / 4 ? secret : 0;
    underlineptr_ = (lines_per_char_ - 1) * bpl_;

    Log(" update");

    //      Log("time: %d  cursor: %d(%d)  blink: %d\n", frametime_,
    //      attr_cursor_, cursor_type_, attr_blink_);
    ExpandImage(image, region);
  }
  Log("\n");
}

// ---------------------------------------------------------------------------
//  テキスト画面消去
//
void CRTC::ClearText(uint8_t* dest) {
  uint32_t y;

  //  screen_height_ = 300;
  for (y = 0; y < screen_height_; y++) {
    packed* d = reinterpret_cast<packed*>(dest);
    packed mask = pat_mask_;

    for (uint32_t x = 640 / sizeof(packed) / 4; x > 0; x--) {
      d[0] = (d[0] & mask) | TEXT_RESP;
      d[1] = (d[1] & mask) | TEXT_RESP;
      d[2] = (d[2] & mask) | TEXT_RESP;
      d[3] = (d[3] & mask) | TEXT_RESP;
      d += 4;
    }
    dest += bpl_;
  }

  packed pat0 = colorpattern[0] | TEXT_SETP;
  for (; y < 400; y++) {
    packed* d = reinterpret_cast<packed*>(dest);
    packed mask = pat_mask_;

    for (uint32_t x = 640 / sizeof(packed) / 4; x > 0; x--) {
      d[0] = (d[0] & mask) | pat0;
      d[1] = (d[1] & mask) | pat0;
      d[2] = (d[2] & mask) | pat0;
      d[3] = (d[3] & mask) | pat0;
      d += 4;
    }
    dest += bpl_;
  }
  // すべてのテキストをシークレット属性扱いにする
  memset(attrcache_, secret, 0x1400);
}

// ---------------------------------------------------------------------------
//  画面展開
//
void CRTC::ExpandImage(uint8_t* image, Draw::Region& region) {
  static const packed colorpattern[8] = {PACK(0), PACK(1), PACK(2), PACK(3),
                                         PACK(4), PACK(5), PACK(6), PACK(7)};

  uint8_t attrflag[128];

  int top = 100;
  int bottom = -1;

  int linestep = lines_per_char_ * bpl_;

  int yy = std::min(screen_height_ / lines_per_char_, height_) - 1;

  //  Log("ExpandImage Bank:%d\n", bank_);
  //  image += y * linestep;
  uint8_t* src = vram_[bank_];         // + y * linesize_;
  uint8_t* cache = vram_[bank_ ^= 1];  // + y * linesize_;
  uint8_t* cache_attr = attrcache_;    // + y * width;

  uint32_t left = 999;
  int right = -1;

  for (int y = 0; y <= yy; y++, image += linestep) {
    if (!(mode_ & skipline) || !(y & 1)) {
      attr_ &= ~(overline | underline);
      ExpandAttributes(attrflag, src + width_, y);

      int rightl = -1;
      if (widefont_) {
        for (uint32_t x = 0; x < width_; x += 2) {
          uint8_t a = attrflag[x];
          if ((src[x] ^ cache[x]) | (a ^ cache_attr[x])) {
            pat_col_ = colorpattern[(a >> 5) & 7];
            cache_attr[x] = a;
            rightl = x + 1;
            if (x < left)
              left = x;
            PutCharW((packed*)&image[8 * x], src[x], a);
          }
        }
      } else {
        for (uint32_t x = 0; x < width_; x++) {
          uint8_t a = attrflag[x];
          //                  Log("%.2x ", a);
          if ((src[x] ^ cache[x]) | (a ^ cache_attr[x])) {
            pat_col_ = colorpattern[(a >> 5) & 7];
            cache_attr[x] = a;
            rightl = x;
            if (x < left)
              left = x;
            PutChar((packed*)&image[8 * x], src[x], a);
          }
        }
        //              Log("\n");
      }
      if (rightl >= 0) {
        if (rightl > right)
          right = rightl;
        if (top == 100)
          top = y;
        bottom = y + 1;
      }
    }
    src += linesize_;
    cache += linesize_;
    cache_attr += width_;
  }
  //  Log("\n");
  region.Update(left * 8, lines_per_char_ * top, (right + 1) * 8,
                lines_per_char_ * bottom - 1);
  //  Log("Update: from %3d to %3d\n", region.top, region.bottom);
}

// ---------------------------------------------------------------------------
//  アトリビュート情報を展開
//
void CRTC::ExpandAttributes(uint8_t* dest, const uint8_t* src, uint32_t y) {
  int i;

  if (attr_per_line_ == 0) {
    memset(dest, 0xe0, 80);
    return;
  }

  // コントロールコード有効時にはアトリビュートが1組減るという
  // 記述がどこかにあったけど、嘘ですか？
  uint32_t nattrs = attr_per_line_;  // - (mode_ & control ? 1 : 0);

  // アトリビュート展開
  //  文献では 2 byte で一組となっているが、実は桁と属性は独立している模様
  //  1 byte 目は属性を反映させる桁(下位 7 bit 有効)
  //  2 byte 目は属性値
  memset(dest, 0, 80);
  for (i = 2 * (nattrs - 1); i >= 0; i -= 2)
    dest[src[i] & 0x7f] = 1;

  src++;
  for (i = 0; i < width_; i++) {
    if (dest[i])
      ChangeAttr(*src), src += 2;
    dest[i] = attr_;
  }

  // カーソルの属性を反映
  if (cursor_y_ == y && cursor_x_ < width_)
    dest[cursor_x_] ^= attr_cursor_;
}

// ---------------------------------------------------------------------------
//  アトリビュートコードを内部のフラグに変換
//
void CRTC::ChangeAttr(uint8_t code) {
  if (mode_ & color) {
    if (code & 0x8) {
      attr_ = (attr_ & 0x0f) | (code & 0xf0);
      //          attr_ ^= mode_ & inverse;
    } else {
      attr_ = (attr_ & 0xf0) | ((code >> 2) & 0xd) | ((code & 1) << 1);
      attr_ ^= mode_ & inverse;
      attr_ ^= ((code & 2) && !(code & 1)) ? attr_blink_ : 0;
    }
  } else {
    attr_ =
        0xe0 | ((code >> 2) & 0x0d) | ((code & 1) << 1) | ((code & 0x80) >> 3);
    attr_ ^= mode_ & inverse;
    attr_ ^= ((code & 2) && !(code & 1)) ? attr_blink_ : 0;
  }
}

// ---------------------------------------------------------------------------
//  フォントのアドレスを取得
//
inline const uint8_t* CRTC::GetFont(uint32_t c) {
  return font_ + c * 64;
}

// ---------------------------------------------------------------------------
//  フォント(40文字)のアドレスを取得
//
inline const uint8_t* CRTC::GetFontW(uint32_t c) {
  return font_ + 0x8000 + c * 128;
}

// ---------------------------------------------------------------------------
//  テキスト表示
//
inline void CRTC::PutChar(packed* dest, uint8_t ch, uint8_t attr) {
  const packed* src =
      (const packed*)GetFont(((attr << 4) & 0x100) + (attr & secret ? 0 : ch));

  if (attr & reverse)
    PutReversed(dest, src), PutLineReversed(dest, attr);
  else
    PutNormal(dest, src), PutLineNormal(dest, attr);
}

#define NROW (bpl_ / sizeof(packed))
#define DRAW(dest, data) (dest) = ((dest)&pat_mask_) | (data)

// ---------------------------------------------------------------------------
//  普通のテキスト文字
//
void CRTC::PutNormal(packed* dest, const packed* src) {
  uint32_t h;

  for (h = 0; h < linecharlimit_; h += 2) {
    packed x = *src++ | pat_col_;
    packed y = *src++ | pat_col_;

    DRAW(dest[0], x);
    DRAW(dest[1], y);
    DRAW(dest[NROW + 0], x);
    DRAW(dest[NROW + 1], y);
    dest += bpl_ * 2 / sizeof(packed);
  }
  packed p = pat_col_ | TEXT_RESP;
  for (; h < lines_per_char_; h++) {
    DRAW(dest[0], p);
    DRAW(dest[1], p);
    dest += bpl_ / sizeof(packed);
  }
}

// ---------------------------------------------------------------------------
//  テキスト反転表示
//
void CRTC::PutReversed(packed* dest, const packed* src) {
  uint32_t h;

  for (h = 0; h < linecharlimit_; h += 2) {
    packed x = (*src++ ^ pat_rev_) | pat_col_;
    packed y = (*src++ ^ pat_rev_) | pat_col_;

    DRAW(dest[0], x);
    DRAW(dest[1], y);
    DRAW(dest[NROW + 0], x);
    DRAW(dest[NROW + 1], y);
    dest += bpl_ * 2 / sizeof(packed);
  }

  packed p = pat_col_ ^ pat_rev_;
  for (; h < lines_per_char_; h++) {
    DRAW(dest[0], p);
    DRAW(dest[1], p);
    dest += bpl_ / sizeof(packed);
  }
}

// ---------------------------------------------------------------------------
//  オーバーライン、アンダーライン表示
//
void CRTC::PutLineNormal(packed* dest, uint8_t attr) {
  packed d = pat_col_ | TEXT_SETP;
  if (attr & overline)  // overline
  {
    DRAW(dest[0], d);
    DRAW(dest[1], d);
  }
  if ((attr & underline) && lines_per_char_ > 14) {
    dest = (packed*)(((uint8_t*)dest) + underlineptr_);
    DRAW(dest[0], d);
    DRAW(dest[1], d);
  }
}

void CRTC::PutLineReversed(packed* dest, uint8_t attr) {
  packed d = (pat_col_ | TEXT_SETP) ^ pat_rev_;
  if (attr & overline) {
    DRAW(dest[0], d);
    DRAW(dest[1], d);
  }
  if ((attr & underline) && lines_per_char_ > 14) {
    dest = (packed*)(((uint8_t*)dest) + underlineptr_);
    DRAW(dest[0], d);
    DRAW(dest[1], d);
  }
}

// ---------------------------------------------------------------------------
//  テキスト表示(40 文字モード)
//
inline void CRTC::PutCharW(packed* dest, uint8_t ch, uint8_t attr) {
  const packed* src =
      (const packed*)GetFontW(((attr << 4) & 0x100) + (attr & secret ? 0 : ch));

  if (attr & reverse)
    PutReversedW(dest, src), PutLineReversedW(dest, attr);
  else
    PutNormalW(dest, src), PutLineNormalW(dest, attr);
}

// ---------------------------------------------------------------------------
//  普通のテキスト文字
//
void CRTC::PutNormalW(packed* dest, const packed* src) {
  uint32_t h;
  packed x, y;

  for (h = 0; h < linecharlimit_; h += 2) {
    x = *src++ | pat_col_;
    y = *src++ | pat_col_;
    DRAW(dest[0], x);
    DRAW(dest[1], y);
    DRAW(dest[NROW + 0], x);
    DRAW(dest[NROW + 1], y);

    x = *src++ | pat_col_;
    y = *src++ | pat_col_;
    DRAW(dest[2], x);
    DRAW(dest[3], y);
    DRAW(dest[NROW + 2], x);
    DRAW(dest[NROW + 3], y);
    dest += bpl_ * 2 / sizeof(packed);
  }
  x = pat_col_ | TEXT_RESP;
  for (; h < lines_per_char_; h++) {
    DRAW(dest[0], x);
    DRAW(dest[1], x);
    DRAW(dest[2], x);
    DRAW(dest[3], x);
    dest += bpl_ / sizeof(packed);
  }
}

// ---------------------------------------------------------------------------
//  テキスト反転表示
//
void CRTC::PutReversedW(packed* dest, const packed* src) {
  uint32_t h;
  packed x, y;

  for (h = 0; h < linecharlimit_; h += 2) {
    x = (*src++ ^ pat_rev_) | pat_col_;
    y = (*src++ ^ pat_rev_) | pat_col_;
    DRAW(dest[0], x);
    DRAW(dest[1], y);
    DRAW(dest[NROW + 0], x);
    DRAW(dest[NROW + 1], y);

    x = (*src++ ^ pat_rev_) | pat_col_;
    y = (*src++ ^ pat_rev_) | pat_col_;
    DRAW(dest[2], x);
    DRAW(dest[3], y);
    DRAW(dest[NROW + 2], x);
    DRAW(dest[NROW + 3], y);

    dest += bpl_ * 2 / sizeof(packed);
  }

  x = pat_col_ ^ pat_rev_;
  for (; h < lines_per_char_; h++) {
    DRAW(dest[0], x);
    DRAW(dest[1], x);
    DRAW(dest[2], x);
    DRAW(dest[3], x);
    dest += bpl_ / sizeof(packed);
  }
}

// ---------------------------------------------------------------------------
//  オーバーライン、アンダーライン表示
//
void CRTC::PutLineNormalW(packed* dest, uint8_t attr) {
  packed d = pat_col_ | TEXT_SETP;
  if (attr & overline)  // overline
  {
    DRAW(dest[0], d);
    DRAW(dest[1], d);
    DRAW(dest[2], d);
    DRAW(dest[3], d);
  }
  if ((attr & underline) && lines_per_char_ > 14) {
    dest = (packed*)(((uint8_t*)dest) + underlineptr_);
    DRAW(dest[0], d);
    DRAW(dest[1], d);
    DRAW(dest[2], d);
    DRAW(dest[3], d);
  }
}

void CRTC::PutLineReversedW(packed* dest, uint8_t attr) {
  packed d = (pat_col_ | TEXT_SETP) ^ pat_rev_;
  if (attr & overline) {
    DRAW(dest[0], d);
    DRAW(dest[1], d);
    DRAW(dest[2], d);
    DRAW(dest[3], d);
  }
  if ((attr & underline) && lines_per_char_ > 14) {
    dest = (packed*)(((uint8_t*)dest) + underlineptr_);
    DRAW(dest[0], d);
    DRAW(dest[1], d);
    DRAW(dest[2], d);
    DRAW(dest[3], d);
  }
}

// ---------------------------------------------------------------------------
//  OUT
//
void IOCALL CRTC::PCGOut(uint32_t p, uint32_t d) {
  switch (p) {
    case 0:
      pcgdat_ = d;
      break;
    case 1:
      pcgadr_ = (pcgadr_ & 0xff00) | d;
      break;
    case 2:
      pcgadr_ = (pcgadr_ & 0x00ff) | (d << 8);
      break;
  }

  if (pcgadr_ & 0x1000) {
    uint32_t tmp =
        (pcgadr_ & 0x2000) ? fontrom_[0x400 + (pcgadr_ & 0x3ff)] : pcgdat_;
    Log("PCG: %.4x <- %.2x\n", pcgadr_, tmp);
    pcgram_[pcgadr_ & 0x3ff] = tmp;
    if (pcg_enable_)
      ModifyFont(0x400 + (pcgadr_ & 0x3ff), tmp);
  }
}

// ---------------------------------------------------------------------------
//  OUT
//
void CRTC::EnablePCG(bool enable) {
  pcg_enable_ = enable;
  if (!pcg_enable_) {
    CreateTFont();
    mode_ |= refresh;
  } else {
    for (int i = 0; i < 0x400; i++)
      ModifyFont(0x400 + i, pcgram_[i]);
  }
}

// ---------------------------------------------------------------------------
//  OUT 33H (80SR)
//  bit4 = ひらがな(1)・カタカナ(0)選択
//
void IOCALL CRTC::SetKanaMode(uint32_t, uint32_t data) {
  if (kana_enable_)
    data &= 0x10;
  else
    data = 0;

  if (data != kana_mode_) {
    kana_mode_ = data;
    CreateKanaFont();
    mode_ |= refresh;
  }
}

// ---------------------------------------------------------------------------
//  apply config
//
void CRTC::ApplyConfig(const Config* cfg) {
  kana_enable_ = cfg->basicmode == Config::N80V2;
  EnablePCG((cfg->flags & Config::kEnablePCG) != 0);
}

// ---------------------------------------------------------------------------
//  table
//
const packed CRTC::colorpattern[8] = {PACK(0), PACK(1), PACK(2), PACK(3),
                                      PACK(4), PACK(5), PACK(6), PACK(7)};

// ---------------------------------------------------------------------------
//  状態保存
//
uint32_t IFCALL CRTC::GetStatusSize() {
  return sizeof(Status);
}

bool IFCALL CRTC::SaveStatus(uint8_t* s) {
  Log("*** Save Status\n");
  Status* st = (Status*)s;

  st->rev = ssrev;
  st->cmdm = cmdm_;
  st->cmdc = std::max(cmdc_, 0xff);
  memcpy(st->pcount, pcount_, sizeof(pcount_));
  memcpy(st->param0, param0_, sizeof(param0_));
  st->param1 = param1_;
  st->cursor_x = cursor_x_;
  st->cursor_y = cursor_y_;
  st->cursor_t = cursor_type_;
  st->attr = attr_;
  st->column = column_;
  st->mode = mode_;
  st->status = status_;
  st->event = event_;
  st->color = (pat_rev_ == PACK(0x08));
  return true;
}

bool IFCALL CRTC::LoadStatus(const uint8_t* s) {
  Log("*** Load Status\n");
  const Status* st = (const Status*)s;
  if (st->rev < 1 || ssrev < st->rev)
    return false;
  int i;
  for (i = 0; i < st->pcount[0]; i++)
    Out(i ? 0 : 1, st->param0[i]);
  if (st->pcount[1])
    Out(1, st->param1);
  cmdm_ = st->cmdm, cmdc_ = st->cmdc;
  cursor_x_ = st->cursor_x;
  cursor_y_ = st->cursor_y;
  cursor_type_ = st->cursor_t;
  attr_ = st->attr;
  column_ = st->column;
  mode_ = st->mode;
  status_ = st->status | clear;
  event_ = st->event;
  SetTextMode(st->color);

  scheduler_->DelEvent(sev_);
  if (event_ == 1)
    sev_ = scheduler_->AddEvent(linetime_, this,
                                static_cast<TimeFunc>(&CRTC::ExpandLine));
  else if (event_ > 1)
    sev_ = scheduler_->AddEvent(linetime_ * (event_ - 1), this,
                                static_cast<TimeFunc>(&CRTC::ExpandLineEnd));
  else if (event_ == -1 || st->rev == 1)
    sev_ = scheduler_->AddEvent(linetime_ * vretrace_, this,
                                static_cast<TimeFunc>(&CRTC::StartDisplay), 0);

  return true;
}

// ---------------------------------------------------------------------------
//  device description
//
const Device::Descriptor CRTC::descriptor = {indef, outdef};

const Device::OutFuncPtr CRTC::outdef[] = {
    static_cast<Device::OutFuncPtr>(&CRTC::Reset),
    static_cast<Device::OutFuncPtr>(&CRTC::Out),
    static_cast<Device::OutFuncPtr>(&CRTC::PCGOut),
    static_cast<Device::OutFuncPtr>(&CRTC::SetKanaMode),
};

const Device::InFuncPtr CRTC::indef[] = {
    static_cast<Device::InFuncPtr>(&CRTC::In),
    static_cast<Device::InFuncPtr>(&CRTC::GetStatus),
};
}  // namespace pc88core