
/*
   Portions and patterns taken from mTCP:
   Copyright (C) 2005-2020 Michael B. Brutman (mbbrutman@gmail.com)
   mTCP web page: http://www.brutman.com/mTCP

   mTCP is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   mTCP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with mTCP.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef HANDLERS_H_
#define HANDLERS_H_
#pragma once

// Headers required by this file
#include <stdint.h>
#include "tsrplex.h"

#define NUMBER_OF_TSR_HOOKS                              3
#define TSR_IDENTIFICATION_STRING_LENGTH                 13
#define TSR_IDENTIFICATION_STRING                        "TsrPushNotify"
#define SYSTEM_TIMER_TICK_INTERRUPT_1C                   0x1C
#define DOS_IDLE_INTERRUPT_28h                           0x28
#define DOS_TSR_MULTIPLEX_INTERRUPT_2F                   0x2F

#define TICKS_BEFORE_DRAWING_THE_PUSH_NOTIFICATION       18    // 18 * 55ms = 1 second
#define COLUMNS_ON_SCREEN                                80

#define PACKET_BUFFER_LEN                                1514
#define PUSH_NOTIFICATION_BUFFER_SIZE                    75

#define UDP_PROTOCOL                                     17
#define ETHERNET_HEADER_SIZE_BYTES                       14

typedef uint16_t EtherType;     // 16 bits representing an Ethernet frame type
typedef uint8_t  EthAddr_t[6];  // An Ethernet address is 6 bytes

typedef struct ResidentData
{
    __segment           pspSegment;
    volatile uint8_t    tickCounter;
    uint8_t             tsrID;
    char                tsrIdString[TSR_IDENTIFICATION_STRING_LENGTH];
    TSR_HOOK            tsrHooks[NUMBER_OF_TSR_HOOKS];
    char                data[PUSH_NOTIFICATION_BUFFER_SIZE];
    uint8_t             Perform_Packet_Processing;
    uint8_t             Buffer[ PACKET_BUFFER_LEN ];
    uint16_t            Buffer_len;
    const char          *PKT_DRVR_EYE_CATCHER;
    uint8_t             *Buffer_packetBeingCopied;
    uint16_t            Packet_handle;     // Provided by the packet driver
    uint8_t             Packet_int;  // Provided during initialization
    uint16_t            udpDestPort; // Provided during initialization

} RESIDENT_DATA;


typedef struct IpHeader {

    uint8_t  versHlen;       // vers:4, Hlen:4
    uint8_t  service_type;
    uint16_t total_length;

    // Fragmentation support
    //   flags 0 to 15
    //   0: always 0
    //   1: 0=May Fragment, 1=Don't Fragment
    //   2: 0=Last Fragment, 1=More Fragments
    //   3 to 15: Fragment offset in units of 8 bytes

    uint16_t ident;
    uint16_t flags;          // flags:3, frag_offset:13

    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t chksum;

    uint8_t ip_src[4];
    uint8_t ip_dest[4];
} IP_HEADER;


typedef struct UdpHeader {
    // All of these need to be in network byte order.
    uint16_t src;
    uint16_t dst;
    uint16_t len;
    uint16_t chksum;
} UDP_HEADER;


enum TsrHooks
{
    SYSTEM_TIMER_TICK_1Ch,
    DOS_IDLE_28h,
    DOS_TSR_MULTIPLEX_2Fh
};


//********//
//* Data *//
//********//

extern RESIDENT_DATA    g_residentData;


//*************//
//* Functions *//
//*************//

extern void SetCursorToPushNotificationLocation(void);
extern uint16_t GetSizeOfResidentSegmentInParagraphs(void);
extern void Buffer_init( RESIDENT_DATA far* residentData);
extern int8_t Packet_init( uint8_t packetInt, uint16_t udpDestPort, RESIDENT_DATA far* residentData );
extern int8_t Packet_release_type( uint16_t Packet_handle, uint8_t Packet_int );
extern void Buffer_startReceiving( RESIDENT_DATA far* residentData );
extern void Buffer_stopReceiving( RESIDENT_DATA far* residentData );


#endif /* HANDLERS_H_ */
