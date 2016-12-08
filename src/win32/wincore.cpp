// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 2001.
// ---------------------------------------------------------------------------
//  $Id: wincore.cpp,v 1.42 2003/05/15 13:15:35 cisc Exp $

#include "win32/wincore.h"

#include "common/device.h"
#include "common/file.h"
#include "common/toast.h"
#include "interface/ifguid.h"
#include "interface/ifpc88.h"
#include "pc88/beep.h"
#include "pc88/config.h"
#include "pc88/disk_manager.h"
#include "pc88/joypad.h"
#include "pc88/opn_interface.h"
#include "pc88/pd8257.h"
#include "win32/module.h"
#include "win32/ui.h"
#include "win32/winkeyif.h"
#include "zlib/zlib.h"

#define LOGNAME "wincore"
#include "common/diag.h"

using namespace PC8801;

//                   0123456789abcdef
#define SNAPSHOT_ID "M88 SnapshotData"

// ---------------------------------------------------------------------------
//  構築/消滅
//
WinCore::WinCore() {}

WinCore::~WinCore() {
  Cleanup();
}

// ---------------------------------------------------------------------------
//  初期化
//
bool WinCore::Init(WinUI* _ui,
                   HWND hwnd,
                   Draw* draw,
                   DiskManager* disk,
                   WinKeyIF* keyb,
                   IConfigPropBase* cp,
                   TapeManager* tape) {
  ui = _ui;
  cfgprop = cp;

  if (!PC88::Init(draw, disk, tape))
    return false;

  if (!sound.Init(this, hwnd, 0, 0))
    return false;

  padif.Init();

  if (!ConnectDevices(keyb))
    return false;

  if (!ConnectExternalDevices())
    return false;

  seq.SetClock(40);
  seq.SetSpeed(100);

  if (!seq.Init(this))
    return false;

  return true;
}

// ---------------------------------------------------------------------------
//  後始末
//
bool WinCore::Cleanup() {
  seq.Cleanup();

  for (ExtendModules::iterator i = extmodules.begin(); i != extmodules.end();
       ++i)
    delete *i;
  extmodules.clear();

  return true;
}

// ---------------------------------------------------------------------------
//  リセット
//
void WinCore::Reset() {
  LockObj lock(this);
  PC88::Reset();
}

// ---------------------------------------------------------------------------
//  設定を反映する
//
void WinCore::ApplyConfig(PC8801::Config* cfg) {
  config = *cfg;

  int c = cfg->clock;
  if (cfg->flags & PC8801::Config::fullspeed)
    c = 0;
  if (cfg->flags & PC8801::Config::cpuburst)
    c = -c;
  seq.SetClock(c);
  seq.SetSpeed(cfg->speed / 10);
  seq.SetRefreshTiming(cfg->refreshtiming);

  if (joypad)
    joypad->Connect(&padif);

  PC88::ApplyConfig(cfg);
  sound.ApplyConfig(cfg);
  draw->SetFlipMode(false);
}

// ---------------------------------------------------------------------------
//  Windows 用のデバイスを接続
//
bool WinCore::ConnectDevices(WinKeyIF* keyb) {
  static const IOBus::Connector c_keyb[] = {
      {PC88::pres, IOBus::portout, WinKeyIF::reset},
      {PC88::vrtc, IOBus::portout, WinKeyIF::vsync},
      {0x00, IOBus::portin, WinKeyIF::in},
      {0x01, IOBus::portin, WinKeyIF::in},
      {0x02, IOBus::portin, WinKeyIF::in},
      {0x03, IOBus::portin, WinKeyIF::in},
      {0x04, IOBus::portin, WinKeyIF::in},
      {0x05, IOBus::portin, WinKeyIF::in},
      {0x06, IOBus::portin, WinKeyIF::in},
      {0x07, IOBus::portin, WinKeyIF::in},
      {0x08, IOBus::portin, WinKeyIF::in},
      {0x09, IOBus::portin, WinKeyIF::in},
      {0x0a, IOBus::portin, WinKeyIF::in},
      {0x0b, IOBus::portin, WinKeyIF::in},
      {0x0c, IOBus::portin, WinKeyIF::in},
      {0x0d, IOBus::portin, WinKeyIF::in},
      {0x0e, IOBus::portin, WinKeyIF::in},
      {0x0f, IOBus::portin, WinKeyIF::in},
      {0, 0, 0}};
  if (!bus1.Connect(keyb, c_keyb))
    return false;

  if (FAILED(GetOPN1()->Connect(&sound)))
    return false;
  if (FAILED(GetOPN2()->Connect(&sound)))
    return false;
  if (FAILED(GetBEEP()->Connect(&sound)))
    return false;

  return true;
}

