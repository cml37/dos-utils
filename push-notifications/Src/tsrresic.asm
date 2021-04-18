CPU 8086                        ; Allow 8088/8086 instructions only
BITS 16                          ; Set 16 bit code generation

%include "dos.inc"


; Segment containing resident code
SEGMENT BEGTEXT CLASS=CODE PUBLIC

GLOBAL TsrResiC_JumpToInterruptHandler_


;--------------------------------------------------------------------
; Jump to this function goes to the END of Open Watcom
; void __interrupt ISR(union INTPACK registers).
;
; TsrResiC_JumpToInterruptHandler_
;    Parameters:
;        DX:AX:        Ptr to previous interrupt handler
;        SS:BP        Ptr to INTPACKW:
;                        struct INTPACKW {
;                                unsigned short gs;    [bp]
;                                unsigned short fs;    [bp+2]
;                                unsigned short es;    [bp+4]
;                                unsigned short ds;    [bp+6]
;                                unsigned short di;    [bp+8]
;                                unsigned short si;    [bp+10]
;                                unsigned short bp;    [bp+12]
;                                unsigned short sp;    [bp+14]
;                                unsigned short bx;    [bp+16]
;                                unsigned short dx;    [bp+18]
;                                unsigned short cx;    [bp+20]    ; Handler offset goes here
;                                unsigned short ax;    [bp+22]    ; Handler segment goes here
;                                unsigned short ip;    [bp+24]
;                                unsigned short cs;    [bp+26]
;                                unsigned flags;    }    [bp+28]
;    Returns:
;        Nothing (returns from Interrupt Service Routine)
;    Corrupts registers:
;        Nothing
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
TsrResiC_JumpToInterruptHandler_:
    xchg    ax, dx        ; Segment to AX, offset to DX
    mov     cx, dx        ; Target handler now in AX:CX
    xchg    [bp+20], cx    ; Restore CX, move handler offset to stack
    xchg    [bp+22], ax    ; Restore AX, move handler segment to stack

%ifdef USE_386
    pop     gs
    pop     fs
%else
    pop     bx         ; Skip GS since 286 and older does not have it
    pop     bx         ; Skip FS since 286 and older does not have it
%endif
    pop     es
    pop     ds
    pop     di
    pop     si
    pop     bp
    pop     bx         ; Skip SP
    pop     bx
    pop     dx
    retf          ; "Return" to previous interrupt handler
