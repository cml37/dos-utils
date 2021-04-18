#ifndef BIOSNDOS_H_
#define BIOSNDOS_H_
#pragma once

#include <stdint.h>

extern uint16_t GetCursorCoordinates(void);
extern void SetCursorCoordinates(uint16_t coordinates);
extern void PrintCharacterWithTeletypeOutput(char character);

#endif /* BIOSNDOS_H_ */
