#ifndef BIOSNDOS_H_
#define BIOSNDOS_H_
#pragma once

#include <stdint.h>


typedef struct DosTime
{
	uint8_t		minute;
	uint8_t		hour;
	uint8_t		oneHundredthsOfSecond;
	uint8_t		second;
} DOS_TIME;


extern uint16_t GetCursorCoordinates(void);
extern void SetCursorCoordinates(uint16_t coordinates);
extern void PrintCharacterWithTeletypeOutput(char character);


#endif /* BIOSNDOS_H_ */
