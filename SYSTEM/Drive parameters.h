//############################################################
// Created on: 2019年5月18日
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//版权所有，盗版必究
//STM32电机控制开发板
//匠心科技
//网址: https://shop298362997.taobao.com/
//电机控制答疑邮箱：JXKJ_2007@163.com
//############################################################
#ifndef __DRIVE_PARAMETERS_H
#define __DRIVE_PARAMETERS_H

//PWM频率，单位KHZ
#define  PWM_FREQ                      ((u16) 18)

//H_PWM_L_ON驱动模式：上桥用TIM1 PWM，下桥用普通GPIO常通/常断
#define  H_PWM_L_ON

#ifndef PWM_PERIOD_TICKS
#define PWM_PERIOD_TICKS   (TIM1->ARR + 1)
#endif

#ifndef DUTY_MAX_TICKS
#define DUTY_MAX_TICKS     ((PWM_PERIOD_TICKS * 85) / 100)
#endif

#endif
