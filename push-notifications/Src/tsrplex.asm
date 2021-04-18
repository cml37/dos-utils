CPU 8086                        ; Allow 8088/8086 instructions only
BITS 16                            ; Set 16 bit code generation

%include "dos.inc"
%include "tsrplex.inc"


; Segment containing transient code
SEGMENT _TEXT CLASS=CODE PUBLIC

GLOBAL TsrPlex_FindMultiplexID_, TsrPlex_InstallTsrHooks_, TsrPlex_UninstallTsrHooks_
GLOBAL TsrPlex_MultiplexCall_, TsrPlex_GetCurrentPspSegment_
GLOBAL TsrPlex_FreeEnvironmentalBlockFromPSP_, TsrPlex_FreePspToRemoveTsrFromMemory_


;--------------------------------------------------------------------
; TsrPlex_GetCurrentPspSegment_
;    Parameters:
;        Nothing
;    Returns:
;        AX:        Segment of the PSP for current process
;    Corrupts registers:
;        Nothing
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
TsrPlex_GetCurrentPspSegment_:
    push    bx

    mov     ah, GET_CURRENT_PSP_ADDRESS
    int     DOS_INTERRUPT_21h
    xchg    ax, bx

    pop     bx
    ret


;--------------------------------------------------------------------
; TsrPlex_FindMultiplexID_
;    Parameters:
;        (DS:)AX:    Ptr to MULTIPLEX_SEARCH
;    Returns:
;        Nothing
;    Corrupts registers:
;        AX
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
TsrPlex_FindMultiplexID_:
    push    es
    push    bp
    push    di
    push    si
    push    dx
    push    cx
    push    bx

    push    ds
    push    ax
    xchg    bx, ax        ; DS:SI now points to MULTIPLEX_SEARCH
    les     di, [bx+MULTIPLEX_SEARCH.fszTsrId]
    call    FindAvailableMultiplexIDorPreviousInstance
    pop     bx
    pop     ds
    mov     [bx+MULTIPLEX_SEARCH.bTSR_error], ax    ; Store error and ID

    pop     bx
    pop     cx
    pop     dx
    pop     si
    pop     di
    pop     bp
    pop     es
    ret

;--------------------------------------------------------------------
; FindAvailableMultiplexIDorPreviousInstance
;    Parameters:
;        ES:DI:    Ptr to TSR ID string
;    Returns:
;        AL:        Error code
;        AH:        Available ID or previous instance if found
;    Corrupts:
;        All registers, including segments
;--------------------------------------------------------------------
ALIGN WORD_ALIGN
g_fpszOurTsrIdString:        resb    4
g_bTsrId:                    resb    1

ALIGN JUMP_ALIGN
FindAvailableMultiplexIDorPreviousInstance:
    mov     cx, NUMBER_OF_VALID_MULTIPLEX_IDS
    mov     [cs:g_fpszOurTsrIdString], di
    mov     [cs:g_fpszOurTsrIdString+2], es
    mov     [cs:g_bTsrId], ch        ; Clear to assume no free IDs

ALIGN JUMP_ALIGN
.CheckNextTsrID:
    mov     ah, FIRST_APPLICATION_TSR_MULTIPLEX_ID - 1
    add     ah, cl                    ; TSR ID now in AH
    push    cx

;--------------------------------------------------------------------
; .InstallationCheckForTsrInAH
;    Parameters:
;        AH:            TSR ID
;    Returns:
;        AL:            Non zero if TSR found
;        Registers:    Depends on TSR
;    Corrupts registers:
;        All, including segments
;--------------------------------------------------------------------
.InstallationCheckForTsrInAH:
    xor     bx, bx
    xor     cx, cx
    xor     dx, dx                ; BX, CX and DX should be set to zero
    xor     al, al                ; 0 = TSR_MULTIPLEX.InstallationCheck
    int     DOS_TSR_MULTIPLEX_INTERRUPT_2Fh
    cmp     al, 1
    jb      SHORT .StoreUnusedIDandCheckNextID    ; 0 = Not installed, OK to install
    je      SHORT .PrepareToCheckNextID           ; 1 = Not installed, do NOT install
                                                  ; FFh = Installed
