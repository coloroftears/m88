//  $Id: pcinfo.h,v 1.1 1999/08/26 08:09:59 cisc Exp $

#ifndef incl_pcinfo_h
#define incl_pcinfo_h

#include "win32/types.h"

struct DeviceInfo {
  enum {
    direv = 0x102,
  };
  enum {
    soundsource = 1 << 0,
  };

  int size;
  int rev;
  uint32_t id;
  int flags;

  const int* outporttable;
  const int* inporttable;

  void (*soundmix)(void*, int32_t* s, int len);
  bool (*setrate)(void*, uint rate);
  void (*outport)(void*, uint port, uint data);
  uint (*inport)(void*, uint port);
  void (*eventproc)(void*, uint arg);
  uint (*snapshot)(void*, uint8* data, bool save);
};

struct PCInfo {
  enum { mem_func = 3 };

  int size;
  // DMA
  int (*DMARead)(void*, uint bank, uint8* data, uint nbytes);
  int (*DMAWrite)(void*, uint bank, uint8* data, uint nbytes);

  // Page
  bool (*MemAcquire)(void*,
                     uint page,
                     uint npages,
                     void* read,
                     void* write,
                     uint flags);
  bool (*MemRelease)(void*, uint page, uint npages, uint flags);

  // Timer
  void* (*AddEvent)(void*, uint count, uint arg);
  bool (*DelEvent)(void*, void*);
  uint (*GetTime)(void*);

  // Sound
  void (*SoundUpdate)(void*);
};

#endif  // incl_pcinfo_h