%ifndef TSRPLEX_INC_
%define TSRPLEX_INC_

OUR_TSR_IDENTIFICATION_STRING_LENGTH     EQU        13
FIRST_APPLICATION_TSR_MULTIPLEX_ID       EQU        0C0h
LAST_APPLICATION_TSR_MULTIPLEX_ID        EQU        0FFh
NUMBER_OF_VALID_MULTIPLEX_IDS            EQU        (LAST_APPLICATION_TSR_MULTIPLEX_ID - FIRST_APPLICATION_TSR_MULTIPLEX_ID + 1)

; TSR Multiplex functions (AL register).
; Function 0 is installation check on all programs
struc TSR_MULTIPLEX
    .InstallationCheck                            resb    1    ; Must be function 0
    .GetResidentDataToESDIandPrepareForUnload     resb    1
endstruc

; Error codes from TSR related functions
struc TSR_ERROR
    .NoError                           resb    1    ; 0
    .AnotherInstanceOfOurTsrFound      resb    1    ; 1
    .NoFreeIDsAvailable                resb    1    ; 2
    .TsrHookAlreadyInstalled           resb    1    ; 3
    .TsrHookNotInstalled               resb    1    ; 4
    .TsrHookIsNoLongerTopmost          resb    1    ; 5
    .UnknownMultiplexFunction          resb    1    ; 6
    .FailedToFreeMemory                resb    1    ; 7
endstruc


struc TSR_HOOK
    .ffnPreviousHandler        resb    4
    .fnOurHandler              resb    2
    .bInterrupt                resb    1
    .bFlags                    resb    1
endstruc

; Flag defines for TSR_HOOK.bFlags
FLG_TSRHOOK_INSTALLED          EQU      (1<<0)



struc MULTIPLEX_SEARCH
    .fszTsrId                resb    4

    .bTSR_error              resb    1
    .bID                     resb    1
endstruc


struc MULTIPLEX_IO
    .wIoWordParam:
    .dwIoPtrParam            resb    4
    .bInFunction             resb    1
    .bInTsrID                resb    1
    .bOutTsrError            resb    1
                             resb    1
endstruc


%endif ; TSRPLEX_INC_
