// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  uPD765A
// ---------------------------------------------------------------------------
//  $Id: fdc.h,v 1.12 2001/02/21 11:57:57 cisc Exp $

#if !defined(pc88_fdc_h)
#define pc88_fdc_h

#include "common/device.h"
#include "common/schedule.h"
#include "pc88/floppy.h"
#include "pc88/fdu.h"

class DiskManager;

namespace PC8801 {
class Config;

// ---------------------------------------------------------------------------
//  FDC (765)
//
class FDC : public Device {
 public:
  enum {
    num_drives = 2,
  };
  enum ConnID {
    reset = 0,
    setdata,
    drivecontrol,
    motorcontrol,
    getstatus = 0,
    getdata,
    tcin
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
  typedef FloppyDisk::IDR IDR;
  typedef FDU::WIDDESC WIDDESC;

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
  FDC(const ID& id);
  ~FDC();

  bool Init(DiskManager* dm,
            Scheduler* scheduler,
            IOBus* bus,
            int intport,
            int statport = -1);
  void ApplyConfig(const Config* cfg);

  bool IsBusy() { return phase != idlephase; }

  void IOCALL Reset(uint = 0, uint = 0);
  void IOCALL DriveControl(uint, uint data);    // 2HD/2DD �؂�ւ��Ƃ�
  void IOCALL MotorControl(uint, uint data) {}  // ���[�^�[����
  void IOCALL SetData(uint, uint data);         // �f�[�^�Z�b�g
  uint IOCALL TC(uint);                         // TC
  uint IOCALL Status(uint);                     // �X�e�[�^�X����
  uint IOCALL GetData(uint);                    // �f�[�^�擾

  uint IFCALL GetStatusSize();
  bool IFCALL SaveStatus(uint8* status);
  bool IFCALL LoadStatus(const uint8* status);

  const Descriptor* IFCALL GetDesc() const { return &descriptor; }

 private:
  enum Phase {
    idlephase,
    commandphase,
    executephase,
    execreadphase,
    execwritephase,
    resultphase,
    tcphase,
    timerphase,
    execscanphase,
  };

  struct Drive {
    uint cyrinder;
    uint result;
    uint8 hd;
    uint8 dd;
  };

  enum {
    ssrev = 1,
  };
  struct Snapshot {
    uint8 rev;
    uint8 hdu;  // HD US1 US0
    uint8 hdue;
    uint8 dtl;
    uint8 eot;
    uint8 seekstate;
    uint8 result;
    uint8 status;        // �X�e�[�^�X���W�X�^
    uint8 command;       // ���ݏ������̃R�}���h
    uint8 data;          // �f�[�^���W�X�^
    bool int_requested;  // SENCEINTSTATUS �̌Ăяo����v������
    bool accepttc;

    uint bufptr;
    uint count;  // Exec*Phase �ł̓]���c��o�C�g
    Phase phase, prevphase;
    Phase t_phase;

    IDR idr;

    uint readdiagptr;
    uint readdiaglim;
    uint xbyte;
    uint readdiagcount;

    WIDDESC wid;
    Drive dr[num_drives];
    uint8 buf[0x4000];
  };

  typedef void (FDC::*CommandFunc)();

  void Seek(uint dr, uint cy);
  void IOCALL SeekEvent(uint dr);
  void ReadID();
  void ReadData(bool deleted, bool scan);
  void ReadDiagnostic();
  void WriteData(bool deleted);
  void WriteID();

  void SetTimer(Phase phase, int ticks);
  void DelTimer();
  void IOCALL PhaseTimer(uint p);
  void Intr(bool i);

  bool IDIncrement();
  void GetSectorParameters();
  uint CheckCondition(bool write);

  void ShiftToIdlePhase();
  void ShiftToCommandPhase(int);
  void ShiftToExecutePhase();
  void ShiftToExecReadPhase(int, uint8* data = 0);
  void ShiftToExecWritePhase(int);
  void ShiftToExecScanPhase(int);
  void ShiftToResultPhase(int);
  void ShiftToResultPhase7();

  uint GetDeviceStatus(uint dr);

  DiskManager* diskmgr;
  IOBus* bus;
  int pintr;
  Scheduler* scheduler;

  Scheduler::Event* timerhandle;
  uint seektime;

  uint status;  // �X�e�[�^�X���W�X�^
  uint8* buffer;
  uint8* bufptr;
  int count;     // Exec*Phase �ł̓]���c��o�C�g
  uint command;  // ���ݏ������̃R�}���h
  uint data;     // �f�[�^���W�X�^
  Phase phase, prevphase;
  Phase t_phase;
  bool int_requested;  // SENCEINTSTATUS �̌Ăяo����v������
  bool accepttc;
  bool showstatus;

  bool diskwait;

  IDR idr;
  uint hdu;  // HD US1 US0
  uint hdue;
  uint dtl;
  uint eot;
  uint seekstate;
  uint result;

  uint8* readdiagptr;
  uint8* readdiaglim;
  uint xbyte;
  uint readdiagcount;

  uint litdrive;
  uint fdstat;
  uint pfdstat;

  WIDDESC wid;
  Drive drive[4];

  static const CommandFunc CommandTable[32];

  void CmdInvalid();
  void CmdSpecify();
  void CmdRecalibrate();
  void CmdSenceIntStatus();
  void CmdSenceDeviceStatus();
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
}

#endif  // !defined(pc88_fdc_h)
