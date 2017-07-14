// ---------------------------------------------------------------------------
//  PC-8801 emulator
//  Copyright (C) cisc 1999.
// ---------------------------------------------------------------------------
//  デバイスと進行管理
// ---------------------------------------------------------------------------
//  $Id: pc88.cpp,v 1.53 2003/09/28 14:35:35 cisc Exp $

//  Memory Bus Banksize <= 0x400

#include "pc88/pc88.h"

#include <algorithm>

#include "common/status.h"
#include "common/time_keeper.h"
#include "common/toast.h"
#include "pc88/base.h"
#include "pc88/beep.h"
#include "pc88/calendar.h"
#include "pc88/config.h"
#include "pc88/crtc.h"
#include "pc88/disk_manager.h"
#include "pc88/fdc.h"
#include "pc88/interrupt_controller.h"
#include "pc88/joypad.h"
#include "pc88/kanjirom.h"
#include "pc88/memory.h"
#include "pc88/opn_interface.h"
#include "pc88/pd8257.h"
#include "pc88/screen.h"
#include "pc88/sio.h"
#include "pc88/subsys.h"
#include "pc88/tape_manager.h"
#include "win32/monitors/loadmon.h"

//#define LOGNAME "pc88"
#include "common/diag.h"

namespace pc88core {

// ---------------------------------------------------------------------------
//  構築・破棄
//
PC88::PC88()
    : main_cpu_(DEV_ID('C', 'P', 'U', '1')),
      sub_cpu_(DEV_ID('C', 'P', 'U', '2')),
      base(0),
      mem1(0),
      dmac(0),
      knj1(0),
      knj2(0),
      scrn(0),
      intc(0),
      crtc(0),
      fdc(0),
      subsys(0),
      siotape(0),
      opn1(0),
      opn2(0),
      caln(0),
      diskmgr_(0),
      beep(0),
      siomidi(0),
      joypad(0) {
  assert((1 << MemoryManager::pagebits) <= 0x400);
  sched_.reset(new Scheduler(this));
  DIAGINIT(&main_cpu_);
  dexc = 0;
}

PC88::~PC88() {
  //  devlist_.Cleanup();

  delete base;
  delete mem1;
  delete dmac;
  delete knj1;
  delete knj2;
  delete scrn;
  delete intc;
  delete crtc;
  delete fdc;
  delete subsys;
  delete siotape;
  delete siomidi;
  delete caln;
  delete joypad;
}

// ---------------------------------------------------------------------------
//  初期化
//
bool PC88::Init(Draw* _draw, DiskManager* disk, TapeManager* tape) {
  draw = _draw;
  diskmgr_ = disk;
  tapemgr_ = tape;

  if (!sched_->Init())
    return false;

  if (!draw->Init(640, 400, 8))
    return false;

  if (!tapemgr_->Init(sched_.get(), 0, 0))
    return false;

  MemoryPage *read, *write;

  main_cpu_.GetPages(&read, &write);
  if (!main_mm_.Init(0x10000, read, write))
    return false;

  sub_cpu_.GetPages(&read, &write);
  if (!sub_mm_.Init(0x10000, read, write))
    return false;

  if (!main_bus_.Init(kPortEnd, &devlist_) || !sub_bus_.Init(kPortEnd2, &devlist_))
    return false;

  if (!ConnectDevices() || !ConnectDevices2())
    return false;

  Reset();
  region.Reset();
  clock_ = 1;
  return true;
}

// ---------------------------------------------------------------------------
//  執行
//  1 tick = 10μs
//
SchedTimeDelta PC88::Proceed(SchedTimeDelta ticks,
                             SchedClock clock,
                             SchedClock effective_clock) {
  clock_ = std::max(1, clock);
  eclock_ = std::max(1, effective_clock);
  return sched_->Proceed(ticks);
}

// ---------------------------------------------------------------------------
//  実行
//
SchedTimeDelta PC88::Execute(SchedTimeDelta ticks) {
  LOADBEGIN("Core.CPU");
  int exc = ticks * clock_;
  if (!(cpumode & stopwhenidle) || subsys->IsBusy() || fdc->IsBusy()) {
    if ((cpumode & 1) == ms11)
      exc = Z80Util::ExecDual(&main_cpu_, &sub_cpu_, exc);
    else
      exc = Z80Util::ExecDual2(&main_cpu_, &sub_cpu_, exc);
  } else {
    exc = Z80Util::ExecSingle(&main_cpu_, &sub_cpu_, exc);
  }
  exc += dexc;
  dexc = exc % clock_;
  LOADEND("Core.CPU");
  return exc / clock_;
}

// ---------------------------------------------------------------------------
//  実行クロック数変更
//
void PC88::Shorten(SchedTimeDelta ticks) {
  Z80Util::StopDual(ticks * clock_);
}

SchedTimeDelta PC88::GetTicks() {
  return (Z80Util::GetCCount() + dexc) / clock_;
}

// ---------------------------------------------------------------------------
//  VSync
//
void PC88::VSync() {
  statusdisplay.UpdateDisplay();
  if (cfgflags & Config::kWatchRegister)
    Toast::Show(10, 0, "%.4X(%.2X)/%.4X", main_cpu_.GetPC(), main_cpu_.GetReg().ireg,
                sub_cpu_.GetPC());
}

// ---------------------------------------------------------------------------
//  画面更新
//
void PC88::UpdateScreen(bool refresh) {
  int dstat = draw->GetStatus();
  if (dstat & Draw::shouldrefresh)
    refresh = true;

  LOADBEGIN("Screen");

  if (!updated || refresh) {
    if (!(cfgflags & Config::kDrawPriorityLow) ||
        (dstat & (Draw::readytodraw | Draw::shouldrefresh)))
    //      if (dstat & (Draw::readytodraw | Draw::shouldrefresh))
    {
      int bpl;
      uint8_t* image;

      //          crtc->SetSize();
      if (draw->Lock(&image, &bpl)) {
        Log("(%d -> %d) ", region.top, region.bottom);
        crtc->UpdateScreen(image, bpl, region, refresh);
        Log("(%d -> %d) ", region.top, region.bottom);
        scrn->UpdateScreen(image, bpl, region, refresh);
        Log("(%d -> %d)\n", region.top, region.bottom);

        bool palchanged = scrn->UpdatePalette(draw);
        draw->Unlock();
        updated = palchanged || region.Valid();
      }
    }
  }
  LOADEND("Screen");
  if (draw->GetStatus() & Draw::readytodraw) {
    if (updated) {
      updated = false;
      draw->DrawScreen(region);
      region.Reset();
    } else {
      Draw::Region r;
      r.Reset();
      draw->DrawScreen(r);
    }
  }
}

// ---------------------------------------------------------------------------
//  リセット
//
void PC88::Reset() {
  bool cd = false;
  if (IsCDSupported())
    cd = (base->GetBasicMode() & 0x40) != 0;

  base->SetFDBoot(cd || diskmgr_->GetCurrentDisk(0) >= 0);
  base->Reset();  // Switch 関係の更新

  bool isv2 = (main_bus_.In(0x31) & 0x40) != 0;
  bool isn80v2 = (base->GetBasicMode() == Config::N80V2);

  if (isv2)
    dmac->ConnectRd(mem1->GetTVRAM(), 0xf000, 0x1000);
  else
    dmac->ConnectRd(mem1->GetRAM(), 0, 0x10000);
  dmac->ConnectWr(mem1->GetRAM(), 0, 0x10000);

  opn1->SetOPNMode((cfgflags & Config::kEnableOPNA) != 0);
  opn1->Enable(isv2 || !(cfgflag2 & Config::kDisableOPN44));
  opn2->SetOPNMode((cfgflags & Config::kOPNAOnA8) != 0);
  opn2->Enable((cfgflags & (Config::kOPNAOnA8 | Config::kOPNOnA8)) != 0);

  if (!isn80v2)
    opn1->SetIMask(0x32, 0x80);
  else
    opn1->SetIMask(0x33, 0x02);

  main_bus_.Out(kPortReset, base->GetBasicMode());
  main_bus_.Out(0x30, 1);
  main_bus_.Out(0x30, 0);
  main_bus_.Out(0x31, 0);
  main_bus_.Out(0x32, 0x80);
  main_bus_.Out(0x33, isn80v2 ? 0x82 : 0x02);
  main_bus_.Out(0x34, 0);
  main_bus_.Out(0x35, 0);
  main_bus_.Out(0x40, 0);
  main_bus_.Out(0x53, 0);
  main_bus_.Out(0x5f, 0);
  main_bus_.Out(0x70, 0);
  main_bus_.Out(0x99, cd ? 0x10 : 0x00);
  main_bus_.Out(0xe2, 0);
  main_bus_.Out(0xe3, 0);
  main_bus_.Out(0xe6, 0);
  main_bus_.Out(0xf1, 1);
  sub_bus_.Out(kPortReset2, 0);

  // Toast::Show(10, 1000, "CPUMode = %d", cpumode);
}

// ---------------------------------------------------------------------------
//  デバイス接続
//
bool PC88::ConnectDevices() {
  static const IOBus::Connector c_cpu1[] = {
      {kPortReset, IOBus::portout, static_cast<uint8_t>(Z80Int::kReset)},
      {kPortIRQ, IOBus::portout, static_cast<uint8_t>(Z80Int::kIRQ)},
      {0, 0, 0}};
  if (!main_bus_.Connect(&main_cpu_, c_cpu1))
    return false;
  if (!main_cpu_.Init(&main_mm_, &main_bus_, kPortIntAck))
    return false;

  static const IOBus::Connector c_base[] = {
      {kPortReset, IOBus::portout, Base::kReset},
      {kVRTC, IOBus::portout, Base::kVRTC},
      {0x30, IOBus::portin, Base::kIn30},
      {0x31, IOBus::portin, Base::kIn31},
      {0x40, IOBus::portin, Base::kIn40},
      {0x6e, IOBus::portin, Base::kIn6e},
      {0, 0, 0}};
  base = new Base(DEV_ID('B', 'A', 'S', 'E'));
  if (!base || !main_bus_.Connect(base, c_base))
    return false;
  if (!base->Init(this))
    return false;
  devlist_.Add(tapemgr_);

  static const IOBus::Connector c_dmac[] = {
      {kPortReset, IOBus::portout, PD8257::kReset},
      {0x60, IOBus::portout, PD8257::kSetAddr},
      {0x61, IOBus::portout, PD8257::kSetCount},
      {0x62, IOBus::portout, PD8257::kSetAddr},
      {0x63, IOBus::portout, PD8257::kSetCount},
      {0x64, IOBus::portout, PD8257::kSetAddr},
      {0x65, IOBus::portout, PD8257::kSetCount},
      {0x66, IOBus::portout, PD8257::kSetAddr},
      {0x67, IOBus::portout, PD8257::kSetCount},
      {0x68, IOBus::portout, PD8257::kSetMode},
      {0x60, IOBus::portin, PD8257::kGetAddr},
      {0x61, IOBus::portin, PD8257::kGetCount},
      {0x62, IOBus::portin, PD8257::kGetAddr},
      {0x63, IOBus::portin, PD8257::kGetCount},
      {0x64, IOBus::portin, PD8257::kGetAddr},
      {0x65, IOBus::portin, PD8257::kGetCount},
      {0x66, IOBus::portin, PD8257::kGetAddr},
      {0x67, IOBus::portin, PD8257::kGetCount},
      {0x68, IOBus::portin, PD8257::kGetStat},
      {0, 0, 0}};
  dmac = new PD8257(DEV_ID('D', 'M', 'A', 'C'));
  if (!main_bus_.Connect(dmac, c_dmac))
    return false;

  static const IOBus::Connector c_crtc[] = {
      {kPortReset, IOBus::portout, CRTC::kReset},
      {0x50, IOBus::portout, CRTC::kOut},
      {0x51, IOBus::portout, CRTC::kOut},
      {0x50, IOBus::portin, CRTC::kGetStatus},
      {0x51, IOBus::portin, CRTC::kIn},
      {0x00, IOBus::portout, CRTC::kPCGOut},
      {0x01, IOBus::portout, CRTC::kPCGOut},
      {0x02, IOBus::portout, CRTC::kPCGOut},
      {0x33, IOBus::portout, CRTC::kSetKanaMode},
      {0, 0, 0}};
  crtc = new CRTC(DEV_ID('C', 'R', 'T', 'C'));
  if (!crtc || !main_bus_.Connect(crtc, c_crtc))
    return false;

  static const IOBus::Connector c_mem1[] = {
      {kPortReset, IOBus::portout, Memory::kReset},
      {0x31, IOBus::portout, Memory::kOut31},
      {0x32, IOBus::portout, Memory::kOut32},
      {0x33, IOBus::portout, Memory::kOut33},
      {0x34, IOBus::portout, Memory::kOut34},
      {0x35, IOBus::portout, Memory::kOut35},
      {0x5c, IOBus::portout, Memory::kOut5x},
      {0x5d, IOBus::portout, Memory::kOut5x},
      {0x5e, IOBus::portout, Memory::kOut5x},
      {0x5f, IOBus::portout, Memory::kOut5x},
      {0x70, IOBus::portout, Memory::kOut70},
      {0x71, IOBus::portout, Memory::kOut71},
      {0x78, IOBus::portout, Memory::kOut78},
      {0x99, IOBus::portout, Memory::kOut99},
      {0xe2, IOBus::portout, Memory::kOute2},
      {0xe3, IOBus::portout, Memory::kOute3},
      {0xf0, IOBus::portout, Memory::kOutf0},
      {0xf1, IOBus::portout, Memory::kOutf1},
      {kVRTC, IOBus::portout, Memory::kVRTC},
      {0x32, IOBus::portin, Memory::kIn32},
      {0x33, IOBus::portin, Memory::kIn33},
      {0x5c, IOBus::portin, Memory::kIn5c},
      {0x70, IOBus::portin, Memory::kIn70},
      {0x71, IOBus::portin, Memory::kIn71},
      {0xe2, IOBus::portin, Memory::kIne2},
      {0xe3, IOBus::portin, Memory::kIne3},
      {0, 0, 0}};
  mem1 = new Memory(DEV_ID('M', 'E', 'M', '1'));
  if (!mem1 || !main_bus_.Connect(mem1, c_mem1))
    return false;
  if (!mem1->Init(&main_mm_, &main_bus_, crtc, main_cpu_.GetWaits()))
    return false;

  if (!crtc->Init(&main_bus_, sched_.get(), dmac, draw))
    return false;

  static const IOBus::Connector c_knj1[] = {
      {0xe8, IOBus::portout, KanjiROM::kSetL},
      {0xe9, IOBus::portout, KanjiROM::kSetH},
      {0xe8, IOBus::portin, KanjiROM::kReadL},
      {0xe9, IOBus::portin, KanjiROM::kReadH},
      {0, 0, 0}};
  knj1 = new KanjiROM(DEV_ID('K', 'N', 'J', '1'));
  if (!knj1 || !main_bus_.Connect(knj1, c_knj1))
    return false;
  if (!knj1->Init("kanji1.rom"))
    return false;

  static const IOBus::Connector c_knj2[] = {
      {0xec, IOBus::portout, KanjiROM::kSetL},
      {0xed, IOBus::portout, KanjiROM::kSetH},
      {0xec, IOBus::portin, KanjiROM::kReadL},
      {0xed, IOBus::portin, KanjiROM::kReadH},
      {0, 0, 0}};
  knj2 = new KanjiROM(DEV_ID('K', 'N', 'J', '2'));
  if (!knj2 || !main_bus_.Connect(knj2, c_knj2))
    return false;
  if (!knj2->Init("kanji2.rom"))
    return false;

  static const IOBus::Connector c_scrn[] = {
      {kPortReset, IOBus::portout, Screen::kReset},
      {0x30, IOBus::portout, Screen::kOut30},
      {0x31, IOBus::portout, Screen::kOut31},
      {0x32, IOBus::portout, Screen::kOut32},
      {0x33, IOBus::portout, Screen::kOut33},
      {0x52, IOBus::portout, Screen::kOut52},
      {0x53, IOBus::portout, Screen::kOut53},
      {0x54, IOBus::portout, Screen::kOut54},
      {0x55, IOBus::portout, Screen::kOut55To5b},
      {0x56, IOBus::portout, Screen::kOut55To5b},
      {0x57, IOBus::portout, Screen::kOut55To5b},
      {0x58, IOBus::portout, Screen::kOut55To5b},
      {0x59, IOBus::portout, Screen::kOut55To5b},
      {0x5a, IOBus::portout, Screen::kOut55To5b},
      {0x5b, IOBus::portout, Screen::kOut55To5b},
      {0, 0, 0}};
  scrn = new Screen(DEV_ID('S', 'C', 'R', 'N'));
  if (!scrn || !main_bus_.Connect(scrn, c_scrn))
    return false;
  if (!scrn->Init(&main_bus_, mem1, crtc))
    return false;

  static const IOBus::Connector c_intc[] = {
      {kPortReset, IOBus::portout, InterruptController::kReset},
      {kPortInt0, IOBus::portout, InterruptController::kRequest},
      {kPortInt1, IOBus::portout, InterruptController::kRequest},
      {kPortInt2, IOBus::portout, InterruptController::kRequest},
      {kPortInt3, IOBus::portout, InterruptController::kRequest},
      {kPortInt4, IOBus::portout, InterruptController::kRequest},
      {kPortInt5, IOBus::portout, InterruptController::kRequest},
      {kPortInt6, IOBus::portout, InterruptController::kRequest},
      {kPortInt7, IOBus::portout, InterruptController::kRequest},
      {0xe4, IOBus::portout, InterruptController::kSetReg},
      {0xe6, IOBus::portout, InterruptController::kSetMask},
      {kPortIntAck, IOBus::portin, InterruptController::kIntAck},
      {0, 0, 0}};
  intc = new InterruptController(DEV_ID('I', 'N', 'T', 'C'));
  if (!intc || !main_bus_.Connect(intc, c_intc))
    return false;
  if (!intc->Init(&main_bus_, kPortIRQ, kPortInt0))
    return false;

  static const IOBus::Connector c_subsys[] = {
      {kPortReset, IOBus::portout, SubSystem::reset},
      {0xfc, IOBus::portout | IOBus::sync, SubSystem::m_set0},
      {0xfd, IOBus::portout | IOBus::sync, SubSystem::m_set1},
      {0xfe, IOBus::portout | IOBus::sync, SubSystem::m_set2},
      {0xff, IOBus::portout | IOBus::sync, SubSystem::m_setcw},
      {0xfc, IOBus::portin | IOBus::sync, SubSystem::m_read0},
      {0xfd, IOBus::portin | IOBus::sync, SubSystem::m_read1},
      {0xfe, IOBus::portin | IOBus::sync, SubSystem::m_read2},
      {0, 0, 0}};
  subsys = new SubSystem(DEV_ID('S', 'U', 'B', ' '));
  if (!subsys || !main_bus_.Connect(subsys, c_subsys))
    return false;

  static const IOBus::Connector c_sio[] = {
      {kPortReset, IOBus::portout, SIO::reset},
      {0x20, IOBus::portout, SIO::setdata},
      {0x21, IOBus::portout, SIO::setcontrol},
      {kPortSIOIn, IOBus::portout, SIO::acceptdata},
      {0x20, IOBus::portin, SIO::getdata},
      {0x21, IOBus::portin, SIO::getstatus},
      {0, 0, 0}};
  siotape = new SIO(DEV_ID('S', 'I', 'O', ' '));
  if (!siotape || !main_bus_.Connect(siotape, c_sio))
    return false;
  if (!siotape->Init(&main_bus_, kPortInt0, kPortSIOReq))
    return false;

  static const IOBus::Connector c_tape[] = {
      {kPortSIOReq, IOBus::portout, TapeManager::requestdata},
      {0x30, IOBus::portout, TapeManager::out30},
      {0x40, IOBus::portin, TapeManager::in40},
      {0, 0, 0}};
  if (!main_bus_.Connect(tapemgr_, c_tape))
    return false;
  if (!tapemgr_->Init(sched_.get(), &main_bus_, kPortSIOIn))
    return false;

  static const IOBus::Connector c_opn1[] = {
      {kPortReset, IOBus::portout, OPNInterface::reset},
      {0x32, IOBus::portout, OPNInterface::setintrmask},
      {0x44, IOBus::portout, OPNInterface::setindex0},
      {0x45, IOBus::portout, OPNInterface::writedata0},
      {0x46, IOBus::portout, OPNInterface::setindex1},
      {0x47, IOBus::portout, OPNInterface::writedata1},
      {kPortTimeSync, IOBus::portout, OPNInterface::sync},
      {0x44, IOBus::portin, OPNInterface::readstatus},
      {0x45, IOBus::portin, OPNInterface::readdata0},
      {0x46, IOBus::portin, OPNInterface::readstatusex},
      {0x47, IOBus::portin, OPNInterface::readdata1},
      {0, 0, 0}};
  opn1 = new OPNInterface(DEV_ID('O', 'P', 'N', '1'));
  if (!opn1 || !opn1->Init(&main_bus_, kPortInt4, kPortOPNIO, sched_.get()))
    return false;
  if (!main_bus_.Connect(opn1, c_opn1))
    return false;
  opn1->SetIMask(0x32, 0x80);

  static const IOBus::Connector c_opn2[] = {
      {kPortReset, IOBus::portout, OPNInterface::reset},
      {0xaa, IOBus::portout, OPNInterface::setintrmask},
      {0xa8, IOBus::portout, OPNInterface::setindex0},
      {0xa9, IOBus::portout, OPNInterface::writedata0},
      {0xac, IOBus::portout, OPNInterface::setindex1},
      {0xad, IOBus::portout, OPNInterface::writedata1},
      {0xa8, IOBus::portin, OPNInterface::readstatus},
      {0xa9, IOBus::portin, OPNInterface::readdata0},
      {0xac, IOBus::portin, OPNInterface::readstatusex},
      {0xad, IOBus::portin, OPNInterface::readdata1},
      {0, 0, 0}};
  opn2 = new OPNInterface(DEV_ID('O', 'P', 'N', '2'));
  if (!opn2->Init(&main_bus_, kPortInt4, kPortOPNIO, sched_.get()))
    return false;
  if (!opn2 || !main_bus_.Connect(opn2, c_opn2))
    return false;
  opn2->SetIMask(0xaa, 0x80);

  static const IOBus::Connector c_caln[] = {
      {kPortReset, IOBus::portout, Calendar::kReset},
      {0x10, IOBus::portout, Calendar::kOut10},
      {0x40, IOBus::portout, Calendar::kOut40},
      {0x40, IOBus::portin, Calendar::kIn40},
      {0, 0, 0}};
  caln = new Calendar(DEV_ID('C', 'A', 'L', 'N'));
  if (!caln || !caln->Init())
    return false;
  if (!main_bus_.Connect(caln, c_caln))
    return false;

  static const IOBus::Connector c_beep[] = {
      {0x40, IOBus::portout, pc88core::Beep::kOut40}, {0, 0, 0}};
  beep = new pc88core::Beep(DEV_ID('B', 'E', 'E', 'P'));
  if (!beep || !beep->Init())
    return false;
  if (!main_bus_.Connect(beep, c_beep))
    return false;

  static const IOBus::Connector c_siom[] = {
      {kPortReset, IOBus::portout, SIO::reset},
      {0xc2, IOBus::portout, SIO::setdata},
      {0xc3, IOBus::portout, SIO::setcontrol},
      {0, IOBus::portout, SIO::acceptdata},
      {0xc2, IOBus::portin, SIO::getdata},
      {0xc3, IOBus::portin, SIO::getstatus},
      {0, 0, 0}};
  siomidi = new SIO(DEV_ID('S', 'I', 'O', 'M'));
  if (!siomidi || !main_bus_.Connect(siomidi, c_siom))
    return false;
  if (!siomidi->Init(&main_bus_, 0, kPortSIOReq))
    return false;

  static const IOBus::Connector c_joy[] = {
      {kPortOPNIO, IOBus::portin, JoyPad::kGetDir},
      {kPortOPNIO2, IOBus::portin, JoyPad::kGetButton},
      {kVRTC, IOBus::portout, JoyPad::kVSync},
      {0, 0, 0}};
  joypad = new JoyPad();  // DEV_ID('J', 'O', 'Y', ' '));
  if (!joypad)
    return false;
  if (!main_bus_.Connect(joypad, c_joy))
    return false;

  return true;
}

// ---------------------------------------------------------------------------
//  デバイス接続(サブCPU)
//
bool PC88::ConnectDevices2() {
  static const IOBus::Connector c_cpu2[] = {
      {kPortReset2, IOBus::portout, static_cast<uint8_t>(Z80Int::kReset)},
      {kPortIRQ2, IOBus::portout, static_cast<uint8_t>(Z80Int::kIRQ)},
      {0, 0, 0}};
  if (!sub_bus_.Connect(&sub_cpu_, c_cpu2))
    return false;
  if (!sub_cpu_.Init(&sub_mm_, &sub_bus_, kPortIntAck2))
    return false;

  static const IOBus::Connector c_mem2[] = {
      {kPortIntAck2, IOBus::portin, SubSystem::intack},
      {0xfc, IOBus::portout | IOBus::sync, SubSystem::s_set0},
      {0xfd, IOBus::portout | IOBus::sync, SubSystem::s_set1},
      {0xfe, IOBus::portout | IOBus::sync, SubSystem::s_set2},
      {0xff, IOBus::portout | IOBus::sync, SubSystem::s_setcw},
      {0xfc, IOBus::portin | IOBus::sync, SubSystem::s_read0},
      {0xfd, IOBus::portin | IOBus::sync, SubSystem::s_read1},
      {0xfe, IOBus::portin | IOBus::sync, SubSystem::s_read2},
      {0, 0, 0}};
  if (!subsys || !sub_bus_.Connect(subsys, c_mem2))
    return false;
  if (!subsys->Init(&sub_mm_))
    return false;

  static const IOBus::Connector c_fdc[] = {
      {kPortReset2, IOBus::portout, FDC::kReset},
      {0xfb, IOBus::portout, FDC::kSetData},
      {0xf4, IOBus::portout, FDC::kDriveControl},
      {0xf8, IOBus::portout, FDC::kMotorControl},
      {0xf8, IOBus::portin, FDC::kTCIn},
      {0xfa, IOBus::portin, FDC::kGetStatus},
      {0xfb, IOBus::portin, FDC::kGetData},
      {0, 0, 0}};
  fdc = new pc88core::FDC(DEV_ID('F', 'D', 'C', ' '));
  if (!sub_bus_.Connect(fdc, c_fdc))
    return false;
  if (!fdc->Init(diskmgr_, sched_.get(), &sub_bus_, kPortIRQ2, kPortFDStat))
    return false;

  return true;
}

// ---------------------------------------------------------------------------
//  設定反映
//
void PC88::ApplyConfig(Config* cfg) {
  cfgflags = cfg->flags;
  cfgflag2 = cfg->flag2;

  base->SetSwitch(cfg);
  scrn->ApplyConfig(cfg);
  mem1->ApplyConfig(cfg);
  crtc->ApplyConfig(cfg);
  fdc->ApplyConfig(cfg);
  beep->EnableSING(!(cfg->flags & Config::kDisableSing));
  opn1->SetFMMixMode(!!(cfg->flag2 & Config::kUseFMClock));
  opn1->SetVolume(cfg);
  opn2->SetFMMixMode(!!(cfg->flag2 & Config::kUseFMClock));
  opn2->SetVolume(cfg);

  cpumode = (cfg->cpumode == Config::msauto)
                ? (cfg->mainsubratio > 1 ? ms21 : ms11)
                : (cfg->cpumode & 1);
  if ((cfg->flags & Config::kSubCPUControl) != 0)
    cpumode |= stopwhenidle;

  if (cfg->flags & pc88core::Config::kEnablePad) {
    joypad->SetButtonMode(cfg->flags & Config::kSwappedButtons
                              ? JoyPad::SWAPPED
                              : JoyPad::NORMAL);
  } else {
    joypad->SetButtonMode(JoyPad::DISABLED);
  }

  //  EnablePad((cfg->flags & pc88core::Config::enablepad) != 0);
  //  if (padenable)
  //      cfg->flags &= ~pc88core::Config::enablemouse;
  //  EnableMouse((cfg->flags & pc88core::Config::enablemouse) != 0);
}

// ---------------------------------------------------------------------------
//  音量変更
//
void PC88::SetVolume(pc88core::Config* cfg) {
  opn1->SetVolume(cfg);
  opn2->SetVolume(cfg);
}

// ---------------------------------------------------------------------------
//  1 フレーム分の時間を求める．
//
SchedTimeDelta PC88::GetFramePeriod() const {
  return crtc ? crtc->GetFramePeriod() : TimeKeeper::GetResolution() / 60;
}

// ---------------------------------------------------------------------------
//  仮想時間と現実時間の同期を取ったときに呼ばれる
//
void PC88::TimeSync() {
  main_bus_.Out(kPortTimeSync, 0);
}

}  // namespace pc88core
