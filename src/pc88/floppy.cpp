// ---------------------------------------------------------------------------
//  M88 - PC-8801 Emulator.
//  Copyright (C) cisc 1998, 1999.
// ---------------------------------------------------------------------------
//  $Id: floppy.cpp,v 1.4 1999/07/29 14:35:31 cisc Exp $

#include "headers.h"
#include "floppy.h"

// ---------------------------------------------------------------------------
//  �\�z
//
FloppyDisk::FloppyDisk()
{
    ntracks = 0;
    curtrack = 0;
    cursector = 0;
    curtracknum = ~0;
}

FloppyDisk::~FloppyDisk()
{
}

// ---------------------------------------------------------------------------
//  ������
//
bool FloppyDisk::Init(DiskType _type, bool _readonly)
{
    static const int trtbl[] = { 84, 164, 164 };
    
    type = _type;
    readonly = _readonly;
    ntracks = trtbl[type];

    curtrack = 0;
    cursector = 0;
    curtracknum = ~0;
    return true;
}

// ---------------------------------------------------------------------------
//  �w��̃g���b�N�ɃV�[�N
//
void FloppyDisk::Seek(uint tr)
{
    if (tr != curtracknum)
    {
        curtracknum = tr;
        curtrack = tr < 168 ? tracks + tr : 0;
        cursector = 0;
    }
}

// ---------------------------------------------------------------------------
//  �Z�N�^��ǂݏo��
//
FloppyDisk::Sector* FloppyDisk::GetSector()
{
    if (!cursector)
    {
        if (curtrack)
            cursector = curtrack->sector;
    }
    
    Sector* ret = cursector;
    
    if (cursector)
        cursector = cursector->next;
    
    return ret;
}

// ---------------------------------------------------------------------------
//  �w�肵�� ID ������
//
bool FloppyDisk::FindID(IDR idr, uint density)
{
    if (!curtrack)
        return false;
    
    Sector* first = cursector;
    do
    {
        if (cursector)
        {
            if (cursector->id == idr)
            {
                if ((cursector->flags & 0xc0) == (density & 0xc0))
                    return true;
            }
            cursector = cursector->next;
        }
        else
        {
            cursector = curtrack->sector;
        }
    } while (cursector != first);
    
    return false;
}

// ---------------------------------------------------------------------------
//  �Z�N�^���𓾂�
//
uint FloppyDisk::GetNumSectors()
{
    int n = 0;
    if (curtrack)
    {
        Sector* sec = curtrack->sector;
        while (sec)
        {
            sec = sec->next;
            n++;
        }
    }
    return n;
}

// ---------------------------------------------------------------------------
//  �g���b�N���̃Z�N�^�f�[�^�̑��ʂ𓾂�
//
uint FloppyDisk::GetTrackSize()
{
    int size=0;

    if (curtrack)
    {
        Sector* sec = curtrack->sector;
        while (sec)
        {
            size += sec->size;
            sec = sec->next;
        }
    }
    return size;
}

// ---------------------------------------------------------------------------
//  Floppy::Resize
//  �Z�N�^�̃T�C�Y��傫�������ꍇ�ɂ�����Z�N�^�ׂ��̍Č�
//  sector �͌��ݑI�����Ă���g���b�N�ɑ����Ă���K�v������D
//
bool FloppyDisk::Resize(Sector* sec, uint newsize)
{
    assert(curtrack && sec);

    int extend = newsize - sec->size - 0x40;
    
    // sector ���g�� resize
    delete[] sec->image;
    sec->image = new uint8[newsize];
    sec->size = newsize;

    if (!sec->image)
    {
        sec->size = 0;
        return false;
    }
    
    cursector = sec->next;
    while (extend > 0 && cursector)
    {
        Sector* next = cursector->next;
        extend -= cursector->size + 0xc0;
        delete[] cursector->image;
        delete cursector;
        sec->next = cursector = next;
    }
    if (extend > 0)
    {
        int gapsize = GetTrackCapacity() - GetTrackSize() - 0x60 * GetNumSectors();
        extend -= gapsize;
    }
    while (extend > 0 && cursector)
    {
        Sector* next = cursector->next;
        extend -= cursector->size + 0xc0;
        delete[] cursector->image;
        delete cursector;
        curtrack->sector = cursector = next;
    }
    if (extend > 0)
        return false;

    return true;
}

// ---------------------------------------------------------------------------
//  FloppyDisk::FormatTrack
//
bool FloppyDisk::FormatTrack(int nsec, int secsize)
{
    Sector* sec;

    if (!curtrack)
        return false;
    
    // ������g���b�N��j��
    sec = curtrack->sector;
    while (sec)
    {
        Sector* next = sec->next;
        delete[] sec->image;
        delete sec;
        sec = next;
    }
    curtrack->sector = 0;
    
    if (nsec)
    {
        // �Z�N�^���쐬
        cursector = 0;
        for (int i=0; i<nsec; i++)
        {
            Sector* newsector = new Sector;
            if (!newsector)
                return false;
            curtrack->sector = newsector;
            newsector->next = cursector;
            newsector->size = secsize;
            if (secsize)
            {
                newsector->image = new uint8[secsize];
                if (!newsector->image)
                {
                    newsector->size = 0;
                    return false;
                }
            }
            else
            {
                newsector->image = 0;
            }
            cursector = newsector;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
//  �Z�N�^��ǉ�
//
FloppyDisk::Sector* FloppyDisk::AddSector(int size)
{
    if (!curtrack)
        return 0;
    
    Sector* newsector = new Sector;
    if (!newsector)
        return 0;
    if (size)
    {
        newsector->image = new uint8[size];
        if (!newsector->image)
        {
            delete newsector;
            return 0;
        }
    }
    else
    {
        newsector->image = 0;
    }
    
    if (!cursector)
        cursector = curtrack->sector;
    
    if (cursector)
    {
        newsector->next = cursector->next;
        cursector->next = newsector;
    }
    else
    {
        newsector->next = 0;
        curtrack->sector = newsector;
    }
    cursector = newsector;
    return newsector;
}

// ---------------------------------------------------------------------------
//  �g���b�N�̗e�ʂ𓾂�
//
uint FloppyDisk::GetTrackCapacity()
{
    static const int table[3] = { 6250, 6250, 10416 }; 
    return table[type];
}

// ---------------------------------------------------------------------------
//  �g���b�N�𓾂�
//
FloppyDisk::Sector* FloppyDisk::GetFirstSector(uint tr)
{
    if (tr < 168)
        return tracks[tr].sector;
    return 0;
}