// ---------------------------------------------------------------------------
//  スナップショット保存
//
bool WinCore::SaveSnapshot(const char* filename) {
  LockObj lock(this);

  bool docomp = !!(config.flag2 & Config::compresssnapshot);

  uint32_t size = devlist.GetStatusSize();
  uint8_t* buf = new uint8_t[docomp ? size * 129 / 64 + 20 : size];
  if (!buf)
    return false;
  memset(buf, 0, size);

  if (devlist.SaveStatus(buf)) {
    uLongf esize = size * 129 / 64 + 20 - 4;
    if (docomp) {
      if (Z_OK != compress(buf + size + 4, &esize, buf, size)) {
        delete[] buf;
        return false;
      }
      *(int32_t*)(buf + size) = -(int32_t)esize;
      esize += 4;
    }

    SnapshotHeader ssh;
    memcpy(ssh.id, SNAPSHOT_ID, 16);

    ssh.major = ssmajor;
    ssh.minor = ssminor;
    ssh.datasize = size;
    ssh.basicmode = config.basicmode;
    ssh.clock = int16_t(config.clock);
    ssh.erambanks = uint16_t(config.erambanks);
    ssh.cpumode = int16_t(config.cpumode);
    ssh.mainsubratio = int16_t(config.mainsubratio);
    ssh.flags = config.flags | (esize < size ? 0x80000000 : 0);
    ssh.flag2 = config.flag2;
    for (uint32_t i = 0; i < 2; i++)
      ssh.disk[i] = (int8_t)diskmgr->GetCurrentDisk(i);

    FileIO file;
    if (file.Open(filename, FileIO::create)) {
      file.Write(&ssh, sizeof(ssh));
      if (esize < size)
        file.Write(buf + size, esize);
      else
        file.Write(buf, size);
    }
  }
  delete[] buf;
  return true;
}

// ---------------------------------------------------------------------------
//  スナップショット復元
//
bool WinCore::LoadSnapshot(const char* filename, const char* diskname) {
  LockObj lock(this);

  FileIO file;
  if (!file.Open(filename, FileIO::readonly))
    return false;

  SnapshotHeader ssh;
  if (file.Read(&ssh, sizeof(ssh)) != sizeof(ssh))
    return false;
  if (memcmp(ssh.id, SNAPSHOT_ID, 16))
    return false;
  if (ssh.major != ssmajor || ssh.minor > ssminor)
    return false;

  // applyconfig
  const uint32_t fl1a = Config::subcpucontrol | Config::fullspeed |
                        Config::enableopna | Config::enablepcg | Config::fv15k |
                        Config::cpuburst | Config::cpuclockmode |
                        Config::digitalpalette | Config::opnona8 |
                        Config::opnaona8 | Config::enablewait;
  const uint32_t fl2a = Config::disableopn44;

  config.flags = (config.flags & ~fl1a) | (ssh.flags & fl1a);
  config.flag2 = (config.flag2 & ~fl2a) | (ssh.flag2 & fl2a);
  config.basicmode = ssh.basicmode;
  config.clock = ssh.clock;
  config.erambanks = ssh.erambanks;
  config.cpumode = ssh.cpumode;
  config.mainsubratio = ssh.mainsubratio;
  ApplyConfig(&config);

  // Reset
  PC88::Reset();

  // 読み込み

  uint8_t* buf = new uint8_t[ssh.datasize];
  bool r = false;

  if (buf) {
    bool read = false;
    if (ssh.flags & 0x80000000) {
      int32_t csize;

      file.Read(&csize, 4);
      if (csize < 0) {
        csize = -csize;
        uint8_t* cbuf = new uint8_t[csize];

        if (cbuf) {
          uLongf bufsize = ssh.datasize;
          file.Read(cbuf, csize);
          read = uncompress(buf, &bufsize, cbuf, csize) == Z_OK;

          delete[] cbuf;
        }
      }
    } else
      read = file.Read(buf, ssh.datasize) == ssh.datasize;

    if (read) {
      r = devlist.LoadStatus(buf);
      if (r && diskname) {
        for (uint32_t i = 0; i < 2; i++) {
          diskmgr->Unmount(i);
          diskmgr->Mount(i, diskname, false, ssh.disk[i], false);
        }
      }
      if (!r) {
        Toast::Show(70, 3000, "バージョンが異なります");
        PC88::Reset();
      }
    }
    delete[] buf;
  }
  return r;
}

