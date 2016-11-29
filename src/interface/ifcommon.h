// ----------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//  拡張モジュール用インターフェース定義
// ----------------------------------------------------------------------------
//  $Id: ifcommon.h,v 1.8 2002/04/07 05:40:09 cisc Exp $

#pragma once

#include <windows.h>

#include "common/types.h"

#define RELEASE(r) \
  if (r)           \
  r->Release(), r = 0

// ----------------------------------------------------------------------------
//
//
struct IUnk {
  virtual int32_t IFCALL QueryInterface(REFIID, void**) = 0;
  virtual uint32_t IFCALL AddRef() = 0;
  virtual uint32_t IFCALL Release() = 0;
};

// ----------------------------------------------------------------------------
//  音源のインターフェース
//
struct ISoundControl;
struct ISoundSource {
  virtual bool IFCALL Connect(ISoundControl* sc) = 0;
  virtual bool IFCALL SetRate(uint32_t rate) = 0;
  virtual void IFCALL Mix(int32_t* s, int length) = 0;
};

// ----------------------------------------------------------------------------
//  音源制御のインターフェース
//
struct ISoundControl {
  virtual bool IFCALL Connect(ISoundSource* src) = 0;
  virtual bool IFCALL Disconnect(ISoundSource* src) = 0;

  virtual bool IFCALL Update(ISoundSource* src) = 0;
  virtual int IFCALL GetSubsampleTime(ISoundSource* src) = 0;
};

// ----------------------------------------------------------------------------
//  メモリ管理のインターフェース
//
struct IMemoryManager {
  virtual int IFCALL Connect(void* inst, bool highpriority = false) = 0;
  virtual bool IFCALL Disconnect(uint32_t pid) = 0;

  virtual bool IFCALL AllocR(uint32_t pid,
                             uint32_t addr,
                             uint32_t length,
                             uint8_t* ptr) = 0;
  virtual bool IFCALL AllocR(uint32_t pid,
                             uint32_t addr,
                             uint32_t length,
                             uint32_t(MEMCALL*)(void*, uint32_t)) = 0;
  virtual bool IFCALL ReleaseR(uint32_t pid,
                               uint32_t addr,
                               uint32_t length) = 0;
  virtual uint32_t IFCALL Read8P(uint32_t pid, uint32_t addr) = 0;

  virtual bool IFCALL AllocW(uint32_t pid,
                             uint32_t addr,
                             uint32_t length,
                             uint8_t* ptr) = 0;
  virtual bool IFCALL AllocW(uint32_t pid,
                             uint32_t addr,
                             uint32_t length,
                             void(MEMCALL*)(void*, uint32_t, uint32_t)) = 0;
  virtual bool IFCALL ReleaseW(uint32_t pid,
                               uint32_t addr,
                               uint32_t length) = 0;
  virtual void IFCALL Write8P(uint32_t pid, uint32_t addr, uint32_t data) = 0;
};

// ----------------------------------------------------------------------------
//  メモリ空間にアクセスするためのインターフェース
//
struct IMemoryAccess {
  virtual uint32_t IFCALL Read8(uint32_t addr) = 0;
  virtual void IFCALL Write8(uint32_t addr, uint32_t data) = 0;
};

// ----------------------------------------------------------------------------
//  IO 空間にアクセスするためのインターフェース
//
struct IIOAccess {
  virtual uint32_t IFCALL In(uint32_t port) = 0;
  virtual void IFCALL Out(uint32_t port, uint32_t data) = 0;
};

// ----------------------------------------------------------------------------
//  デバイスのインターフェース
//
struct IDevice {
  typedef uint32_t ID;
  typedef uint32_t (IOCALL IDevice::*InFuncPtr)(uint32_t port);
  typedef void (IOCALL IDevice::*OutFuncPtr)(uint32_t port, uint32_t data);
  typedef void (IOCALL IDevice::*TimeFunc)(uint32_t arg);
  struct Descriptor {
    const InFuncPtr* indef;
    const OutFuncPtr* outdef;
  };

  virtual const ID& IFCALL GetID() const = 0;
  virtual const Descriptor* IFCALL GetDesc() const = 0;
  virtual uint32_t IFCALL GetStatusSize() = 0;
  virtual bool IFCALL LoadStatus(const uint8_t* status) = 0;
  virtual bool IFCALL SaveStatus(uint8_t* status) = 0;
};

// ----------------------------------------------------------------------------
//  IO 空間にデバイスを接続するためのインターフェース
//
struct IIOBus {
  enum ConnectRule {
    end = 0,
    portin = 1,
    portout = 2,
    sync = 4,
  };
  struct Connector {
    uint16_t bank;
    uint8_t rule;
    uint8_t id;
  };

  virtual bool IFCALL Connect(IDevice* device, const Connector* connector) = 0;
  virtual bool IFCALL Disconnect(IDevice* device) = 0;
};

// ----------------------------------------------------------------------------
//  タイマー管理のためのインターフェース
//
struct SchedulerEvent;

struct IScheduler {
  typedef SchedulerEvent* Handle;

  virtual Handle IFCALL AddEvent(int count,
                                 IDevice* dev,
                                 IDevice::TimeFunc func,
                                 int arg = 0,
                                 bool repeat = false) = 0;
  virtual void IFCALL SetEvent(Handle ev,
                               int count,
                               IDevice* dev,
                               IDevice::TimeFunc func,
                               int arg = 0,
                               bool repeat = false) = 0;
  virtual bool IFCALL DelEvent(IDevice* dev) = 0;
  virtual bool IFCALL DelEvent(Handle ev) = 0;
};

// ----------------------------------------------------------------------------
//  システム内時間取得のためのインターフェース
//
struct ITime {
  virtual int IFCALL GetTime() = 0;
};

// ----------------------------------------------------------------------------
//  より精度の高い時間を取得するためのインターフェース
//
struct ICPUTime {
  virtual uint32_t IFCALL GetCPUTick() = 0;
  virtual uint32_t IFCALL GetCPUSpeed() = 0;
};

// ----------------------------------------------------------------------------
//  システム内のインターフェースに接続するためのインターフェース
//
struct ISystem {
  virtual void* IFCALL QueryIF(REFIID iid) = 0;
};

// ----------------------------------------------------------------------------
//  モジュールの基本インターフェース
//
struct IModule {
  virtual void IFCALL Release() = 0;
  virtual void* IFCALL QueryIF(REFIID iid) = 0;
};

// ----------------------------------------------------------------------------
//  エミュレータ上のシステムをいじる時に使うロック
//  IMemoryManager / IIOBus / IScheduler 等に対する操作を
//  初期化，終了時，またはメモリ, IO, タイマーコールバック以外から
//  行う場合，ロックをかける必要がある
//
struct ILockCore {
  virtual void IFCALL Lock() = 0;
  virtual void IFCALL Unlock() = 0;
};

// ----------------------------------------------------------------------------
//  現在アクティブになっているメモリの種類を取得
//
struct IGetMemoryBank {
  virtual uint32_t IFCALL GetRdBank(uint32_t) = 0;
  virtual uint32_t IFCALL GetWrBank(uint32_t) = 0;
};
