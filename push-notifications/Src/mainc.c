#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include "handlers.h"
#include "biosndos.h"

static RESIDENT_DATA far* GetFarPointerToResidentData(void);
static TSR_ERROR FindLoadedInstanceOrNewID(RESIDENT_DATA far* pResidentData);
static inline bool PreviousInstanceFound(TSR_ERROR tsrError);
static inline bool NoTsrErrors(TSR_ERROR tsrError);

static TSR_ERROR InstallTsrToMemory(uint8_t packetInt, uint16_t udpDestPort, RESIDENT_DATA far* pResidentData);
static void ReturnToDosWithResidentSegmentInRAM(void);

static TSR_ERROR RemoveTsrFromMemory(uint16_t tsrID);
static RESIDENT_DATA far* GetResidentDataFromLoadedInstance(MULTIPLEX_IO* pMultiplexIO);
static void UnchainInterruptHandlers(MULTIPLEX_IO* pMultiplexIO, const RESIDENT_DATA far* pResidentDataInRam);
static TSR_ERROR UnloadLoadedInstanceFromMemory(__segment pspSegment);
static void RemovePushNotificationFromScreen(void);

static void PrintTsrError(TSR_ERROR tsrError);
static char* GetTsrErrorString(TSR_ERROR tsrError);


#define ReturnIfTsrError(tsrError)    \
    if ( (tsrError) != NO_ERROR )    \
        return (tsrError)


int main(int argc, char* argv[]) {
    uint8_t packetInt;
    uint16_t udpDestPort;

    RESIDENT_DATA far* pResidentData;
    TSR_ERROR          tsrError;

    pResidentData = GetFarPointerToResidentData();
    tsrError      = FindLoadedInstanceOrNewID(pResidentData);
    if ( PreviousInstanceFound(tsrError) ) {
        tsrError = RemoveTsrFromMemory(pResidentData->tsrID);
        if ( NoTsrErrors(tsrError) ) {
            RemovePushNotificationFromScreen();
            puts("TSR Unloaded Successfully.");
        }
    }
    else if ( NoTsrErrors(tsrError) ) {
        if (argc != 3) {
            puts("Usage: tsrpush.exe <Packet_Driver_Vector> <Listening_UDP_Port>");
            puts("Example: tsrpush.exe 0x65 20000");
            return -1;
        }
        sscanf( argv[1], "%x", &packetInt );
        udpDestPort = atoi( argv[2] );

        tsrError = InstallTsrToMemory(packetInt, udpDestPort, pResidentData);
        if ( NoTsrErrors(tsrError) ) {
            Buffer_startReceiving(pResidentData);
            ReturnToDosWithResidentSegmentInRAM();
        }
    }
    PrintTsrError(tsrError);
    return tsrError;
}


static RESIDENT_DATA far* GetFarPointerToResidentData(void) {
    __segment codeSegment;

    codeSegment = GetCodeSegment();
    return codeSegment:>&g_residentData;
}

static TSR_ERROR FindLoadedInstanceOrNewID(RESIDENT_DATA far* pResidentData) {
    MULTIPLEX_SEARCH  multiplexSearch;

    multiplexSearch.inFarIdString = FP_SEG(pResidentData):>pResidentData->tsrIdString;
    TsrPlex_FindMultiplexID(&multiplexSearch);
    pResidentData->tsrID = multiplexSearch.outID;
    return multiplexSearch.outError;
}

static inline bool PreviousInstanceFound(TSR_ERROR tsrError) {
    return tsrError == ANOTHER_INSTANCE_OF_OUR_TSR_LOADED;
}

static inline bool NoTsrErrors(TSR_ERROR tsrError) {
    return tsrError == NO_ERROR;
}

static TSR_ERROR InstallTsrToMemory(uint8_t packetInt, uint16_t udpDestPort, RESIDENT_DATA far* pResidentData) {
    TSR_ERROR    tsrError = NO_ERROR;
    int rc;
    pResidentData->pspSegment = TsrPlex_GetCurrentPspSegment();

    Buffer_init(pResidentData);

    rc = Packet_init(packetInt, udpDestPort, pResidentData);
    if ( rc != 0 ) {
        tsrError = COULD_NOT_LOAD_PACKET;
    }
    ReturnIfTsrError(tsrError);

    tsrError = TsrPlex_InstallTsrHooks(FP_SEG(pResidentData):>pResidentData->tsrHooks, NUMBER_OF_TSR_HOOKS);
    ReturnIfTsrError(tsrError);

    tsrError = TsrPlex_FreeEnvironmentalBlockFromPSP(pResidentData->pspSegment);
    PrintTsrError(tsrError);

    return NO_ERROR;    // Do not quit, just leave 256 byte environmental block in RAM
}

