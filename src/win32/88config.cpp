// ---------------------------------------------------------------------------
//  M88 - PC-8801 emulator
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: 88config.cpp,v 1.28 2003/09/28 14:35:35 cisc Exp $

#include "win32/88config.h"

#include <windows.h>

#include "common/clamp.h"
#include "interface/ifcommon.h"
#include "pc88/config.h"

static const char* AppName = "M88p2 for Windows";

namespace m88win {

// ---------------------------------------------------------------------------
//  LoadConfigEntry
//
static bool LoadConfigEntry(const char* inifile,
                            const char* entry,
                            int* value,
                            int def,
                            bool applydefault) {
  int n = GetPrivateProfileInt(AppName, entry, -1, inifile);

  if (n == -1 && applydefault)
    n = def;
  if (n != -1) {
    *value = n;
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
//  LoadConfigDirectory
//
void LoadConfigDirectory(Config* cfg,
                         const char* inifile,
                         const char* entry,
                         bool readalways) {
  if (readalways || (cfg->flags & pc88core::Config::kSaveDirectory)) {
    char path[MAX_PATH];
    if (GetPrivateProfileString(AppName, entry, ";", path, MAX_PATH, inifile)) {
      if (path[0] != ';')
        SetCurrentDirectory(path);
    }
  }
}

// ---------------------------------------------------------------------------
//  M88Config::LoadConfig
//
#define VOLUME_BIAS 100

#define LOADVOLUMEENTRY(key, vol)                                   \
  if (LoadConfigEntry(inifile, key, &n, VOLUME_BIAS, applydefault)) \
    vol = n - VOLUME_BIAS;

void LoadConfig(Config* cfg, const char* inifile, bool applydefault) {
  int n;

  n = Config::kSubCPUControl | Config::kSaveDirectory | Config::kUseArrowFor10 |
      Config::kOPNAOnA8;
  LoadConfigEntry(inifile, "Flags", &cfg->flags, n, applydefault);
  cfg->flags &= ~Config::kSpecialPalette;

  LoadConfigEntry(inifile, "Flag2", &cfg->flag2, 0, applydefault);
  cfg->flag2 &= ~(Config::kMask0 | Config::kMask1 | Config::kMask2);

  if (LoadConfigEntry(inifile, "CPUClock", &n, 40, applydefault))
    cfg->set_clock(Limit(n, 1000, 1));

  cfg->set_speed(1000);

  if (LoadConfigEntry(inifile, "RefreshTiming", &n, 1, applydefault))
    cfg->refreshtiming = Limit(n, 4, 1);

  if (LoadConfigEntry(inifile, "BASICMode", &n, Config::N88V2, applydefault)) {
    if (n == Config::N80 || n == Config::N88V1 || n == Config::N88V1H ||
        n == Config::N88V2 || n == Config::N802 || n == Config::N80V2 ||
        n == Config::N88V2CD)
      cfg->basicmode = Config::BASICMode(n);
    else
      cfg->basicmode = Config::N88V2;
  }

  if (LoadConfigEntry(inifile, "Sound", &n, 6, applydefault)) {
    static const uint16_t srate[] = {0,     11025, 22050, 44100,
                                     44100, 48000, 55467};
    if (n < 7)
      cfg->sound = srate[n];
    else
      cfg->sound = Limit(n, 55466 * 2, 8000);
  }

  if (LoadConfigEntry(inifile, "ERAMBank", &n, 4, applydefault))
    cfg->erambanks = Limit(n, 256, 0);

  if (LoadConfigEntry(inifile, "KeyboardType", &n, 0, applydefault))
    cfg->keytype = static_cast<Config::KeyType>(n);

  if (LoadConfigEntry(inifile, "Switches", &n, pc88core::DipSwitch::DefaultValue(),
                      applydefault))
    cfg->set_dipsw(n);

  if (LoadConfigEntry(inifile, "SoundBuffer", &n, 200, applydefault))
    cfg->soundbuffer = Limit(n, 1000, 50);

  if (LoadConfigEntry(inifile, "MouseSensibility", &n, 4, applydefault))
    cfg->mousesensibility = Limit(n, 10, 1);

  if (LoadConfigEntry(inifile, "CPUMode", &n, Config::msauto, applydefault))
    cfg->cpumode = Limit(n, 2, 0);

  if (LoadConfigEntry(inifile, "ROMEOLatency", &n, 100, applydefault))
    cfg->romeolatency = Limit(n, 500, 0);

  LOADVOLUMEENTRY("VolumeFM", cfg->volfm);
  LOADVOLUMEENTRY("VolumeSSG", cfg->volssg);
  LOADVOLUMEENTRY("VolumeADPCM", cfg->voladpcm);
  LOADVOLUMEENTRY("VolumeRhythm", cfg->volrhythm);
  LOADVOLUMEENTRY("VolumeBD", cfg->volbd);
  LOADVOLUMEENTRY("VolumeSD", cfg->volsd);
  LOADVOLUMEENTRY("VolumeTOP", cfg->voltop);
  LOADVOLUMEENTRY("VolumeHH", cfg->volhh);
  LOADVOLUMEENTRY("VolumeTOM", cfg->voltom);
  LOADVOLUMEENTRY("VolumeRIM", cfg->volrim);
}

// ---------------------------------------------------------------------------
//  SaveEntry
//
static bool SaveEntry(const char* inifile,
                      const char* entry,
                      int value,
                      bool applydefault) {
  char buf[MAX_PATH];
  if (applydefault || -1 != GetPrivateProfileInt(AppName, entry, -1, inifile)) {
    wsprintf(buf, "%d", value);
    WritePrivateProfileString(AppName, entry, buf, inifile);
    return true;
  }
  return false;
}

static bool SaveEntry(const char* inifile,
                      const char* entry,
                      const char* value,
                      bool applydefault) {
  bool apply = applydefault;
  if (!applydefault) {
    char buf[8];
    GetPrivateProfileString(AppName, entry, ";", buf, sizeof(buf), inifile);
    if (buf[0] != ';')
      apply = true;
  }
  if (apply)
    WritePrivateProfileString(AppName, entry, value, inifile);
  return apply;
}

// ---------------------------------------------------------------------------
//  SaveConfig
//
void SaveConfig(pc88core::Config* cfg, const char* inifile, bool writedefault) {
  char buf[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, buf);
  SaveEntry(inifile, "Directory", buf, writedefault);

  SaveEntry(inifile, "Flags", cfg->flags, writedefault);
  SaveEntry(inifile, "Flag2", cfg->flag2, writedefault);
  SaveEntry(inifile, "CPUClock", cfg->clock(), writedefault);
  SaveEntry(inifile, "RefreshTiming", cfg->refreshtiming, writedefault);
  SaveEntry(inifile, "BASICMode", cfg->basicmode, writedefault);
  SaveEntry(inifile, "Sound", cfg->sound, writedefault);
  SaveEntry(inifile, "Switches", cfg->dipsw(), writedefault);
  SaveEntry(inifile, "SoundBuffer", cfg->soundbuffer, writedefault);
  SaveEntry(inifile, "MouseSensibility", cfg->mousesensibility, writedefault);
  SaveEntry(inifile, "CPUMode", cfg->cpumode, writedefault);
  SaveEntry(inifile, "KeyboardType", cfg->keytype, writedefault);
  SaveEntry(inifile, "ERAMBank", cfg->erambanks, writedefault);
  SaveEntry(inifile, "ROMEOLatency", cfg->romeolatency, writedefault);

  SaveEntry(inifile, "VolumeFM", cfg->volfm + VOLUME_BIAS, writedefault);
  SaveEntry(inifile, "VolumeSSG", cfg->volssg + VOLUME_BIAS, writedefault);
  SaveEntry(inifile, "VolumeADPCM", cfg->voladpcm + VOLUME_BIAS, writedefault);
  SaveEntry(inifile, "VolumeRhythm", cfg->volrhythm + VOLUME_BIAS,
            writedefault);
  SaveEntry(inifile, "VolumeBD", cfg->volbd + VOLUME_BIAS, writedefault);
  SaveEntry(inifile, "VolumeSD", cfg->volsd + VOLUME_BIAS, writedefault);
  SaveEntry(inifile, "VolumeTOP", cfg->voltop + VOLUME_BIAS, writedefault);
  SaveEntry(inifile, "VolumeHH", cfg->volhh + VOLUME_BIAS, writedefault);
  SaveEntry(inifile, "VolumeTOM", cfg->voltom + VOLUME_BIAS, writedefault);
  SaveEntry(inifile, "VolumeRIM", cfg->volrim + VOLUME_BIAS, writedefault);
}

}  // namespace m88win
