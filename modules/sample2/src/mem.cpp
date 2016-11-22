//  $Id: mem.cpp,v 1.1 1999/10/10 01:43:28 cisc Exp $

#include "sample2/src/headers.h"
#include "sample2/src/mem.h"

GVRAMReverse::GVRAMReverse() : Device(0), mm(0), gvram(false) {}

GVRAMReverse::~GVRAMReverse() {
  if (mm && mid != -1) {
    mm->Disconnect(mid);
  }
}

bool GVRAMReverse::Init(IMemoryManager* _mm) {
  mm = _mm;
  mid = mm->Connect(this, true);
  if (mid == -1)
    return false;

  return true;
}

// ---------------------------------------------------------------------------
//  I/O �|�[�g���Ď�

void IFCALL GVRAMReverse::Out32(uint32_t, uint32_t r) {
  p32 = r;
  Update();
}

void IFCALL GVRAMReverse::Out35(uint32_t, uint32_t r) {
  p35 = r;
  Update();
}

void IFCALL GVRAMReverse::Out5x(uint32_t a, uint32_t) {
  p5x = a & 3;
  Update();
}

// ---------------------------------------------------------------------------
//  GVRAM ���I������Ă��鎞�ɂ�������t�b�N�֐�

uint32_t MEMCALL GVRAMReverse::MRead(void* p, uint32_t a) {
  GVRAMReverse* mp = reinterpret_cast<GVRAMReverse*>(p);
  if (a < 0xfe80)  // �\���̈���Ȃ�
  {
    a -= 0xc000;  // �A�h���X���㉺���]����
    int y = 199 - a / 80;
    int x = a % 80;
    a = 0xc000 + x + y * 80;
  }
  return mp->mm->Read8P(mp->mid, a);  // �{���̃�������ԂւƃA�N�Z�X
}

void MEMCALL GVRAMReverse::MWrite(void* p, uint32_t a, uint32_t d) {
  GVRAMReverse* mp = reinterpret_cast<GVRAMReverse*>(p);
  if (a < 0xfe80) {
    a -= 0xc000;
    int y = 199 - a / 80;
    int x = a % 80;
    a = 0xc000 + x + y * 80;
  }
  mp->mm->Write8P(mp->mid, a, d);
}

// ---------------------------------------------------------------------------
//  GVRAM ���I������Ă���΁CGVRAM �̈��������

void GVRAMReverse::Update() {
  bool g = false;
  if (p32 & 0x40) {
    p5x = 3;
    if (p35 & 0x80)
      g = true;
  } else {
    if (p5x < 3)
      g = true;
  }

  if (g != gvram) {
    gvram = g;
    if (g) {
      // ������
      mm->AllocR(mid, 0xc000, 0x4000, MRead);
      mm->AllocW(mid, 0xc000, 0x4000, MWrite);
    } else {
      // �J������
      mm->ReleaseR(mid, 0xc000, 0x4000);
      mm->ReleaseW(mid, 0xc000, 0x4000);
    }
  }
}

// ---------------------------------------------------------------------------
//  device description
//
const Device::Descriptor GVRAMReverse::descriptor = {/*indef*/ 0, outdef};

const Device::OutFuncPtr GVRAMReverse::outdef[] = {
    STATIC_CAST(Device::OutFuncPtr, &Out32),
    STATIC_CAST(Device::OutFuncPtr, &Out35),
    STATIC_CAST(Device::OutFuncPtr, &Out5x),
};

/*
const Device::InFuncPtr GVRAMReverse::indef[] =
{
    0,
};
*/
