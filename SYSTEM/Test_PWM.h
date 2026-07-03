#ifndef __TEST_PWM_H
#define __TEST_PWM_H

#include "stm32f10x.h"

/* Minimal standalone PWM hardware test.
 * Only touches GPIO + TIM1, no ADC/Hall/interrupt/state machine involved.
 * Use this to isolate whether PA8/PA9/PA10 (TIM1 CH1/2/3, high side)
 * can physically output PWM on this board.
 */
void Test_PWM_Init(void);

#endif
