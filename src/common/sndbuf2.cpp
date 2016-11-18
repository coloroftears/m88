//  $Id: sndbuf2.cpp,v 1.2 2003/05/12 22:26:34 cisc Exp $

#include "headers.h"
#include "sndbuf2.h"
#include "misc.h"

// ---------------------------------------------------------------------------
//  Sound Buffer
//
SoundBuffer2::SoundBuffer2()
: source(0), buffer(0), buffersize(0)
{
    fillwhenempty = true;
}

SoundBuffer2::~SoundBuffer2()
{
    Cleanup();
}

bool SoundBuffer2::Init(SoundSource* _source, int _buffersize)
{
    CriticalSection::Lock lock(cs);
    
    delete[] buffer; buffer = 0;
    
    source = 0;
    if (!_source)
        return true;

    buffersize = _buffersize;

    ch = _source->GetChannels();
    read = 0; write = 0;

    if (!ch || buffersize <= 0)
        return false;
    
    buffer = new Sample[ch * buffersize];
    if (!buffer)
        return false;

    memset(buffer, 0, ch * buffersize * sizeof(Sample));
    source = _source;
    return true;
}

void SoundBuffer2::Cleanup()
{
    CriticalSection::Lock lock(cs);
    
    delete[] buffer; buffer = 0;
}

// ---------------------------------------------------------------------------
//  �o�b�t�@�ɉ���ǉ�
//
int SoundBuffer2::Fill(int samples)
{
    CriticalSection::Lock lock(cs);
    if (source)
        return FillMain(samples);
    return 0;
}

int SoundBuffer2::FillMain(int samples)
{
    // �����O�o�b�t�@�̋󂫂��v�Z
    int free = buffersize - GetAvail();

    if (!fillwhenempty && (samples > free-1))
    {
        int skip = Min(samples-free+1, buffersize-free);
        free += skip;
        read += skip;
        if (read > buffersize)
            read -= buffersize;
    }
    
    // �������ނׂ��f�[�^�ʂ��v�Z
    samples = Min(samples, free-1);
    if (samples > 0)
    {
        // ��������
        if (buffersize - write >= samples)
        {
            // ��x�ŏ�����ꍇ
            source->Get(buffer + write * ch, samples);
        }
        else
        {
            // �Q�x�ɕ����ď����ꍇ
            source->Get(buffer + write * ch, buffersize - write);
            source->Get(buffer, samples - (buffersize - write));
        }
        write += samples;
        if (write >= buffersize)
            write -= buffersize;
    }
    return samples;
}

// ---------------------------------------------------------------------------
//  �o�b�t�@���特��Ⴄ
//
int SoundBuffer2::Get(Sample* dest, int samples)
{
    CriticalSection::Lock lock(cs);
    if (!buffer)
        return 0;

    for (int s=samples; s>0; )
    {
        int xsize = Min(s, buffersize - read);
        
        int avail = GetAvail();

        // �����s���Ȃ�ǉ�
        if (xsize <= avail || fillwhenempty)
        {
            if (xsize > avail)
                FillMain(xsize - avail);
            memcpy(dest, buffer + read * ch, xsize * ch * sizeof(Sample));
            dest += xsize * ch;
            read += xsize;
        }
        else
        {
            if (avail > 0)
            {
                memcpy(dest, buffer + read * ch, avail * ch * sizeof(Sample));
                dest += avail * ch;
                read += avail;
            }
            memset(dest, 0, (xsize - avail) * ch * sizeof(Sample));
            dest += (xsize - avail) * ch;
        }
        
        s -= xsize;
        if (read >= buffersize)
            read -= buffersize;
    }
    return samples;
}

// ---------------------------------------------------------------------------
//  �o�b�t�@���󂩁C��ɋ߂���Ԃ�?
//
bool SoundBuffer2::IsEmpty()
{
    return GetAvail() == 0;
}

