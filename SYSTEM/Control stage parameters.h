//############################################################
// Created on: 2019年5月18日
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//版权所有，盗版必究
//STM32电机控制开发板
//匠心科技
//网址: https://shop298362997.taobao.com/
//电机控制答疑邮箱：JXKJ_2007@163.com
//############################################################
#ifndef __CONTROL_STAGE_PARAMETERS_H
#define __CONTROL_STAGE_PARAMETERS_H

//母线电压采样（调试打印用）
#define BusVolt_CHANNEL                 ADC_Channel_11
#define BusVolt_GPIO_PORT   			      GPIOC
#define BusVolt_GPIO_PIN     		        GPIO_Pin_1

//电位器（可选，用于开环调速）
#define POT_CHANNEL                     ADC_Channel_8
#define POT_GPIO_PORT        		        GPIOB
#define POT_GPIO_PIN         		        GPIO_Pin_0

//三相桥 - 上桥（TIM1 CH1/2/3，PWM）
#define PHASE_UH_GPIO_PORT              GPIOA
#define PHASE_UH_GPIO_PIN               GPIO_Pin_8

#define PHASE_VH_GPIO_PORT              GPIOA
#define PHASE_VH_GPIO_PIN               GPIO_Pin_9

#define PHASE_WH_GPIO_PORT              GPIOA
#define PHASE_WH_GPIO_PIN               GPIO_Pin_10

//三相桥 - 下桥（普通GPIO，开关型）
#define PHASE_UL_GPIO_PORT              GPIOB
#define PHASE_UL_GPIO_PIN               GPIO_Pin_13

#define PHASE_VL_GPIO_PORT              GPIOB
#define PHASE_VL_GPIO_PIN               GPIO_Pin_14

#define PHASE_WL_GPIO_PORT              GPIOB
#define PHASE_WL_GPIO_PIN               GPIO_Pin_15

//按键
#define RUN_GPIO_PORT            				GPIOC
#define RUN_GPIO_PIN             				GPIO_Pin_5

#define STOP_GPIO_PORT            			GPIOB
#define STOP_GPIO_PIN             			GPIO_Pin_1

#define UP_GPIO_PORT            				GPIOB
#define UP_GPIO_PIN             				GPIO_Pin_10

#define DOWN_GPIO_PORT            			GPIOB
#define DOWN_GPIO_PIN           			  GPIO_Pin_11

#define DIR_GPIO_PORT           				GPIOB
#define DIR_GPIO_PIN             				GPIO_Pin_12

#define RUN_STATUS                      GPIO_ReadInputDataBit(RUN_GPIO_PORT,RUN_GPIO_PIN)
#define STOP_STATUS                     GPIO_ReadInputDataBit(STOP_GPIO_PORT,STOP_GPIO_PIN)
#define UP_STATUS                       GPIO_ReadInputDataBit(UP_GPIO_PORT,UP_GPIO_PIN)
#define DOWN_STATUS                     GPIO_ReadInputDataBit(DOWN_GPIO_PORT,DOWN_GPIO_PIN)
#define DIR_STATUS                      GPIO_ReadInputDataBit(DIR_GPIO_PORT,DIR_GPIO_PIN)

#endif
