#ifndef __ULN2003AN_H
#define __ULN2003AN_H
#include "stm32f1xx_hal.h"


void win_open(uint32_t speed);
void win_close(uint32_t speed);
void Win_Motor_GPIO_Init(void);

#endif
