#ifndef TSRPLEX_H_
#define TSRPLEX_H_
#pragma once

typedef void far __interrupt (*INTERRUPT_HANDLER)(void);
typedef void (*INTERRUPT_HANDLER_OFFSET)(union INTPACK registers);

typedef struct TsrHook
{
    INTERRUPT_HANDLER           previousHandler;
    INTERRUPT_HANDLER_OFFSET    ourHandler;
    uint8_t                     interruptNumber;
    uint8_t                     flags;
} TSR_HOOK;

// Defines for TSR_HOOK.flags
#define FLG_TSRHOOK_INSTALLED   (1<<0)


typedef enum MultiplexFunction
{
    INSTALLATION_CHECK,
    GET_RESIDENT_DATA_AND_PREPARE_FOR_UNLOAD
} MULTIPLEX_FUNCTION;


typedef enum TsrError
{
    NO_ERROR,
    ANOTHER_INSTANCE_OF_OUR_TSR_LOADED,
    NO_FREE_ID_AVAILABLE,
    TSR_HOOK_ALREADY_INSTALLED,
    TSR_HOOK_NOT_INSTALLED,
    TSR_HOOK_NO_LONGER_ON_TOP_OF_CHAIN,
    UNKNOWN_MULTIPLEX_FUNCTION,
    FAILED_TO_FREE_MEMORY,
    COULD_NOT_LOAD_PACKET
} TSR_ERROR;


typedef struct MultiplexSearch
{
    char far*    inFarIdString;

    TSR_ERROR    outError;        // Must be a byte (Open Watcom default for small enums)
    uint8_t      outID;
} MULTIPLEX_SEARCH;


typedef struct MultiplexIO
{
    union
    {
        void far*         ioPtrParam;
        uint16_t          ioWordParam;
    };
    MULTIPLEX_FUNCTION    inFunction;
    uint8_t               inTsrID;
    union
    {
        TSR_ERROR         outTsrError;
        uint16_t          align;
    };
} MULTIPLEX_IO;


//******************************//
//* In-Line Assembly functions *//
//******************************//

extern uint16_t htons( uint16_t );
#pragma aux htons = \
  "xchg al, ah"     \
  parm [ax]         \
  modify [ax]       \
  value [ax];

#define ntohs( x ) htons( x )

extern __segment GetCodeSegment(void);
#pragma aux GetCodeSegment =                       \
    "mov ax, cs"    /* Copy CS to AX */            \
    value [ax];        /* Return value in AX */

extern void LoadCodeSegmentToDataSegment(void);
#pragma aux LoadCodeSegmentToDataSegment =         \
    "push cs"        /* Copy CS... */              \
    "pop ds"        /* ...to DS */                 \
    modify [];        /* No registers corrupted */

extern void EnableInterrupts(void);
#pragma aux EnableInterrupts =                      \
    "sti"                                           \
    modify [];

extern void DisableInterrupts(void);
#pragma aux DisableInterrupts =                     \
    "cli"                                           \
    modify [];


//*************//
//* Functions *//
//*************//

extern __segment TsrPlex_GetCurrentPspSegment(void);
extern void TsrPlex_FindMultiplexID(MULTIPLEX_SEARCH* pMultiplexSearch);
extern TSR_ERROR TsrPlex_InstallTsrHooks(TSR_HOOK far tsrHooks[], uint16_t count);
extern TSR_ERROR TsrPlex_UninstallTsrHooks(TSR_HOOK far tsrHooks[], uint16_t count);
extern void TsrPlex_MultiplexCall(MULTIPLEX_IO* pMultiplexIO);
extern TSR_ERROR TsrPlex_FreeEnvironmentalBlockFromPSP(__segment pspSegment);
extern TSR_ERROR TsrPlex_FreePspToRemoveTsrFromMemory(__segment pspSegment);


#endif /* TSRPLEX_H_ */
