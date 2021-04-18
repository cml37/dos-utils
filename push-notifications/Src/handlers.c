
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

#include <stdbool.h>
#include <dos.h>
#include <string.h>
#include "handlers.h"
#include "tsrresic.h"
#include "biosndos.h"

// All data and code here goes to the resident BEGTEXT code segment
#pragma data_seg("BEGTEXT", "CODE");
#pragma code_seg("BEGTEXT", "CODE");

// Function prototypes
static void Buffer_free( const uint8_t *buffer);
static void Packet_process_internal( void );
static void __interrupt SystemTimerTickAtVector1Ch(union INTPACK registers);

static void __interrupt DosIdleAtVector28h(union INTPACK registers);
static inline bool TimeToDrawPushNotification(void);
static void PrintPushNotificationToScreen(char *data);

static void __interrupt DosTsrMultiplexAtVector2Fh(union INTPACK registers);
static inline bool OurHandlerWasCalled(uint8_t tsrID);


//********//
//* Data *//
//********//

RESIDENT_DATA g_residentData =
{
        0,                            // RESIDENT_DATA.pspSegment
        0,                            // .tickCounter
        0,                            // .tsrID
        TSR_IDENTIFICATION_STRING,    // .tsrIdString[]
        {                             // .tsrHooks[]
            { 0, (INTERRUPT_HANDLER_OFFSET)SystemTimerTickAtVector1Ch, SYSTEM_TIMER_TICK_INTERRUPT_1C, 0 },
            { 0, (INTERRUPT_HANDLER_OFFSET)DosIdleAtVector28h, DOS_IDLE_INTERRUPT_28h, 0 },
            { 0, (INTERRUPT_HANDLER_OFFSET)DosTsrMultiplexAtVector2Fh, DOS_TSR_MULTIPLEX_INTERRUPT_2F, 0 }
        }
};

// The magic receiver function.
//
// This is the function called by the packet driver when it receives a packet.
// There are actually two calls; one call to get a buffer for the new packet
// and one call to tell us when the packet has been copied into the buffer.
//
// Once the second call is made a new buffer is available to use for processing.

static void __interrupt receiver( union INTPACK r ) {

  LoadCodeSegmentToDataSegment();

  if ( r.w.ax == 0 ) {
    if ( (r.w.cx>PACKET_BUFFER_LEN) || (g_residentData.Perform_Packet_Processing == 0) ) {
      r.w.es = r.w.di = 0;
    }
    else {
      r.w.es = FP_SEG( g_residentData.Buffer);
      r.w.di = FP_OFF( g_residentData.Buffer);
    }
  }
  else {
    g_residentData.Buffer_len = r.w.cx;
    Packet_process_internal();
    PrintPushNotificationToScreen(g_residentData.data);
  }

  // Custom epilog code.  Some packet drivers can handle the normal
  // compiler generated epilog, but the Xircom PE3-10BT drivers definitely
  // can not.

_asm {
  pop ax
  pop ax
  pop es
  pop ds
  pop di
  pop si
  pop bp
  pop bx
  pop bx
  pop dx
  pop cx
  pop ax
  retf
};
}

// Packet_process_internal
//

