; ----------------------------------------------------------------------------
;   �f�B�X�N�A�N�Z�X ROM
;
;   �g�p���郁�����̈�
;   �풓:   f310-f31b   (�\�����荞�݃x�N�^��)
;   ��풓: f04f-f086   (PAINT/CIRCLE ���[�N)
;       e9b9-eaba   (�s���̓o�b�t�@)
;
;   �g�p�\����
;
;   CMD LOAD "�t�@�C����" [,R]
;       �t�@�C�����Ŏw�肳�ꂽ BASIC �v���O������ǂݍ���
;       ASCII/���ԃR�[�h�̂ǂ���̃t�@�C�����Ή����Ă���
;
;   CMD SAVE "�t�@�C����" [,A/B]
;       BASIC �v���O�������w�肳�ꂽ�t�@�C�����ŕۑ�
;       �I�v�V���������̏ꍇ ASCII �e�L�X�g�Ƃ��ĕۑ������D
;       (,A �I�v�V���������l)
;       ���ԃR�[�h�̂܂܂ŕۑ�����ɂ� ,B �I�v�V����������
;
;   CMD BLOAD "�t�@�C����" [,�ǂݍ��݃A�h���X] [,R]
;       �@�B��t�@�C����ǂݍ���
;
;   CMD BSAVE "�t�@�C����", �J�n�A�h���X, ����
;       �@�B��t�@�C���̕ۑ�
;

SNERR       equ 0393h
FCERR       equ 0b06h

DIOPORT     equ 0d0h
DIOCMD      equ DIOPORT+0
DIOSTAT     equ DIOPORT+0
DIODAT      equ DIOPORT+1

LINKER      equ 05bdh
LINEA2N     equ 01bbbh
TEXTW2R     equ 44d5h
TEXTR2W     equ 44a4h

TXTTAB      equ 0e658h
TXTEND      equ 0eb18h

temp0       equ 0eab9h

tempramworkarea equ 0f04fh
RAMCodeArea equ 0e9b9h

        org 6000h

;       org $-4
;bloadhdr   dw  $+4
;       dw  codeend


eromid      db  'R4'

; ----------------------------------------------------------------------------
;   ����������
;
erominit:
        jr  initialize

TitleMsg    db  "M88 Disk I/O Extention 0.23",13,10,0

initialize:
        call    DetectHW
        ret c
        
        call    SetupRAMRoutine
        
        ld  hl,TitleMsg
        call    putmsg
        
        
        ld  hl,block0start
        ld  de,block0
        ld  bc,block0len
        ldir
        in  a,(71h)
        ld  (CMDStub+1),a

;   CMD �t�b�N���g�p����Ă��邩�`�F�b�N
        ld  hl,(0eeb7h)
        ld  de,4dc1h
        or  a
        sbc hl,de
        jr  nz, init_err
        
;       �g���R�}���h�Ƀt�b�N
        ld  hl,CMDStub
        ld  (0eeb7h),hl
        ret
        
init_err:
        ld  hl,InstallFailMsg
        call    putmsg
        ret

InstallFailMsg  db  "CMD extention is alredy in use. Installation aborted.",13,10,0

    incl "stub.asm"

; ----------------------------------------------------------------------------
;   �g���R�}���h�G���g��
;
CMDEntry:
        call    SetupRAMRoutine
        
        ld  a,(hl)
        inc hl
        cp  0c8h
        jp  z,LSAVE
        cp  0c1h
        jp  z,LLOAD
        cp  0d4h
        jp  z,LBLOAD
        cp  0d5h
        jp  z,LBSAVE
        call    ROMCALL
        dw  SNERR

; ----------------------------------------------------------------------------
;   �g���n�[�h���o
;
DetectHW:
        ld  a,80h
        out (DIOCMD),a
        in  a,(DIOSTAT)
        cp  1
        jr  nz,detecthw_e
        xor a
        out (DIODAT),a
        in  a,(DIOSTAT)
        or  a
        ret z
detecthw_e:
        scf
        ret

; ----------------------------------------------------------------------------
;   �t�@�C���l�[���̎擾
;   
getfilename:
        call    ROMCALL
        dw  11d3h       ; eval
        push    hl
        call    ROMCALL
        dw  56c9h
        ld  b,(hl)
        inc b
        dec b
        jp  z,getfilename_e
        inc hl
        ld  e,(hl)
        inc hl
        ld  d,(hl)
        
        ld  a,80h       ; set filename
        out (DIOCMD),a
        ld  a,b
        out (DIODAT),a
        
getfilename_1:
        ld  a,(de)
        out (DIODAT),a
        inc de
        djnz    getfilename_1
        
        pop hl
        ret
        
getfilename_e:
        call    ROMCALL
        dw  FCERR

 incl "load.asm"
 incl "save.asm"
 incl "misc.asm"


codeend     equ $
 
        end
