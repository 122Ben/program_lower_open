//############################################################
// Created on: 2019年5月18日
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//版权所有，盗版必究
//STM32电机控制开发板
//匠心科技
//网址: https://shop298362997.taobao.com/
//电机控制答疑邮箱：JXKJ_2007@163.com
//############################################################
#ifndef _Tim1_PWM_H
#define _Tim1_PWM_H 
#include "stm32f10x.h"

void Tim1_PWM_Init(void);
void TIM1_Gpio(void);
void Start_Motor(void);
void Stop_Motor(void);
void MOS_Q15PWM(void) ;    // 切换Q15MOS驱动
void MOS_Q16PWM(void) ;    // 切换Q16MOS驱动
void MOS_Q26PWM(void) ;    // 切换Q26MOS驱动
void MOS_Q24PWM(void) ;    // 切换Q24MOS驱动
void MOS_Q34PWM(void) ;    // 切换Q34MOS驱动
void MOS_Q35PWM(void) ;    // 切换Q35MOS驱动

#endif
