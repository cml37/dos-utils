CPU 8086                        ; Allow 8088/8086 instructions only
BITS 16                         ; Set 16 bit code generation

%include "dos.inc"


; Segment containing code
SEGMENT BEGTEXT CLASS=CODE PUBLIC

GLOBAL GetCursorCoordinates_, SetCursorCoordinates_
GLOBAL PrintCharacterWithTeletypeOutput_


;--------------------------------------------------------------------
; GetCursorCoordinates_
;    Parameters:
;        Nothing
;    Returns:
;        AL:        Hardware cursor column (X-coordinate)
;        AH:        Hardware cursor row (Y-coordinate)
;    Corrupts registers:
;        Nothing
;--------------------------------------------------------------------
GetCursorCoordinates_:
    push    cx
    push    bx

    mov     ah, 03h        ; VIDEO - GET CURSOR POSITION AND SIZE
    xor     bx, bx        ; BH = Page number
    int     10h
    xchg    ax, dx

    pop     bx
    pop     cx
    ret


;--------------------------------------------------------------------
; SetCursorCoordinates_
;    Parameters:
;        AX:        Cursor coordinates (low byte = column, high byte = row)
;    Returns:
;        Nothing
;    Corrupts registers:
;        AX
;--------------------------------------------------------------------
SetCursorCoordinates_:
    push    dx
    push    bx

    xchg    dx, ax        ; DX = Cursor coordinates
    mov     ah, 02h       ; VIDEO - SET CURSOR POSITION
    xor     bx, bx        ; BH = Page number
    int     10h

    pop     bx
    pop     dx
    ret


;--------------------------------------------------------------------
; PrintCharacterWithTeletypeOutput_
;    Parameters:
;        AL:        Character to print
;    Returns:
;        Nothing
;    Corrupts registers:
;        AX
;--------------------------------------------------------------------
PrintCharacterWithTeletypeOutput_:
    push    bx

    mov        ah, 0Eh       ; VIDEO - TELETYPE OUTPUT
    xor        bx, bx        ; BH = Page number
    int        10h

    pop        bx
    ret
