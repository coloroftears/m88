// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  CriticalSection Class for Win32
// ---------------------------------------------------------------------------
//  $Id: CritSect.h,v 1.3 1999/08/01 14:19:13 cisc Exp $

#pragma once

#ifdef _WIN32

#include <windows.h>

class CriticalSection final {
 public:
  class Lock final {
   public:
    explicit Lock(CriticalSection& c) : cs_(c) { cs_.lock(); }
    ~Lock() { cs_.unlock(); }

   private:
    CriticalSection& cs_;
  };

  CriticalSection() { InitializeCriticalSection(&css_); }
  ~CriticalSection() { DeleteCriticalSection(&css_); }

  void lock() { EnterCriticalSection(&css_); }
  void unlock() { LeaveCriticalSection(&css_); }

 private:
  CRITICAL_SECTION css_;
};

#else  // !_WIN32

#include <mutex>

class CriticalSection final {
 public:
  class Lock final {
   public:
    explicit Lock(CriticalSection& c) : cs_(c) { cs_.lock(); }
    ~Lock() { cs_.unlock(); }

   private:
    CriticalSection& cs_;
  };

  CriticalSection() {}
  ~CriticalSection() {}

  bool try_lock() { return mtx_.try_lock(); }

  void lock() { mtx_.lock(); }
  void unlock() { mtx_.unlock(); }

 private:
  std::mutex mtx_;
};

#endif  // _WIN32
