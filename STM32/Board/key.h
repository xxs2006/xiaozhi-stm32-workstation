#ifndef __KEY_H
#define __KEY_H

#include "main.h"

/*
 * 4 push buttons on PB0/PB1/PB4/PB5 (external 10k pull-ups, active-low).
 * Buzzer on PA11 (active buzzer, high = beep).
 *
 * SW1 (PB0): toggle work light = RELAY1 + LED strip (cold 200 / warm 500)
 * SW2 (PB1): toggle fan (RELAY2) + window motor together
 * SW3 (PB4): RGB strip mode cycle: rainbow-breath -> rainbow-flow -> white solid -> off
 * SW4 (PB5): buzzer manual alarm toggle + auto alarm when temp > 30 C
 */

void Key_Init(void);       /* configure key GPIOs + buzzer GPIO */
void Key_Task_Start(void); /* create the key scan task (20 ms tick) */

#endif /* __KEY_H */
