// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1997, 2000.
// ---------------------------------------------------------------------------
//  $Id: tapemgr.cpp,v 1.3 2000/08/06 09:58:51 cisc Exp $

#include "pc88/tape_manager.h"

#include <string.h>

#include <algorithm>

#include "common/file.h"
#include "common/scheduler.h"
#include "common/toast.h"

#define LOGNAME "tape"
#include "common/diag.h"

#define T88ID "PC-8801 Tape Image(T88)"

namespace pc88core {

// ---------------------------------------------------------------------------
//  構築
//
TapeManager::TapeManager()
    : Device(DEV_ID('T', 'A', 'P', 'E')), scheduler_(0), tags_(0) {
  datasize_ = 0;
  datatype_ = 0;
  pinput_ = 0;
  pos_ = 0;
  bus_ = 0;
  offset_ = 0;
  mode_ = T_BLANK;
  motor_ = false;
}

// ---------------------------------------------------------------------------
//  破棄
//
TapeManager::~TapeManager() {
  Close();
}

// ---------------------------------------------------------------------------
//  初期化
//
bool TapeManager::Init(Scheduler* s, IOBus* b, int pi) {
  scheduler_ = s;
  bus_ = b;
  pinput_ = pi;

  motor_ = false;
  timercount_ = 0;
  timerremain_ = 0;
  tick_ = 0;
  return true;
}

// ---------------------------------------------------------------------------
//  T88 を開く
//
bool TapeManager::Open(const char* file) {
  Close();

  FileIO fio;
  if (!fio.Open(file, FileIO::readonly))
    return false;

  // ヘッダ確認
  char buf[24];
  fio.Read(buf, 24);
  if (memcmp(buf, T88ID, 24))
    return false;

  // タグのリスト構造を展開
  Tag* prv = 0;
  do {
    TagHdr hdr;
    fio.Read(&hdr, 4);

    Tag* tag = (Tag*)new uint8_t[sizeof(Tag) - 1 + hdr.length];
    if (!tag) {
      Close();
      return false;
    }

    tag->prev = prv;
    tag->next = 0;
    (prv ? prv->next : tags_) = tag;
    tag->id = hdr.id;
    tag->length = hdr.length;
    fio.Read(tag->data, tag->length);
    prv = tag;
  } while (prv->id);

  if (!Rewind())
    return false;
  return true;
}

// ---------------------------------------------------------------------------
//  とじる
//
bool TapeManager::Close() {
  if (scheduler_)
    SetTimer(0);
  while (tags_) {
    Tag* n = tags_->next;
    delete[] tags_;
    tags_ = n;
  }
  return true;
}

// ---------------------------------------------------------------------------
//  まきもどす
//
bool TapeManager::Rewind(bool timer) {
  pos_ = tags_;
  if (event_) {
    scheduler_->DelEvent(event_);
    event_ = nullptr;
  }
  if (pos_) {
    tick_ = 0;

    // バージョン確認
    // 最初のタグはバージョンタグになるはず？
    if (pos_->id != T_VERSION || pos_->length < 2 ||
        *(uint16_t*)pos_->data != T88VER)
      return false;

    pos_ = pos_->next;
    Proceed(timer);
  }
  return true;
}

// ---------------------------------------------------------------------------
//  モータ
//
bool TapeManager::Motor(bool s) {
  if (motor_ == s)
    return true;
  if (s) {
    Toast::Show(10, 2000, "Motor on: %d %d", timerremain_, timercount_);
    time_ = scheduler_->GetTime();
    if (timerremain_)
      event_ = scheduler_->AddEvent(timercount_ * 125 / 6, this,
                                    static_cast<TimeFunc>(&TapeManager::Timer));
    motor_ = true;
  } else {
    if (timercount_) {
      SchedTimeDelta td = (scheduler_->GetTime() - time_) * 6 / 125;
      timerremain_ = std::max(10U, timerremain_ - td);
      if (event_) {
        scheduler_->DelEvent(event_);
        event_ = nullptr;
      }
      Toast::Show(10, 2000, "Motor off: %d %d", timerremain_, timercount_);
    }
    motor_ = false;
  }
  return true;
}

// ---------------------------------------------------------------------------

uint32_t TapeManager::GetPos() {
  if (motor_) {
    if (timercount_)
      return tick_ + (scheduler_->GetTime() - time_) * 6 / 125;
    else
      return tick_;
  } else {
    return tick_ + timercount_ - timerremain_;
  }
}

// ---------------------------------------------------------------------------
//  タグを処理
//
void TapeManager::Proceed(bool timer) {
  while (pos_) {
    Log("TAG %d\n", pos_->id);
    switch (pos_->id) {
      case T_END:
        mode_ = T_BLANK;
        pos_ = 0;
        Toast::Show(50, 0, "end of tape", tick_);
        timercount_ = 0;
        return;

      case T_BLANK:
      case T_SPACE:
      case T_MARK: {
        BlankTag* t = (BlankTag*)pos_->data;
        mode_ = (Mode)pos_->id;

        if (t->pos + t->tick - tick_ <= 0)
          break;

        if (timer)
          SetTimer(t->pos + t->tick - tick_);
        else
          timercount_ = t->pos + t->tick - tick_;

        pos_ = pos_->next;
        return;
      }

      case T_DATA: {
        DataTag* t = (DataTag*)pos_->data;
        mode_ = T_DATA;

        data_ = t->data;
        datasize_ = t->length;
        datatype_ = t->type;
        offset_ = 0;

        if (!datasize_)
          break;

        pos_ = pos_->next;
        if (timer)
          SetTimer(datatype_ & 0x100 ? 44 : 88);
        else
          timercount_ = t->tick;
        return;
      }
    }
    pos_ = pos_->next;
  }
}

// ---------------------------------------------------------------------------
//
//
void IOCALL TapeManager::Timer(uint32_t) {
  tick_ += timercount_;
  Toast::Show(50, 0, "tape: %d", tick_);

  if (mode_ == T_DATA) {
    Send(*data_++);
    offset_++;
    if (--datasize_ > 0) {
      SetTimer(datatype_ & 0x100 ? 44 : 88);
      return;
    }
    Log("\n");
  }
  Proceed();
}

// ---------------------------------------------------------------------------
//  キャリア確認
//
bool TapeManager::Carrier() {
  if (mode_ == T_MARK) {
    Log("*");
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
//  タイマー更新
//
void TapeManager::SetTimer(int count) {
  if (count > 100)
    Log("Timer: %d\n", count);
  if (event_) {
    scheduler_->DelEvent(event_);
    event_ = nullptr;
  }
  timercount_ = count;
  if (motor_) {
    time_ = scheduler_->GetTime();
    if (count)  // 100000/4800
      event_ = scheduler_->AddEvent(count * 125 / 6, this,
                                    static_cast<TimeFunc>(&TapeManager::Timer));
  } else
    timerremain_ = count;
}

// ---------------------------------------------------------------------------
//  バイト転送
//
inline void TapeManager::Send(uint32_t byte) {
  Log("%.2x ", byte);
  bus_->Out(pinput_, byte);
}

// ---------------------------------------------------------------------------
//  即座にデータを要求する
//
void TapeManager::RequestData(uint32_t, uint32_t) {
  if (mode_ == T_DATA) {
    const SchedTimeDelta kImmediate = 1;
    scheduler_->DelEvent(event_);
    event_ = scheduler_->AddEvent(kImmediate, this,
                                  static_cast<TimeFunc>(&TapeManager::Timer));
  }
}

// ---------------------------------------------------------------------------
//  シークする
//
bool TapeManager::Seek(uint32_t newpos, uint32_t off) {
  if (!Rewind(false))
    return false;

  while (pos_ && (tick_ + timercount_) < newpos) {
    tick_ += timercount_;
    Proceed(false);
  }
  if (!pos_)
    return false;

  switch (pos_->prev->id) {
    int l;

    case T_BLANK:
    case T_SPACE:
    case T_MARK:
      mode_ = (Mode)pos_->prev->id;
      l = tick_ + timercount_ - newpos;
      tick_ = newpos;
      SetTimer(l);
      break;

    case T_DATA:
      mode_ = T_DATA;
      offset_ = off;
      newpos = tick_ + offset_ * (datatype_ ? 44 : 88);
      data_ += offset_;
      datasize_ -= offset_;
      SetTimer(datatype_ ? 44 : 88);
      break;

    default:
      return false;
  }
  return true;
}

// ---------------------------------------------------------------------------

void IOCALL TapeManager::Out30(uint32_t, uint32_t d) {
  Motor(!!(d & 8));
}

uint32_t IOCALL TapeManager::In40(uint32_t) {
  return IOBus::Active(Carrier() ? 4 : 0, 4);
}

// ---------------------------------------------------------------------------
//  状態保存
//
uint32_t IFCALL TapeManager::GetStatusSize() {
  return sizeof(Status);
}

bool IFCALL TapeManager::SaveStatus(uint8_t* s) {
  Status* status = (Status*)s;
  status->rev = SSREV;
  status->motor = motor_;
  status->pos = GetPos();
  status->offset = offset_;
  Toast::Show(0, 1000, "tapesave: %d", status->pos);
  return true;
}

bool IFCALL TapeManager::LoadStatus(const uint8_t* s) {
  const Status* status = (const Status*)s;
  if (status->rev != SSREV)
    return false;
  motor_ = status->motor;
  Seek(status->pos, status->offset);
  Toast::Show(0, 1000, "tapesave: %d", GetPos());
  return true;
}

// ---------------------------------------------------------------------------
//  device description
//
const Device::Descriptor TapeManager::descriptor = {TapeManager::indef,
                                                    TapeManager::outdef};

const Device::OutFuncPtr TapeManager::outdef[] = {
    static_cast<Device::OutFuncPtr>(&TapeManager::RequestData),
    static_cast<Device::OutFuncPtr>(&TapeManager::Out30),
};

const Device::InFuncPtr TapeManager::indef[] = {
    static_cast<Device::InFuncPtr>(&TapeManager::In40),
};

}  // namespace pc88core