// ---------------------------------------------------------------------------
//  外部もじゅーるのためにインターフェースを提供する
//
void* WinCore::QueryIF(REFIID id) {
  if (id == M88IID_IOBus1)
    return static_cast<IIOBus*>(&bus1);
  if (id == M88IID_IOBus2)
    return static_cast<IIOBus*>(&bus2);
  if (id == M88IID_IOAccess1)
    return static_cast<IIOAccess*>(&bus1);
  if (id == M88IID_IOAccess2)
    return static_cast<IIOAccess*>(&bus2);
  if (id == M88IID_MemoryManager1)
    return static_cast<IMemoryManager*>(&mm1);
  if (id == M88IID_MemoryManager2)
    return static_cast<IMemoryManager*>(&mm2);
  if (id == M88IID_MemoryAccess1)
    return static_cast<IMemoryAccess*>(&mm1);
  if (id == M88IID_MemoryAccess2)
    return static_cast<IMemoryAccess*>(&mm2);
  if (id == M88IID_SoundControl)
    return static_cast<ISoundControl*>(&sound);
  if (id == M88IID_Scheduler)
    return static_cast<IScheduler*>(GetScheduler());
  if (id == M88IID_Time)
    return static_cast<ITime*>(GetScheduler());
  if (id == M88IID_CPUTime)
    return static_cast<ICPUTime*>(this);
  if (id == M88IID_DMA)
    return static_cast<IDMAAccess*>(GetDMAC());
  if (id == M88IID_ConfigPropBase)
    return cfgprop;
  if (id == M88IID_LockCore)
    return static_cast<ILockCore*>(this);
  if (id == M88IID_GetMemoryBank)
    return static_cast<IGetMemoryBank*>(GetMem1());

  return 0;
}

bool WinCore::ConnectExternalDevices() {
  FileFinder ff;
  extern char m88dir[MAX_PATH];
  char buf[MAX_PATH];
  strncpy(buf, m88dir, MAX_PATH);
  strncat(buf, "*.m88", MAX_PATH);

  if (ff.FindFile(buf)) {
    while (ff.FindNext()) {
      const char* modname = ff.GetFileName();
      ExtendModule* em = ExtendModule::Create(modname, this);
      if (em)
        extmodules.push_back(em);
    }
  }
  return true;
}

// ---------------------------------------------------------------------------
//  PAD を接続
//
/*
bool WinCore::EnablePad(bool enable)
{
    if (padenable == enable)
        return true;
    padenable = enable;

    LockObj lock(this);
    if (enable)
    {
        static const IOBus::Connector c_pad[] =
        {
            { PC88::popnio  , IOBus::portin, WinJoyPad::getdir },
            { PC88::popnio+1, IOBus::portin, WinJoyPad::getbutton },
            { PC88::vrtc,     IOBus::portout, WinJoyPad::vsync },
            { 0, 0, 0 }
        };
        if (!bus1.Connect(&pad, c_pad)) return false;
        pad.Init();
    }
    else
    {
        bus1.Disconnect(&pad);
    }
    return true;
}
*/
// ---------------------------------------------------------------------------
//  マウスを接続
//
/*
bool WinCore::EnableMouse(bool enable)
{
    if (mouseenable == enable)
        return true;
    mouseenable = enable;

    LockObj lock(this);
    if (enable)
    {
        static const IOBus::Connector c_mouse[] =
        {
            { PC88::popnio  , IOBus::portin, WinMouse::getmove },
            { PC88::popnio+1, IOBus::portin, WinMouse::getbutton },
            { 0x40,           IOBus::portout, WinMouse::strobe },
            { PC88::vrtc,     IOBus::portout, WinMouse::vsync },
            { 0, 0, 0 }
        };
        if (!bus1.Connect(&mouse, c_mouse)) return false;
        ActivateMouse(true);
    }
    else
    {
        bus1.Disconnect(&mouse);
        ActivateMouse(false);
    }
    return true;
}
*/