static void Packet_process_internal( void ) {

  uint8_t    *packet;
  uint16_t   packet_len;
  EtherType  protocol;
  uint16_t   etherType;
  EthAddr_t  *fromEthAddr;

  packet = g_residentData.Buffer;
  packet_len = g_residentData.Buffer_len;


  // Packet routing.
  //
  // Bytes 13 and 14 (packet[12] and packet[13]) have the protocol type
  // in them.
  //
  //   Ip:  0800
  //
  // Compare 16 bits at a time.  Because we are on a little-endian machine
  // flip the bytes that we are comparing with when treating the values
  // as 16 bit words.  (The registered EtherType word is already flipped
  // to be in network byte order.)

  protocol    = ( ( uint16_t * ) packet )[6];
  etherType   = ntohs( ( ( uint16_t * ) packet )[6] );
  fromEthAddr = ( EthAddr_t * )( &packet[6] );

  // If this is an IP Packet
  if ( etherType == 0x0800 ) {
      UDP_HEADER *udpHeader;
      uint8_t    ipHeaderLen;
      uint16_t   udpDestPort;
      uint16_t   udpLen;
      char*      udpData;
      uint8_t    i;
      uint8_t    ending_pos = 0;
      IP_HEADER  *ipHeader = ( IP_HEADER * )( packet + ETHERNET_HEADER_SIZE_BYTES );

      if ( ipHeader->protocol == UDP_PROTOCOL ) {

          ipHeaderLen = ( ( ipHeader->versHlen & 0xF ) << 2 );
          udpHeader = ( UDP_HEADER * ) ( packet + ETHERNET_HEADER_SIZE_BYTES + ipHeaderLen );
          udpDestPort = ntohs( udpHeader->dst );

          if ( udpDestPort == g_residentData.udpDestPort ) {
              udpLen  = ntohs( udpHeader->len );
              udpData = ( char* ) (packet + ETHERNET_HEADER_SIZE_BYTES + ipHeaderLen + sizeof( UDP_HEADER ) );

              for ( i = 0; i < udpLen - sizeof( UDP_HEADER ); i++ ) {
                  if ( i == PUSH_NOTIFICATION_BUFFER_SIZE ) {
                      break;
                  }
                  g_residentData.data[i] = udpData[i];
                  ending_pos = i;
              }

              for ( i = ending_pos + 1; i < PUSH_NOTIFICATION_BUFFER_SIZE; i++ ) {
                  g_residentData.data[i] = ' ';
              }
          }
      }
   }
}

int8_t Packet_release_type( uint16_t Packet_handle, uint8_t Packet_int ) {

  int8_t rc = -1;

  union REGS inregs, outregs;
  struct SREGS segregs;

  inregs.h.ah = 0x3;
  inregs.x.bx = Packet_handle;

  int86x( Packet_int, &inregs, &outregs, &segregs );

  if ( !outregs.x.cflag ) {
    rc = 0;
  }

  return rc;
}



//***********************************//
//* System Timer Tick Interrupt 1Ch *//
//***********************************//

static void __interrupt SystemTimerTickAtVector1Ch(union INTPACK registers)
{
    LoadCodeSegmentToDataSegment();
    g_residentData.tickCounter++;

    // Cannot use _chain_intr() because it is part of the libraries that
    // were removed from memory when we called _dos_keep().
    // TsrResiC_JumpToInterruptHandler() must be at the end of ISR function!
    TsrResiC_JumpToInterruptHandler(g_residentData.tsrHooks[SYSTEM_TIMER_TICK_1Ch].previousHandler);
}


//**************************//
//* DOS Idle Interrupt 28h *//
//**************************//

static void __interrupt DosIdleAtVector28h(union INTPACK registers)
{
    LoadCodeSegmentToDataSegment();
    if ( TimeToDrawPushNotification() )
        PrintPushNotificationToScreen(g_residentData.data);

    TsrResiC_JumpToInterruptHandler(g_residentData.tsrHooks[DOS_IDLE_28h].previousHandler);
}

static inline bool TimeToDrawPushNotification(void)
{
    return (g_residentData.tickCounter % TICKS_BEFORE_DRAWING_THE_PUSH_NOTIFICATION) == 0;
}

static void PrintPushNotificationToScreen(char *data)
{
    uint16_t initialCursorLocation;
    uint8_t  i = 0;
    initialCursorLocation = GetCursorCoordinates();
    SetCursorToPushNotificationLocation();

    for ( i = 0; i < PUSH_NOTIFICATION_BUFFER_SIZE; i++ ){
        PrintCharacterWithTeletypeOutput( data[i] );
    }
    SetCursorCoordinates(initialCursorLocation);
}

void SetCursorToPushNotificationLocation(void)
{
    SetCursorCoordinates(COLUMNS_ON_SCREEN - PUSH_NOTIFICATION_BUFFER_SIZE - 1);
}


//***********************************//
//* DOS TSR Multiplex Interrupt 2Fh *//
//***********************************//

