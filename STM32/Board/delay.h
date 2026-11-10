#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f1xx_hal.h"

void TIM4_Delay_Init(void);
void Delay_us(uint16_t us);
void Delay_ms(uint32_t ms);

#endif /* __DELAY_H */   
