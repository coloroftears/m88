// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  FDIF �p PIO (8255) �̃G�~�����[�V����
//  �E8255 �̃��[�h 0 �̂݃G�~�����[�g
// ---------------------------------------------------------------------------
//  $Id: pio.h,v 1.2 1999/03/24 23:27:13 cisc Exp $

#ifndef PC88_FDIF_PIO_H
#define PC88_FDIF_PIO_H

#include "device.h"

namespace PC8801
{

class PIO
{

public:
    PIO() { Reset(); }
        
    void Connect(PIO* p) { partner = p; }
    
    void Reset();
    void SetData(uint adr, uint data);
    void SetCW(uint data);
    uint Read0();
    uint Read1();
    uint Read2();
    
    uint Port(uint num) { return port[num]; }

private:
    uint8 port[4];
    uint8 readmask[4];
    PIO* partner;
};

}

// ---------------------------------------------------------------------------
//  �|�[�g�ɏo��
//
inline void PC8801::PIO::SetData(uint adr, uint data)
{
    adr &= 3;
    port[adr] = data;
}

// ---------------------------------------------------------------------------
//  �|�[�g�������
//
inline uint PC8801::PIO::Read0()
{
    uint data = partner->Port(1);
    return (data & readmask[0]) | (port[1] & ~readmask[0]);
}

inline uint PC8801::PIO::Read1()
{
    uint data = partner->Port(0);
    return (data & readmask[1]) | (port[1] & ~readmask[1]);
}

inline uint PC8801::PIO::Read2()
{
    uint data = partner->Port(2);
    data = ((data << 4) & 0xf0) + ((data >> 4) & 0x0f);     // rotate 4 bits
    return (data & readmask[2]) | (port[2] & ~readmask[2]);
}

#endif // FDIF_PIO_H
