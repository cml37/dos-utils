#ifndef HANDLERS_H_
#define HANDLERS_H_
#pragma once

// Headers required by this file
#include <stdint.h>
#include "tsrplex.h"

#define NUMBER_OF_TSR_HOOKS								3
#define TSR_IDENTIFICATION_STRING_LENGTH				13
#define TSR_IDENTIFICATION_STRING						"TsrPushNotify"
#define SYSTEM_TIMER_TICK_INTERRUPT_1C					0x1C
#define DOS_IDLE_INTERRUPT_28h							0x28
#define DOS_TSR_MULTIPLEX_INTERRUPT_2F					0x2F

#define TICKS_BEFORE_DRAWING_THE_PUSH_NOTIFICATION		18	// 18 * 55ms = 1 second
#define COLUMNS_ON_SCREEN								80

#define PACKET_RB_SIZE                      			PACKET_BUFFERS+1
#define PACKET_BUFFERS                      			2
#define PACKET_BUFFER_LEN                   			1514
#define PUSH_NOTIFICATION_BUFFER_SIZE       			75

typedef uint16_t EtherType;     // 16 bits representing an Ethernet frame type
typedef uint8_t  EthAddr_t[6];  // An Ethernet address is 6 bytes

typedef struct ResidentData
{
	__segment			pspSegment;
	volatile uint8_t	tickCounter;
	uint8_t				tsrID;
	char				tsrIdString[TSR_IDENTIFICATION_STRING_LENGTH];
	TSR_HOOK			tsrHooks[NUMBER_OF_TSR_HOOKS];
	char                data[PUSH_NOTIFICATION_BUFFER_SIZE];
    uint8_t             Buffer_first;   // Oldest packet in the ring, first to process
    uint8_t             Buffer_next;    // Newest packet in the ring, add incoming here.
    uint8_t             *Buffer[ PACKET_RB_SIZE ];
    uint16_t            Buffer_len[ PACKET_RB_SIZE ];
    uint8_t             *Buffer_fs[ PACKET_BUFFERS ];
    uint8_t             Buffer_pool[ PACKET_BUFFERS ][ PACKET_BUFFER_LEN ];
    uint8_t             Buffer_fs_index;
    const char          *PKT_DRVR_EYE_CATCHER;
    uint8_t             *Buffer_packetBeingCopied;
    uint16_t            Packet_handle;     // Provided by the packet driver
    uint8_t             Packet_int;  // Provided during initialization


} RESIDENT_DATA;

enum TsrHooks
{
	SYSTEM_TIMER_TICK_1Ch,
	DOS_IDLE_28h,
	DOS_TSR_MULTIPLEX_2Fh
};


//********//
//* Data *//
//********//

extern RESIDENT_DATA	g_residentData;


//*************//
//* Functions *//
//*************//

extern void SetCursorToPushNotificationLocation(void);
extern uint16_t GetSizeOfResidentSegmentInParagraphs(void);
extern void Buffer_init( RESIDENT_DATA far* residentData);
extern int8_t Packet_init( uint8_t packetInt, RESIDENT_DATA far* residentData );
extern void Buffer_startReceiving( RESIDENT_DATA far* residentData );


#endif /* HANDLERS_H_ */
