
#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

void TIM1_PWM_Init(void);
void TIM1_PWM_ConfigAndStart(uint32_t percent);
void TIM1_PWM_Start(void);
void TIM1_PWM_Stop(void);

void TIM2_Init(uint16_t period);
void TIM2_ISR(void);
void TIM2_Start(void);
void TIM2_Stop(void);
_Bool TIM2_Expired(void);

void TIM3_PWM_Init(void);
// channel: TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3
// percent: 0..100 [%]
void TIM3_PWM_ConfigAndStart(uint32_t channel, uint32_t percent);
void TIM3_PWM_Config(uint32_t channel, uint32_t percent);
void TIM3_PWM_Start(uint32_t channel);
void TIM3_PWM_Stop(uint32_t channel);

void TIM4_Init(void);
void TIM4_ISR(void);
_Bool TIM4_Expired(void);

#endif	//_TIMER_H
