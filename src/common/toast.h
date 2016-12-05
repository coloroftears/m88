// Copyright (C) coloroftears 2016.

#pragma once

class Toast {
 public:
  static bool Show(int priority, int duration, const char* str, ...);
};
