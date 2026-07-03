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

#define TEST_MANUELL                    0

//霍尔方式
#define  HALL

//无霍尔方式
//#define  HALLLESS

//1:开环；2：速度环；3速度环、电流环；
#define  LOOP                           2 

//PWM频率，单位KHZ;
#define  PWM_FREQ                      ((u16) 18) 

//配置目标转矩，增大可提高力矩 
#define  Torque                         0x00FF

//Hallless 滤波长度
#define  FILTER_LONG                    0xFFFF 

//速度PID
#define  Speed_Kp                       0.05 
#define  Speed_Ki                       0.01 
#define  Speed_Kc                       0.01 

//电流PID
#define  Current_Kp                     2 
#define  Current_Ki                     0.2
#define  Current_Kc                     0.01 

//H_PWM_L_PWM驱动模式
//#define  H_PWM_L_PWM

//H_PWM_L_ON驱动模式
#define  H_PWM_L_ON

//硬件测试模式：1=只跑Test_PWM.c的裸PWM测试（PA8/9/10固定20%占空比），不进入任何电机控制逻辑
//确认高边PWM能正常输出后，改回0，恢复正常运行
#define  HARDWARE_PWM_TEST   0

#ifndef PWM_PERIOD_TICKS
#define PWM_PERIOD_TICKS   (TIM1->ARR + 1)
#endif

#ifndef DUTY_MAX_TICKS
// 启动阶段放宽到 85%，后续可恢复到 70%
#define DUTY_MAX_TICKS     ((PWM_PERIOD_TICKS * 85) / 100)
#endif


#endif
