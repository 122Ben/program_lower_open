#ifndef __OPENLOOP_H
#define __OPENLOOP_H

#include "stm32f10x.h"

void OpenLoop_KeyInit(void);
void OpenLoop_KeyScan(void);
void OpenLoop_PWM_Init(void);
void OpenLoop_ADC_Init(void);
void OpenLoop_DebugTask(void);

#endif