;--------------------------------------------------------------------
; .CheckIfFoundTsrIsOurPreviouslyLoadedInstance
;    Parameters:
;        ES:DI:        Identification string if previously loaded instance of our TSR
;--------------------------------------------------------------------
.CheckIfFoundTsrIsOurPreviouslyLoadedInstance:
    mov     cx, OUR_TSR_IDENTIFICATION_STRING_LENGTH
    lds     si, [cs:g_fpszOurTsrIdString]
    repe    cmpsb    ; Compare DS:SI to ES:DI
    je      SHORT .PreviouslyLoadedInstanceOfOurTsrFound
    jmp     SHORT .PrepareToCheckNextID

ALIGN JUMP_ALIGN
.StoreUnusedIDandCheckNextID:
    mov     [cs:g_bTsrId], ah    ; Store available ID
.PrepareToCheckNextID:
    pop     cx
    loop    .CheckNextTsrID      ; Check next ID for previously loaded instance

    xor     ax, ax               ; AL = TSR_ERROR.NoErrors
    or      ah, [cs:g_bTsrId]    ; Load available ID to AH
    jz      SHORT .NoAvailableIDsAreAvailable
    ret
.NoAvailableIDsAreAvailable:
    mov     al, TSR_ERROR.NoFreeIDsAvailable
    ret

.PreviouslyLoadedInstanceOfOurTsrFound:
    pop     cx
    mov     al, TSR_ERROR.AnotherInstanceOfOurTsrFound
    ret


;--------------------------------------------------------------------
; TsrPlex_InstallTsrHooks_
;    Parameters:
;        DX:AX:    Ptr to array of TSR_HOOKs to install
;        BX:        Number of TSR_HOOKs in array
;    Returns:
;        AX:        TSR_ERROR code
;    Corrupts registers:
;        BX, DX
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
TsrPlex_InstallTsrHooks_:
    push    ds
    push    si
    push    cx

    xchg    si, ax
    mov     ds, dx
    mov     cx, bx
    call    TsrPlex_InstallTsrHooksFromDSSIwithCountInCX

    pop     cx
    pop     si
    pop     ds
    ret

;--------------------------------------------------------------------
; TsrPlex_InstallTsrHooksFromDSSIwithCountInCX
;    Parameters:
;        DS:SI:    Ptr to array of TSR_HOOKs to install
;        CX:        Number of TSR_HOOKs in array
;    Returns:
;        AX:        TSR_ERROR code
;    Corrupts registers:
;        BX, CX, DX
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
TsrPlex_InstallTsrHooksFromDSSIwithCountInCX:
    push    es
    push    di
    push    si

    xor     di, di            ; Clear errors
ALIGN JUMP_ALIGN
.InstallNextTsrHook:
    call    InstallTsrHookFromDSSI
    jnc     SHORT .AdjustSItoNextTsrHook
    xchg    di, ax            ; Store most previous error
.AdjustSItoNextTsrHook:
    add     si, BYTE TSR_HOOK_size
    loop    .InstallNextTsrHook

    xchg    ax, di            ; Error code to AX
    pop     si
    pop     di
    pop     es
    ret


;--------------------------------------------------------------------
; InstallTsrHookFromDSSI
;    Parameters:
;        DS:SI:    Ptr to TSR_HOOK to install
;    Returns:
;        AX:        TSR_ERROR code
;        CF:        Set if error
;                Cleared if no errors
;    Corrupts registers:
;        BX, DX, ES
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
InstallTsrHookFromDSSI:
    test    BYTE [si+TSR_HOOK.bFlags], FLG_TSRHOOK_INSTALLED
    jz      SHORT .BackupPreviousHandler
    mov     ax, TSR_ERROR.TsrHookAlreadyInstalled
    stc
    ret

ALIGN JUMP_ALIGN
.BackupPreviousHandler:
    mov     al, [si+TSR_HOOK.bInterrupt]
    call    GetHandlerToESBXforInterruptInAL
    mov     [si+TSR_HOOK.ffnPreviousHandler], bx
    mov     [si+TSR_HOOK.ffnPreviousHandler+2], es

.InstallOurHandler:
    or      BYTE [si+TSR_HOOK.bFlags], FLG_TSRHOOK_INSTALLED
    mov     dx, [si+TSR_HOOK.fnOurHandler]    ; Handler must be in the same segment as TSR_HOOK
    call    SetHandlerFromDSDXforInterruptInAL
    xor     ax, ax            ; No errors
    ret


