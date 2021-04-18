#ifndef TSRRESIC_H_
#define TSRRESIC_H_
#pragma once

#include "tsrplex.h"


//*************//
//* Functions *//
//*************//

#pragma aux TsrResiC_JumpToInterruptHandler aborts; // Jump instead of call this function
extern void TsrResiC_JumpToInterruptHandler(INTERRUPT_HANDLER interruptHandler);


#endif /* TSRRESIC_H_ */
