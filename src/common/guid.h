// Copyright (C) coloroftears 2016.

#pragma once

#ifdef _WIN32

#include <windows.h>

#else  // !_WIN32

#include <stdint.h>

struct GUID {
  GUID(int32_t d1,
       int16_t d2,
       int16_t d3,
       char c0,
       char c1,
       char c2,
       char c3,
       char c4,
       char c5,
       char c6,
       char c7)
      : data1(d1), data2(d2), data3(d3) {
    data4[0] = c0;
    data4[1] = c1;
    data4[2] = c2;
    data4[3] = c3;
    data4[4] = c4;
    data4[5] = c5;
    data4[6] = c6;
    data4[7] = c7;
  }

  int32_t data1;
  int16_t data2;
  int16_t data3;
  char data4[8];
};

using REFIID = const GUID&;

#if defined(INIT_GUID)
#define DEFINE_GUID(name, data1, data2, data3, c0, c1, c2, c3, c4, c5, c6, c7) \
  GUID name(data1, data2, data3, c0, c1, c2, c3, c4, c5, c6, c7)
#else
#define DEFINE_GUID(name, data1, data2, data3, c0, c1, c2, c3, c4, c5, c6, c7) \
  extern GUID name
#endif

inline bool operator==(const GUID& a, const GUID& b) {
  return (a.data1 == b.data1) && (a.data2 == b.data2) && (a.data3 == b.data3) &&
         (a.data4[0] == b.data4[0]) && (a.data4[1] == b.data4[1]) &&
         (a.data4[2] == b.data4[2]) && (a.data4[3] == b.data4[3]) &&
         (a.data4[4] == b.data4[4]) && (a.data4[5] == b.data4[5]) &&
         (a.data4[6] == b.data4[6]) && (a.data4[7] == b.data4[7]);
}

#endif  // _WIN32