;--------------------------------------------------------------------
; TsrPlex_UninstallTsrHooks_
;    Parameters:
;        DX:AX:    Ptr to array of TSR_HOOKs to uninstall
;        BX:        Number of TSR_HOOKs in array
;    Returns:
;        AX:        TSR_ERROR code
;    Corrupts registers:
;        BX, DX
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
TsrPlex_UninstallTsrHooks_:
    push    ds
    push    si
    push    cx

    xchg    si, ax
    mov     ds, dx
    mov     cx, bx
    call    TsrPlex_UninstallTsrHooksFromDSSIwithCountInDX

    pop     cx
    pop     si
    pop     ds
    ret

;--------------------------------------------------------------------
; TsrPlex_UninstallTsrHooksFromDSSIwithCountInCX
;    Parameters:
;        DS:SI:    Ptr to array of TSR_HOOKs to uninstall
;        CX:        Number of TSR_HOOKs in array
;    Returns:
;        AX:        TSR_ERROR code
;    Corrupts registers:
;        BX, CX, DX
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
TsrPlex_UninstallTsrHooksFromDSSIwithCountInDX:
    push    es
    push    si

    mov     dx, cx
    push    si
    call    CanTsrHooksBeUninstalledFromDSSIwithCountInCX
    pop     si
    jc      SHORT .ReturnWithErrorCodeInAX

    mov     cx, dx
ALIGN JUMP_ALIGN
.UninstallNextTsrHook:
    call    UninstallTsrHookFromDSSI
    add     si, BYTE TSR_HOOK_size
    loop    .UninstallNextTsrHook
.ReturnWithErrorCodeInAX:
    pop     si
    pop     es
    ret

;--------------------------------------------------------------------
; UninstallTsrHookFromDSSI
;    Parameters:
;        DS:SI:    Ptr to TSR_HOOK to uninstall
;    Returns:
;        AX:        TSR_ERROR code
;    Corrupts registers:
;        BX, DX, ES
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
UninstallTsrHookFromDSSI:
    call    CanTsrHookInDSSIbeUninstalled
    jnc     SHORT .RestorePreviousHandler
    ret

ALIGN JUMP_ALIGN
.RestorePreviousHandler:
    mov     al, [si+TSR_HOOK.bInterrupt]
    and     BYTE [si+TSR_HOOK.bFlags], ~FLG_TSRHOOK_INSTALLED
    push    ds
    lds     dx, [si+TSR_HOOK.ffnPreviousHandler]
    call    SetHandlerFromDSDXforInterruptInAL
    pop     ds
    xor     ax, ax         ; No errors
    ret


;--------------------------------------------------------------------
; CanTsrHooksBeUninstalledFromDSSIwithCountInCX
;    Parameters:
;        DS:SI:    Ptr to array of TSR_HOOKs
;        CX:        Number of TSR_HOOKs in array
;    Returns:
;        AX:        TSR_ERROR code
;        CF:        Set if error
;                Cleared if no errors
;    Corrupts registers:
;        BX, CX, SI, ES
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
CanTsrHooksBeUninstalledFromDSSIwithCountInCX:
    call    CanTsrHookInDSSIbeUninstalled
    cmp     al, TSR_ERROR.TsrHookIsNoLongerTopmost
    je      SHORT .AbortSinceAtLeastOneOfTheHooksCannotBeUninstalled
    add     si, BYTE TSR_HOOK_size
    loop    CanTsrHooksBeUninstalledFromDSSIwithCountInCX
    xor     ax, ax
    ret
.AbortSinceAtLeastOneOfTheHooksCannotBeUninstalled:
    stc
    ret

;--------------------------------------------------------------------
; CanTsrHookInDSSIbeUninstalled
;    Parameters:
;        DS:SI:    Ptr to TSR_HOOK
;    Returns:
;        AX:        TSR_ERROR code
;        CF:        Set if error
;                Cleared if no errors and TSR_HOOK can be uninstalled
;    Corrupts registers:
;        BX, ES
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
CanTsrHookInDSSIbeUninstalled:
    test    BYTE [si+TSR_HOOK.bFlags], FLG_TSRHOOK_INSTALLED
    jnz     SHORT .CheckIfHandlerCanBeRestored
    mov     ax, TSR_ERROR.TsrHookNotInstalled
    stc
    ret

ALIGN JUMP_ALIGN
.CheckIfHandlerCanBeRestored:
    mov     al, [si+TSR_HOOK.bInterrupt]
    call    GetHandlerToESBXforInterruptInAL
    cmp     bx, [si+TSR_HOOK.fnOurHandler]
    jne     SHORT .AnotherTsrInstalledOverOurHandler
    xor     ax, ax         ; No errors, handler can be uninstalled
    ret
