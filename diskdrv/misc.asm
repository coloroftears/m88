; ----------------------------------------------------------------------------
;   ���b�Z�[�W�\��
;
putmsg:
        ld  a,(hl)
        inc hl
        or  a
        ret z
        call    ROMCALL
        dw  3e0dh
        jr  putmsg

; ----------------------------------------------------------------------------
;   HL ����� ASCIZ ������(0-255)�̒������v�Z
;
strlen:
        push    hl
strlen_1:
        ld  a,(hl)
        inc hl
        or  a
        jr  nz,strlen_1
        ld  a,l
        pop hl
        sub l
        ret

; ----------------------------------------------------------------------------
;   HL �� 10 �i���̕�����ɕϊ��� (DE)�`  �Ɋi�[
;
num2dec:
        xor a
        ld  bc,10000
        call    num2dec_1
        ld  bc,1000
        call    num2dec_1
        ld  bc,100
        call    num2dec_1
        ld  bc,10
        call    num2dec_1
        ld  a,l
        add a,'0'
        ld  (de),a
        inc de
        ret
num2dec_1:
        push    af
        ld  a,0ffh
        or  a
num2dec_2:
        inc a
        sbc hl,bc
        jr  nc,num2dec_2
        add hl,bc
        pop bc
        or  b
        ret z
        or  30h
        ld  (de),a
        inc de
        ld  a,30h
        ret
