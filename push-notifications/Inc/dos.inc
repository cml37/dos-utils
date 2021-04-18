%ifndef DOS_INC_
%define DOS_INC_

JUMP_ALIGN                            EQU        2
WORD_ALIGN                            EQU        2

; DOS interrupts
DOS_INTERRUPT_21h                     EQU        21h
DOS_TSR_MULTIPLEX_INTERRUPT_2Fh       EQU        2Fh

; DOS 21h functions
SET_INTERRUPT_VECTOR                  EQU        25h
GET_SYSTEM_TIME                       EQU        2Ch
GET_INTERRUPT_VECTOR                  EQU        35h
FREE_MEMORY                           EQU        49h
GET_CURRENT_PSP_ADDRESS               EQU        62h


; DOS Program Segment Prefix (PSP, first 256 (100h) bytes on top of program)
struc PSP
    .int20hInstruction                resb    2
    .wSizeOfMemoryInParagraphs        resb    2
    .reservedAt4h                     resb    1
    .callToDosFunctionDispatcher      resb    5
    .fpInt22hTerminate                resb    4
    .fpInt23hCtrlC                    resb    4
    .fpInt24hCriticalError            resb    4
    .reservedAt16h                    resb    22
    .wEnvironmentSegment              resb    2
    .reservedAt2Eh                    resb    34
    .int21hAndRetfInstructions        resb    3
    .reservedAt53h                    resb    9
    .FCB1                             resb    16
    .FCB2                             resb    20
    .DiskTransferArea:
    .bCommandLineLength               resb    1
    .szCommandLine                    resb    127
endstruc


%endif ; DOS_INC_
