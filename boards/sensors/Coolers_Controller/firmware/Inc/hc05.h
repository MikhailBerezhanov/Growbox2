
#ifndef _HC05_H
#define _HC05_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define GET_AT_RESPONSE_TMOUT		100		// [ms]

int hc05_init(void);
_Bool hc05_connected(void);
int hc05_tryAT(void);

#endif
