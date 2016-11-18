// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  �J�����_���v(��PD4990A) �̃G�~�����[�V����
// ---------------------------------------------------------------------------
//  $Id: calender.cpp,v 1.4 1999/10/10 01:47:04 cisc Exp $
//  �ETP, 1Hz �@�\��������

#include "headers.h"
#include "calender.h"
#include "misc.h"

//#define LOGNAME "calender"
#include "diag.h"

using namespace PC8801;

// ---------------------------------------------------------------------------
//  Construct/Destruct
//
Calender::Calender(const ID& id)
: Device(id)
{
    diff = 0;
    Reset();
}

Calender::~Calender()
{
}

// ---------------------------------------------------------------------------
//  ���E�o��
//
void IOCALL Calender::Reset(uint, uint)
{
    datain = 0;
    dataoutmode = 0;
    strobe = 0;
    cmd = 0x80;
    scmd = 0;
    for (int i=0; i<6; i++)
        reg[i] = 0;
}

uint IOCALL Calender::In40(uint)
{
    if (dataoutmode)
        return IOBus::Active((reg[0] & 1) << 4, 0x10);
    else
    {
//      SYSTEMTIME st;
//      GetLocalTime(&st);
//      return (st.wSecond & 1) << 4;
        return IOBus::Active(0, 0x10);
    }
}

void IOCALL Calender::Out10(uint, uint data)
{
    pcmd = data & 7;
    datain = (data >> 3) & 1;
}

void IOCALL Calender::Out40(uint, uint data)
{
    uint modified;
    modified = strobe ^ data;
    strobe = data;
    if (modified & data & 2)
        Command();
    if (modified & data & 4)
        ShiftData();
}

// ---------------------------------------------------------------------------
//  ����
//
void Calender::Command()
{
    if (pcmd == 7)
        cmd = scmd | 0x80;
    else
        cmd = pcmd;

    LOG1("Command = %.2x\n", cmd);
    switch (cmd & 15)
    {
    case 0x00:          // register hold
        hold = true;
        dataoutmode = false;
        break;

    case 0x01:          // register shift
        hold = false;
        dataoutmode = true;
        break;
    
    case 0x02:          // time set
        SetTime();
        hold = true;
        dataoutmode = true;
        break;

    case 0x03:          // time read
        GetTime();
        hold = true;
        dataoutmode = false;
        break;
    }
}

// ---------------------------------------------------------------------------
//  �f�[�^�V�t�g
//
void Calender::ShiftData()
{
    if (hold)
    {
        if (cmd & 0x80)
        {
            // shift sreg only
            LOG1("Shift HS %d\n", datain);
            scmd = (scmd >> 1) | (datain << 3);
        }
        else
        {
            LOG1("Shift HP -\n", datain);
        }
    }
    else
    {
        if (cmd & 0x80)
        {
            reg[0] = (reg[0] >> 1) | (reg[1] << 7);
            reg[1] = (reg[1] >> 1) | (reg[2] << 7);
            reg[2] = (reg[2] >> 1) | (reg[3] << 7);
            reg[3] = (reg[3] >> 1) | (reg[4] << 7);
            reg[4] = (reg[4] >> 1) | (reg[5] << 7);
            reg[5] = (reg[5] >> 1) | (scmd   << 7);
            scmd   = (scmd   >> 1) | (datain << 3);
            LOG1("Shift -S %d\n", datain);
        }
        else
        {
            reg[0] = (reg[0] >> 1) | (reg[1] << 7);
            reg[1] = (reg[1] >> 1) | (reg[2] << 7);
            reg[2] = (reg[2] >> 1) | (reg[3] << 7);
            reg[3] = (reg[3] >> 1) | (reg[4] << 7);
            reg[4] = (reg[4] >> 1) | (datain << 7);
            LOG1("Shift -P %d\n", datain);
        }
    }
}

// ---------------------------------------------------------------------------
//  ���Ԏ擾
//
void Calender::GetTime()
{
    time_t ct;
    
    ct = time(&ct) + diff;
    
    tm* lt = localtime(&ct);

    reg[5] = NtoBCD(lt->tm_year % 100);
    reg[4] = (lt->tm_mon+1) * 16 + lt->tm_wday;
    reg[3] = NtoBCD(lt->tm_mday);
    reg[2] = NtoBCD(lt->tm_hour);
    reg[1] = NtoBCD(lt->tm_min);
    reg[0] = NtoBCD(lt->tm_sec);
}

// ---------------------------------------------------------------------------
//  ���Ԑݒ�
//
void Calender::SetTime()
{
    time_t ct;
    time(&ct);
    tm* lt = localtime(&ct);

    tm nt;
    nt.tm_year = (cmd & 0x80) ? BCDtoN(reg[5]) : lt->tm_year;
    if (nt.tm_year < 70) nt.tm_year += 100;
    nt.tm_mon  = (reg[4]-1) >> 4;
    nt.tm_mday = BCDtoN(reg[3]);
    nt.tm_hour = BCDtoN(reg[2]);
    nt.tm_min  = BCDtoN(reg[1]);
    nt.tm_sec  = BCDtoN(reg[0]);
    nt.tm_isdst = 0;

    time_t at = mktime(&nt);
    diff = at - ct;
}

// ---------------------------------------------------------------------------
//  ��ԕۑ�
//
uint IFCALL Calender::GetStatusSize()
{
    return sizeof(Status);
}

bool IFCALL Calender::SaveStatus(uint8* s)
{
    Status* st = (Status*) s;
    st->rev = ssrev;
    st->t = time(&st->t) + diff;
    st->dataoutmode = dataoutmode;
    st->hold = hold;
    st->datain = datain;
    st->strobe = strobe;
    st->cmd = cmd;
    st->scmd = scmd;
    st->pcmd = pcmd;
    memcpy(st->reg, reg, 6);
    return true;
}

bool IFCALL Calender::LoadStatus(const uint8* s)
{
    const Status* st = (const Status*) s;
    if (st->rev != ssrev)
        return false;
    time_t ct;
    diff = st->t - time(&ct);
    dataoutmode = st->dataoutmode;
    hold = st->hold;
    datain = st->datain;
    strobe = st->strobe;
    cmd = st->cmd;
    scmd = st->scmd;
    pcmd = st->pcmd;
    memcpy(reg, st->reg, 6);
    return true;
}

// ---------------------------------------------------------------------------
//  device description
//
const Device::Descriptor Calender::descriptor = { indef, outdef };

const Device::OutFuncPtr Calender::outdef[] = 
{
    STATIC_CAST(Device::OutFuncPtr, &Calender::Reset),
    STATIC_CAST(Device::OutFuncPtr, &Calender::Out10),
    STATIC_CAST(Device::OutFuncPtr, &Calender::Out40),
};

const Device::InFuncPtr Calender::indef[] = 
{
    STATIC_CAST(Device::InFuncPtr, &Calender::In40),
};

