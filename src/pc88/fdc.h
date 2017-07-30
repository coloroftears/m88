// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  uPD765A
// ---------------------------------------------------------------------------
//  $Id: fdc.h,v 1.12 2001/02/21 11:57:57 cisc Exp $

#pragma once

#include <memory>

#include "common/device.h"
#include "pc88/fdu.h"
#include "pc88/floppy.h"

class Scheduler;
class SchedulerEvent;

namespace pc88core {

class Config;
class DiskManager;

// ---------------------------------------------------------------------------
//  FDC (765)
//
class FDC final : public Device {
 public:
  enum {
    num_drives = 2,
  };
  enum ConnID {
    kReset,
    kSetData,
    kDriveControl,
    kMotorControl,
    kGetStatus = 0,
    kGetData,
    kTCIn
  };

  enum Result {
    ST0_NR = 0x000008,
    ST0_EC = 0x000010,
    ST0_SE = 0x000020,
    ST0_AT = 0x000040,
    ST0_IC = 0x000080,
    ST0_AI = 0x0000c0,

    ST1_MA = 0x000100,
    ST1_NW = 0x000200,
    ST1_ND = 0x000400,
    ST1_OR = 0x001000,
    ST1_DE = 0x002000,
    ST1_EN = 0x008000,

    ST2_MD = 0x010000,
    ST2_BC = 0x020000,
    ST2_SN = 0x040000,
    ST2_SH = 0x080000,
    ST2_NC = 0x100000,
    ST2_DD = 0x200000,
    ST2_CM = 0x400000,

    ST3_HD = 0x04,
    ST3_TS = 0x08,
    ST3_T0 = 0x10,
    ST3_RY = 0x20,
    ST3_WP = 0x40,
    ST3_FT = 0x80,
  };
  using IDR = FloppyDisk::IDR;
  using WIDDESC = FDU::WIDDESC;

 protected:
  enum Stat {
    S_D0B = 0x01,
    S_D1B = 0x02,
    S_D2B = 0x04,
    S_D3B = 0x08,
    S_CB = 0x10,
    S_NDM = 0x20,
    S_DIO = 0x40,
    S_RQM = 0x80,
  };

 public:
  explicit FDC(const ID& id);
  ~FDC();

  bool Init(DiskManager* dm,
            Scheduler* scheduler,
            IOBus* bus,
            int intport,
            int statport = -1);
  void ApplyConfig(const Config* cfg);

  bool IsBusy() { return phase_ != kIdlePhase; }

  void IOCALL Reset(uint32_t = 0, uint32_t = 0);
  void IOCALL DriveControl(uint32_t, uint32_t data);  // 2HD/2DD 切り替えとか
  void IOCALL MotorControl(uint32_t, uint32_t data) {}  // モーター制御
  void IOCALL SetData(uint32_t, uint32_t data);         // データセット
  uint32_t IOCALL TC(uint32_t);                         // TC
  uint32_t IOCALL Status(uint32_t);                     // ステータス入力
  uint32_t IOCALL GetData(uint32_t);                    // データ取得

  // Overrides Device.
  const Descriptor* IFCALL GetDesc() const final { return &descriptor; }
  uint32_t IFCALL GetStatusSize() final;
  bool IFCALL SaveStatus(uint8_t* status) final;
  bool IFCALL LoadStatus(const uint8_t* status) final;

 private:
  enum Phase {
    kIdlePhase,
    kCommandPhase,
    kExecutePhase,
    kExecReadPhase,
    kExecWritePhase,
    kResultPhase,
    kTCPhase,
    kTimerPhase,
    kExecScanPhase,
  };

  struct Drive {
    uint32_t cylinder;
    uint32_t result;
    uint8_t hd;
    uint8_t dd;
  };

  enum {
    ssrev = 1,
  };
  struct Snapshot {
    uint8_t rev;
    uint8_t hdu;  // HD US1 US0
    uint8_t hdue;
    uint8_t dtl;
    uint8_t eot;
    uint8_t seekstate;
    uint8_t result;
    uint8_t status;      // ステータスレジスタ
    uint8_t command;     // 現在処理中のコマンド
    uint8_t data;        // データレジスタ
    bool int_requested;  // SENCEINTSTATUS の呼び出しを要求した
    bool accepttc;

    uint32_t bufptr;
    uint32_t count;  // Exec*Phase での転送残りバイト
    Phase phase, prevphase;
    Phase t_phase;

    IDR idr;

    uint32_t readdiagptr;
    uint32_t readdiaglim;
    uint32_t xbyte;
    uint32_t readdiagcount;

    WIDDESC wid;
    Drive dr[num_drives];
    uint8_t buf[0x4000];
  };

  using CommandFunc = void (FDC::*)();

  void Seek(uint32_t dr, uint32_t cy);
  void IOCALL SeekEvent(uint32_t dr);
  void ReadID();
  void ReadData(bool deleted, bool scan);
  void ReadDiagnostic();
  void WriteData(bool deleted);
  void WriteID();

  void SetTimer(Phase phase, SchedTimeDelta ticks);
  void DelTimer();
  void IOCALL PhaseTimer(uint32_t p);
  void Intr(bool i);

  bool IDIncrement();
  void GetSectorParameters();
  uint32_t CheckCondition(bool write);

  void ShiftToIdlePhase();
  void ShiftToCommandPhase(int);
  void ShiftToExecutePhase();
  void ShiftToExecReadPhase(int, uint8_t* data = 0);
  void ShiftToExecWritePhase(int);
  void ShiftToExecScanPhase(int);
  void ShiftToResultPhase(int);
  void ShiftToResultPhase7();

  uint32_t GetDeviceStatus(uint32_t dr);

  DiskManager* diskmgr_;
  IOBus* bus_;
  int pintr_;
  Scheduler* scheduler_ = nullptr;

  SchedulerEvent* timer_handle_ = nullptr;
  SchedTimeDelta seek_time_ = 0;

  // ステータスレジスタ
  uint32_t status_;
  std::unique_ptr<uint8_t[]> buffer_;
  uint8_t* bufptr_;
  // Exec*Phase での転送残りバイト
  int count_;
  // 現在処理中のコマンド
  uint32_t command_;
  // データレジスタ
  uint32_t data_;
  Phase phase_;
  Phase prev_phase_;
  Phase t_phase_;
  // SENCEINTSTATUS の呼び出しを要求した
  bool int_requested_;
  bool accept_tc_;
  bool show_status_;

  bool disk_wait_;

  IDR idr_;
  uint32_t hdu_;  // HD US1 US0
  uint32_t hdue_;
  uint32_t dtl_;
  uint32_t eot_;
  uint32_t seek_state_;
  uint32_t result_;

  uint8_t* readdiag_ptr_ = nullptr;
  uint8_t* readdiag_lim_ = nullptr;
  uint32_t xbyte_;
  uint32_t readdiag_count_;

  uint32_t litdrive_;
  uint32_t fdstat_;
  uint32_t pfdstat_;

  WIDDESC wid_;
  Drive drive_[4];

  static const CommandFunc CommandTable[32];

  void CmdInvalid();
  void CmdSpecify();
  void CmdRecalibrate();
  void CmdSenseIntStatus();
  void CmdSenseDeviceStatus();
  void CmdSeek();
  void CmdReadData();
  void CmdReadDiagnostic();
  void CmdWriteData();
  void CmdReadID();
  void CmdWriteID();
  void CmdScanEqual();

 private:
  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};
}  // namespace pc88core
