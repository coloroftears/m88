// ---------------------------------------------------------------------------
//  M88 - PC-88 Emulator.
//  Copyright (C) cisc 1998.
// ---------------------------------------------------------------------------
//  CRTC (��PD3301) �̃G�~�����[�V����
// ---------------------------------------------------------------------------
//  $Id: crtc.h,v 1.19 2002/04/07 05:40:09 cisc Exp $

#if !defined(PC88_CRTC_H)
#define PC88_CRTC_H

#include "device.h"
#include "draw.h"
#include "schedule.h"

class Scheduler;

namespace PC8801
{
class PD8257;
class Config;
    
// ---------------------------------------------------------------------------
//  CRTC (��PD3301) �y�уe�L�X�g��ʍ���
//
class CRTC : public Device  
{
public:
    enum IDOut
    {
        reset=0, out, pcgout, setkanamode
    };
    enum IDIn
    {
        in = 0, getstatus,
    };

public:
    CRTC(const ID& id);
    ~CRTC();
    bool Init(IOBus* bus, Scheduler* s, PD8257* dmac, Draw* draw);
    const Descriptor* IFCALL GetDesc() const { return &descriptor; }
    
    void UpdateScreen(uint8* image, int bpl, Draw::Region& region, bool refresh);
    void SetSize();
    void ApplyConfig(const Config* config);
    int GetFramePeriod();

    uint IFCALL GetStatusSize();
    bool IFCALL SaveStatus(uint8* status);
    bool IFCALL LoadStatus(const uint8* status);
    
// CRTC Control
    void IOCALL Reset(uint=0, uint=0);
    void IOCALL Out(uint, uint data);
    uint IOCALL In(uint=0);
    uint IOCALL GetStatus(uint=0);
    void IOCALL PCGOut(uint, uint);
    void IOCALL SetKanaMode(uint, uint);
    
    void SetTextMode(bool color);
    void SetTextSize(bool wide);

private:
    enum Mode
    {
        inverse             = 1 << 0,   // reverse bit �Ɠ���
        color               = 1 << 1,
        control             = 1 << 2,
        skipline            = 1 << 3,
        nontransparent      = 1 << 4,
        attribute           = 1 << 5,
        clear               = 1 << 6,
        refresh             = 1 << 7,
        enable              = 1 << 8,
        suppressdisplay     = 1 << 9,
        resize              = 1 << 10,
    };

//  ATTR BIT �z�u       G  R  B  CG UL OL SE RE
    enum TextAttr
    {
        reverse             = 1 << 0,
        secret              = 1 << 1,
        overline            = 1 << 2,
        underline           = 1 << 3,
        graphics            = 1 << 4,
    };

    enum
    {
        dmabank = 2,
    };

private:
    enum
    {
        ssrev = 2,
    };
    struct Status
    {
        uint8 rev;
        uint8 cmdm;
        uint8 cmdc;
        uint8 pcount[2];
        uint8 param0[6];
        uint8 param1;
        uint8 cursor_x, cursor_y;
        int8  cursor_t;
        uint8 mode;
        uint8 status;
        uint8 column;
        uint8 attr;
        uint8 event;
        bool color;
    };

    void HotReset();
    bool LoadFontFile();
    void CreateTFont();
    void CreateTFont(const uint8*, int, int);
    void CreateKanaFont();
    void CreateGFont();
    uint Command(bool a0, uint data);

    void IOCALL StartDisplay(uint=0);
    void IOCALL ExpandLine(uint=0);
    void IOCALL ExpandLineEnd(uint=0);
    int  ExpandLineSub();

    void ClearText(uint8* image);
    void ExpandImage(uint8* image, Draw::Region& region);
    void ExpandAttributes(uint8* dest, const uint8* src, uint y);
    void ChangeAttr(uint8 code);

    const uint8* GetFont(uint c);
    const uint8* GetFontW(uint c);
    void ModifyFont(uint off, uint d);
    void EnablePCG(bool);

    void PutChar(packed* dest, uint8 c, uint8 a);
    void PutNormal(packed* dest, const packed* src);
    void PutReversed(packed* dest, const packed* src);
    void PutLineNormal(packed* dest, uint8 attr);
    void PutLineReversed(packed* dest, uint8 attr);

    void PutCharW(packed* dest, uint8 c, uint8 a);
    void PutNormalW(packed* dest, const packed* src);
    void PutReversedW(packed* dest, const packed* src);
    void PutLineNormalW(packed* dest, uint8 attr);
    void PutLineReversedW(packed* dest, uint8 attr);

    IOBus* bus;
    PD8257* dmac;
    Scheduler* scheduler;
    Scheduler::Event* sev;
    Draw* draw;

    int cmdm, cmdc;
    uint cursormode;
    uint linesize;
    bool line200;           // 15KHz ���[�h
    uint8 attr;
    uint8 attr_cursor;
    uint8 attr_blink;
    uint status;
    uint column;
    int linetime;
    uint frametime;
    uint pcgadr;
    uint pcgdat;
    
    int bpl;
    packed pat_col;
    packed pat_mask;
    packed pat_rev;
    int underlineptr;

    uint8* fontrom;
    uint8* hirarom;
    uint8* font;
    uint8* pcgram;
    uint8* vram[2];
    uint8* attrcache;

    uint bank;              // VRAM Cache �̃o���N
    uint tvramsize;         // 1��ʂ̃e�L�X�g�T�C�Y
    uint screenwidth;       // ��ʂ̕�
    uint screenheight;      // ��ʂ̍���

    uint cursor_x;          // �J�[�\���ʒu
    uint cursor_y;
    uint attrperline;       // 1�s������̃A�g���r���[�g��
    uint linecharlimit;     // 1�s������̃e�L�X�g����
    uint linesperchar;      // 1�s�̃h�b�g��
    uint width;             // �e�L�X�g��ʂ̕�
    uint height;            // �e�L�X�g��ʂ̍���
    uint blinkrate;         // �u�����N�̑��x
    int cursor_type;        // b0:blink, b1:underline (-1=none)
    uint vretrace;          // 
    uint mode;
    bool widefont;
    bool pcgenable;
    bool kanaenable;        // �Ђ�J�i�I��L��
    uint8 kanamode;         // b4 = �Ђ炪�ȃ��[�h

    uint8 pcount[2];
    uint8 param0[6];
    uint8 param1;
    uint8 event;

private:
    static const Descriptor descriptor;
    static const InFuncPtr  indef[];
    static const OutFuncPtr outdef[];
    
    static const packed colorpattern[8];
};


// ---------------------------------------------------------------------------
//  1 �t���[�����ɑ������鎞�Ԃ����߂�
//
inline int CRTC::GetFramePeriod()
{
    return linetime * (height + vretrace);
}

}

#endif // !defined(PC88_CRTC_H)
