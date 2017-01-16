// Copyright (C) coloroftears 2016.

#include "common/file.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FileIO::FileIO() : hfile(nullptr), flags(0), lorigin(0), error(Error::success) {
  path[0] = 0;
}

FileIO::FileIO(const char* filename, uint32_t flg)
    : hfile(nullptr), flags(0), lorigin(0), error(Error::success) {
  Open(filename, flg);
}

FileIO::~FileIO() {
  if (flags & open)
    Close();
}

bool FileIO::Open(const char* filename, uint32_t flg) {
  hfile = 0;
  flags = 0;
  if (flg & create) {
    hfile = fopen(filename, "w");
    flags = hfile ? open : 0;
  } else {
    hfile = fopen(filename, (flg & readonly ? "r" : "r+"));
    flags = (flg & readonly) | (hfile ? open : 0);
  }
  SetLogicalOrigin(0);

  stpncpy(path, filename, MAX_PATH);
  return !!(flags & open);
}

bool FileIO::CreateNew(const char* filename) {
  hfile = fopen(filename, "w");
  return true;
}

bool FileIO::Reopen(uint32_t /* flg*/) {
  hfile = freopen(path, "r", hfile);
  return true;
}

void FileIO::Close() {
  if (hfile)
    fclose(hfile);
  hfile = 0;
}

int32_t FileIO::Read(void* dest, int32_t len) {
  assert(hfile);
  return fread(dest, 1, len, hfile);
}

int32_t FileIO::Write(const void* src, int32_t len) {
  assert(hfile);
  return fwrite(src, 1, len, hfile);
}

bool FileIO::Seek(int32_t fpos, SeekMethod method) {
  assert(hfile);
  switch (method) {
    case begin:
      fseek(hfile, fpos + lorigin, SEEK_SET);
      break;
    case current:
      fseek(hfile, fpos, SEEK_CUR);
      break;
    case end:
      fseek(hfile, fpos, SEEK_END);
  }
  return true;
}

int32_t FileIO::Tellp() {
  assert(hfile);
  return ftell(hfile);
}

bool FileIO::SetEndOfFile() {
  return true;
}