static void __interrupt DosTsrMultiplexAtVector2Fh(union INTPACK registers)
{
    LoadCodeSegmentToDataSegment();
    if ( OurHandlerWasCalled(registers.h.ah) )
    {
        switch ( (MULTIPLEX_FUNCTION) registers.h.al )
        {
        case INSTALLATION_CHECK:
            registers.w.di    = (uint16_t) g_residentData.tsrIdString;
            registers.w.es    = GetCodeSegment();
            registers.h.al    = 0xFF;            // TSR installed
            return;

        case GET_RESIDENT_DATA_AND_PREPARE_FOR_UNLOAD:
            registers.w.di    = (uint16_t) &g_residentData;
            registers.w.es    = GetCodeSegment();
            registers.h.al    = NO_ERROR;
            return;

        default:
            registers.h.al    = UNKNOWN_MULTIPLEX_FUNCTION;
            return;
        }
    }
    else TsrResiC_JumpToInterruptHandler(g_residentData.tsrHooks[DOS_TSR_MULTIPLEX_2Fh].previousHandler);
}

static inline bool OurHandlerWasCalled(uint8_t tsrID)
{
    return g_residentData.tsrID == tsrID;
}


void Buffer_startReceiving( RESIDENT_DATA far* residentData ) { residentData->Perform_Packet_Processing = 1; }

void Buffer_stopReceiving( RESIDENT_DATA far* residentData ) {  residentData->Perform_Packet_Processing = 0; }


// Packet_init
//
// First make sure that there is a packet driver located where the caller
// has specified.  If so, try to register to receive all possible EtherTypes
// from it.

int8_t Packet_init( uint8_t packetInt, uint16_t udpDestPort, RESIDENT_DATA far* residentData ) {

  union REGS inregs, outregs;
  struct SREGS segregs;

  uint16_t far *intVector = ( uint16_t far * ) MK_FP( 0x0, packetInt * 4 );

  uint16_t eyeCatcherOffset = *intVector;
  uint16_t eyeCatcherSegment = *( intVector+1 );

  char far *eyeCatcher = ( char far * ) MK_FP( eyeCatcherSegment, eyeCatcherOffset );

  eyeCatcher += 3; // Skip three bytes of executable code

  if ( _fmemcmp( residentData->PKT_DRVR_EYE_CATCHER, eyeCatcher, 8 ) != 0 ) {
    return -1;
  }

  inregs.h.ah = 0x2;                 // Function 2 (access_type)
  inregs.h.al = 0x1;                 // Interface class (ethernet)
  inregs.x.bx = 0xFFFF;              // Interface type (card/mfg)
  inregs.h.dl = 0;                   // Interface number (assume 0)
  segregs.ds = FP_SEG( NULL );       // Match all EtherTypes
  inregs.x.si = FP_OFF( NULL );
  inregs.x.cx = 0;
  segregs.es = FP_SEG( receiver );   // Receiver function
  inregs.x.di = FP_OFF( receiver );

  int86x( packetInt, &inregs, &outregs, &segregs );

  if ( outregs.x.cflag ) {
    return outregs.h.dh;
  }

  residentData->Packet_int = packetInt;
  residentData->Packet_handle = outregs.x.ax;
  residentData->udpDestPort = udpDestPort;

  return 0;

}


void Buffer_init(RESIDENT_DATA far* residentData) {

  // Initialize Perform_Packet_Processing to zero so that we don't start receiving
  // data before the other data structures are ready.  This happens because
  // we have to initialize the packet driver to get our MAC address, but
  // we need the MAC address to initialize things like ARP.  This allows
  // us to initialize the packet driver without fear of receiving a packet.
  // (Any packet we get in this state will be tossed.)

  residentData->Perform_Packet_Processing = 0;
  residentData->PKT_DRVR_EYE_CATCHER  = "PKT DRVR";
  residentData->Packet_int = 0x0;
  residentData->data[0] = 0;
}


//**************************//
//* End of BEGTEXT segment *//
//**************************//

/**
 * This function must the the last one in the BEGTEXT segment!
 *
 * GetSizeOfResidentSegment() must locate after all other resident functions and global variables.
 * This function must be defined in the last source file if there are more than
 * one source file containing resident functions.
 */
uint16_t GetSizeOfResidentSegmentInParagraphs(void)
{
    uint16_t    sizeInBytes;

    sizeInBytes    = (uint16_t) GetSizeOfResidentSegmentInParagraphs;
    sizeInBytes    += 0x100;    // Size of PSP (do not add when building .COM executable)
    sizeInBytes    += 15;       // Make sure nothing gets lost in partial paragraph

    return sizeInBytes >> 4;
}