static void ReturnToDosWithResidentSegmentInRAM(void) {
    uint16_t paragraphsToKeepInRAM;
    paragraphsToKeepInRAM = GetSizeOfResidentSegmentInParagraphs();
    printf("TSR loaded in RAM with %u bytes used.\n", paragraphsToKeepInRAM<<4);
    puts("Run this program again to remove TSR from memory.");
    flushall();        // Flush all streams before returning to DOS
    _dos_keep(0, paragraphsToKeepInRAM);
}


static TSR_ERROR RemoveTsrFromMemory(uint16_t tsrID) {
    RESIDENT_DATA far*  pResidentDataInRam;
    MULTIPLEX_IO        multiplexIO;

    multiplexIO.inTsrID = tsrID;
    pResidentDataInRam = GetResidentDataFromLoadedInstance(&multiplexIO);
    ReturnIfTsrError(multiplexIO.outTsrError);

    Buffer_stopReceiving( pResidentDataInRam );
    Packet_release_type( pResidentDataInRam->Packet_handle, pResidentDataInRam->Packet_int );

    UnchainInterruptHandlers(&multiplexIO, pResidentDataInRam);
    ReturnIfTsrError(multiplexIO.outTsrError);

    return UnloadLoadedInstanceFromMemory(pResidentDataInRam->pspSegment);
}

static RESIDENT_DATA far* GetResidentDataFromLoadedInstance(MULTIPLEX_IO* pMultiplexIO) {
    pMultiplexIO->inFunction = GET_RESIDENT_DATA_AND_PREPARE_FOR_UNLOAD;
    TsrPlex_MultiplexCall(pMultiplexIO);
    return (RESIDENT_DATA far*) pMultiplexIO->ioPtrParam;
}

static void UnchainInterruptHandlers(MULTIPLEX_IO* pMultiplexIO, const RESIDENT_DATA far* pResidentDataInRam) {
    pMultiplexIO->outTsrError = TsrPlex_UninstallTsrHooks(
            FP_SEG(pResidentDataInRam):>pResidentDataInRam->tsrHooks, NUMBER_OF_TSR_HOOKS);
}

static TSR_ERROR UnloadLoadedInstanceFromMemory(__segment pspSegment) {
    // Call TsrPlex_FreeEnvironmentalBlockFromPSP() here if it has not
    // been freed earlier.
    /**/

    return TsrPlex_FreePspToRemoveTsrFromMemory(pspSegment);
}

static void RemovePushNotificationFromScreen(void) {
    uint16_t initialCursorLocation;
    uint16_t i;

    initialCursorLocation = GetCursorCoordinates();
    SetCursorToPushNotificationLocation();
    for (i=0; i<PUSH_NOTIFICATION_BUFFER_SIZE; i++)
        PrintCharacterWithTeletypeOutput(' ');

    SetCursorCoordinates(initialCursorLocation);
}

static void PrintTsrError(TSR_ERROR tsrError) {
    if ( tsrError != NO_ERROR ) {
        printf("Error occurred!\nError code: %Xh, Error description: %s\n",
            tsrError, GetTsrErrorString(tsrError));
    }
}

static char* GetTsrErrorString(TSR_ERROR tsrError) {
    switch ( tsrError ) {
        case NO_ERROR:
            return "NO_ERROR";
        case ANOTHER_INSTANCE_OF_OUR_TSR_LOADED:
            return "ANOTHER_INSTANCE_OF_OUR_TSR_LOADED";
        case NO_FREE_ID_AVAILABLE:
            return "NO_FREE_ID_AVAILABLE";
        case TSR_HOOK_ALREADY_INSTALLED:
            return "TSR_HOOK_ALREADY_INSTALLED";
        case TSR_HOOK_NOT_INSTALLED:
            return "TSR_HOOK_NOT_INSTALLED";
        case TSR_HOOK_NO_LONGER_ON_TOP_OF_CHAIN:
            return "TSR_HOOK_NO_LONGER_ON_TOP_OF_CHAIN";
        case UNKNOWN_MULTIPLEX_FUNCTION:
            return "UNKNOWN_MULTIPLEX_FUNCTION";
        case FAILED_TO_FREE_MEMORY:
            return "FAILED_TO_FREE_MEMORY";
        case COULD_NOT_LOAD_PACKET:
            return "COULD_NOT_LOAD_PACKET";
        default:
            return "Unknown Error";
        }
}