.AnotherTsrInstalledOverOurHandler:
    mov     ax, TSR_ERROR.TsrHookIsNoLongerTopmost
    stc
    ret


;--------------------------------------------------------------------
; GetHandlerToESBXforInterruptInAL
;    Parameters:
;        AL:        Interrupt number
;    Returns:
;        ES:BX:    Ptr to interrupt handler function
;    Corrupts registers:
;        Nothing
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
GetHandlerToESBXforInterruptInAL:
    mov     ah, GET_INTERRUPT_VECTOR
    int     DOS_INTERRUPT_21h
    ret


;--------------------------------------------------------------------
; SetHandlerFromDSDXforInterruptInAL
;    Parameters:
;        AL:        Interrupt number
;        DS:DX:    Ptr to interrupt handler function 
;    Returns:
;        Nothing
;    Corrupts registers:
;        Nothing
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
SetHandlerFromDSDXforInterruptInAL:
    mov     ah, SET_INTERRUPT_VECTOR
    int     DOS_INTERRUPT_21h
    ret


;--------------------------------------------------------------------
; TsrPlex_FreeEnvironmentalBlockFromPSP_
;    Parameters:
;        AX:        PSP segment
;    Returns:
;        AX:        TSR_ERROR code
;    Corrupts registers:
;        Nothing
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
TsrPlex_FreeEnvironmentalBlockFromPSP_:
    push    es
    mov     es, ax     ; ES now points to PSP
    mov     es, [es:PSP.wEnvironmentSegment]
    jmp     SHORT FreeTsrMemoryBlockFromES

;--------------------------------------------------------------------
; TsrPlex_FreePspToRemoveTsrFromMemory_
;    Parameters:
;        AX:        PSP segment
;    Returns:
;        AX:        TSR_ERROR code
;    Corrupts registers:
;        Nothing
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
TsrPlex_FreePspToRemoveTsrFromMemory_:
    push    es
    mov     es, ax     ; ES now points to PSP
FreeTsrMemoryBlockFromES:
    mov     ah, FREE_MEMORY
    int     DOS_INTERRUPT_21h
    pop     es
    jc      SHORT .ReturnTsrErrorInAX
    xor     ax, ax     ; TSR_ERROR.NoError
    ret
.ReturnTsrErrorInAX:
    mov     ax, TSR_ERROR.FailedToFreeMemory
    ret


;--------------------------------------------------------------------
; TsrPlex_MultiplexCall_
;    Parameters:
;        (DS:)AX:    Ptr to MULTIPLEX_IO
;    Returns:
;        (DS:)AX:    Ptr to MULTIPLEX_IO
;    Corrupts registers:
;        AX
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
TsrPlex_MultiplexCall_:
    push    es
    push    di
    push    bx

    xchg    bx, ax
    les     di, [bx+MULTIPLEX_IO.dwIoPtrParam]
    mov     ax, [bx+MULTIPLEX_IO.bInFunction]    ; AL = Function, AH = ID
    call    TsrPlex_MultiplexCallWithFunctionInALandIDinAH
    mov     [bx+MULTIPLEX_IO.dwIoPtrParam], di
    mov     [bx+MULTIPLEX_IO.dwIoPtrParam+2], es
    mov     [bx+MULTIPLEX_IO.bOutTsrError], al

    pop     bx
    pop     di
    pop     es
    ret

;--------------------------------------------------------------------
; TsrPlex_MultiplexCallWithFunctionInALandIDinAH
;    Parameters:
;        AL:        Multiplex function from TSR_MULTIPLEX
;        AH:        TSR ID
;        ES:DI:    Pointer parameter (depends on function)
;    Returns:
;        AL:        TSR_ERROR code (when calling our TSR)
;        ES:DI:    Ptr return value (depends on function)
;    Corrupts registers:
;        AH
;--------------------------------------------------------------------
ALIGN JUMP_ALIGN
TsrPlex_MultiplexCallWithFunctionInALandIDinAH:
%ifdef USE_386
    push    gs
    push    fs
%endif
    push    ds
    push    bp
    push    si
    push    dx
    push    cx
    push    bx
    int     DOS_TSR_MULTIPLEX_INTERRUPT_2Fh
    pop     bx
    pop     cx
    pop     dx
    pop     si
    pop     bp
    pop     ds
%ifdef USE_386
    pop     fs
    pop     gs
%endif
    ret
