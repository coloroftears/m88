// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1997, 2001.
// ---------------------------------------------------------------------------
//  $Id: keybconn.h,v 1.1 2002/04/07 05:40:10 cisc Exp $

#if !defined(win32_keybconn_h)
#define win32_keybconn_h

// ---------------------------------------------------------------------------

#include "types.h"
#include "device.h"

namespace PC8801 {
class WinKeyIF;
}

// ---------------------------------------------------------------------------
//  �f�o�C�X���o�X�ɐڑ�����ׂ̒�����s���D
//  �f�o�C�X�̊e�@�\�ɑΉ�����|�[�g�ԍ��̃}�b�s���O�������D
//  ���O���K�v���Ȃ��ꍇ�ڑ���ɔj�����Ă����܂�Ȃ��D
//
class DeviceConnector {
 public:
  virtual bool Disconnect();

 protected:
  bool Connect(IOBus* bus, Device* dev, const IOBus::Connector* conn);

 private:
  IOBus* bus;
  Device* dev;
};

// ---------------------------------------------------------------------------

class KeyboardConnector {
 public:
  bool Connect(IOBus* bus, PC8801::WinKeyIF* keyb);

 private:
  IOBus* bus;
  Device* dev;
};

#endif  // !defined(win32_keybconn_h)
