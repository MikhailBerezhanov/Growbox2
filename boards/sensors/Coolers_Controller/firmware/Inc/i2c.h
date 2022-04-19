#ifndef _I2C_H
#define _I2C_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define I2C_ADDR	0x43

#define I2C_DIR_RCV				I2C_DIRECTION_RECEIVE
#define	I2C_DIR_TRANSMIT	I2C_DIRECTION_TRANSMIT
#define I2C_DIR_UNKNOWN		( I2C_DIRECTION_TRANSMIT + 1 )

void I2C1_Init(void);

void I2C1_EV_ISR (void);
void I2C1_ER_ISR (void);

extern I2C_HandleTypeDef hi2c1;
extern volatile uint8_t I2C_transferRequested;
extern volatile uint16_t I2C_transferDir;

#endif	//_I2C_H
