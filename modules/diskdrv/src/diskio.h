// ---------------------------------------------------------------------------
//  PC-8801 emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  $Id: diskio.h,v 1.3 1999/10/10 01:38:05 cisc Exp $

#ifndef pc88_diskio_h
#define pc88_diskio_h

#include "common/device.h"
#include "win32/file.h"

namespace PC8801 {

// ---------------------------------------------------------------------------

class DiskIO : public Device {
 public:
  enum { reset = 0, setcommand, setdata, getstatus = 0, getdata };

 public:
  DiskIO(const ID& id);
  ~DiskIO();
  bool Init();

  void IOCALL Reset(uint = 0, uint = 0);
  void IOCALL SetCommand(uint, uint);
  void IOCALL SetData(uint, uint);
  uint IOCALL GetStatus(uint = 0);
  uint IOCALL GetData(uint = 0);

  const Descriptor* IFCALL GetDesc() const { return &descriptor; }

 private:
  enum Phase {
    idlephase,
    argphase,
    recvphase,
    sendphase,
  };

  void ProcCommand();
  void ArgPhase(int l);
  void SendPhase(uint8_t* p, int l);
  void RecvPhase(uint8_t* p, int l);
  void IdlePhase();

  void CmdSetFileName();
  void CmdWriteFile();
  void CmdReadFile();
  void CmdGetError();
  void CmdWriteFlush();

  uint8_t* ptr;
  int len;

  FileIO file;
  int size;
  int length;

  Phase phase;
  bool writebuffer;
  uint8_t status;
  uint8_t cmd;
  uint8_t err;
  uint8_t arg[5];
  uint8_t filename[MAX_PATH];
  uint8_t buf[1024];

  static const Descriptor descriptor;
  static const InFuncPtr indef[];
  static const OutFuncPtr outdef[];
};
}

#endif  // pc88_diskio_h
