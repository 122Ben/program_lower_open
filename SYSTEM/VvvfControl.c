//############################################################
// Created on: 2019年5月18日
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//版权所有，盗版必究
//STM32电机控制开发板
//匠心科技
//网址: https://shop298362997.taobao.com/
//电机控制答疑邮箱：JXKJ_2007@163.com
//############################################################
#include "VvvfControl.h"
#include "State_Machine.h"
#include "Tim1_PWM.h"
#include "Hallless.h"
#include "Drive parameters.h"
#ifdef HALLLESS
extern    Hallless        Hallless_Three;
extern    State           StateContr;
extern    VvvF_start      VvvF_startPare;

void Delay_US(u32 nCount_temp)
{
	u32 nCount=nCount_temp*8;
	while(nCount--);
}

void Delay_MS(u32 nCount_temp)
{
	u32 nCount=nCount_temp*8000;
	while(nCount--);
}

void Anwerfen(void) 
{
	VvvF_startPare.Voilage_Ref =300;//1500
  StateContr.Duty = VvvF_startPare.Voilage_Ref;
	MOS_Q24PWM(); //固定一相
	Delay_MS(5);
	VvvF_startPare.Freq_T_Ref = 300;
	while(1)
	{
		for(VvvF_startPare.Freq_T_Ref_Count=0;VvvF_startPare.Freq_T_Ref_Count<VvvF_startPare.Freq_T_Ref; VvvF_startPare.Freq_T_Ref_Count++) 
		{
			Delay_US(800);  	//1800
		} 
		VvvF_startPare.Freq_T_Ref-= VvvF_startPare.Freq_T_Ref/15+1;
		VvvF_startPare.Count++;			
		Change_Voltage();
		if(VvvF_startPare.Freq_T_Ref < 25) 
		{ 		
			if(TEST_MANUELL)
			{
				VvvF_startPare.Freq_T_Ref = 25;   //开环强制换向
			}
			else
			{	
				break; 
			}				
		}	
		VvvF_startPare.VvvF_Count++;
		
		if(VvvF_startPare.VvvF_Count == 6)
		{
			VvvF_startPare.VvvF_Count = 0;
		}
		Anwerfen_SW();
	}
}

void Anwerfen_SW(void) 
{
if(StateContr.INVERSION == 0)
{
#ifdef H_PWM_L_PWM
	switch(VvvF_startPare.VvvF_Count)
	{
		case  0x0:  
		{
		MOS_Q24PWM();//B+A-
		}
		break;
		
		case  0x1:   
		{
		MOS_Q34PWM();//C+A-
		}
		break;

		case  0x2:
		{
		MOS_Q35PWM();//C+B-
		}
		break;
	
		case  0x3:     
		{
		MOS_Q15PWM();//A+B-
		}
		break;
		
		case  0x4:    
		{
		MOS_Q16PWM();//A+C-
		}
		break;
		
		case  0x5:    
		{
		MOS_Q26PWM();//B+C-
		}
		break;
		
		default:
		{
		Stop_Motor();
		Hallless_Three.Speed_RPM=0;
		}
		break;
	}
#endif

#ifdef H_PWM_L_ON
	switch(VvvF_startPare.VvvF_Count)
	{
		case  0x0:  
		{
		MOS_Q24PWM();//B+A-
		}
		break;
		
		case  0x1:   
		{
		MOS_Q34PWM();//C+A-
		}
		break;

		case  0x2:
		{
		MOS_Q35PWM();//C+B-
		}
		break;
	
		case  0x3:     
		{
		MOS_Q15PWM();//A+B-
		}
		break;
		
		case  0x4:    
		{
		MOS_Q16PWM();//A+C-
		}
		break;
		
		case  0x5:    
		{
		MOS_Q26PWM();//B+C-
		}
		break;
		
		default:
		{
		Stop_Motor();
		Hallless_Three.Speed_RPM=0;
		}
		break;
	}
#endif
}
if(StateContr.INVERSION == 1)
{
#ifdef H_PWM_L_PWM
	switch(VvvF_startPare.VvvF_Count)
	{
		case  0x5:  
		{
		MOS_Q24PWM();//B+A-
		}
		break;
		
		case  0x4:   
		{
		MOS_Q34PWM();//C+A-
		}
		break;

		case  0x3:
		{
		MOS_Q35PWM();//C+B-
		}
		break;
	
		case  0x2:     
		{
		MOS_Q15PWM();//A+B-
		}
		break;
		
		case  0x1:    
		{
		MOS_Q16PWM();//A+C-
		}
		break;
		
		case  0x0:    
		{
		MOS_Q26PWM();//B+C-
		}
		break;
		
		default:
		{
		Stop_Motor();
		Hallless_Three.Speed_RPM=0;
		}
		break;
	}
#endif

#ifdef H_PWM_L_ON
	switch(VvvF_startPare.VvvF_Count)
	{
		case  0x5:  
		{
		MOS_Q24PWM();//B+A-
		}
		break;
		
		case  0x4:   
		{
		MOS_Q34PWM();//C+A-
		}
		break;

		case  0x3:
		{
		MOS_Q35PWM();//C+B-
		}
		break;
	
		case  0x2:     
		{
		MOS_Q15PWM();//A+B-
		}
		break;
		
		case  0x1:    
		{
		MOS_Q16PWM();//A+C-
		}
		break;
		
		case  0x0:    
		{
		MOS_Q26PWM();//B+C-
		}
		break;
		
		default:
		{
		Stop_Motor();
		Hallless_Three.Speed_RPM=0;
		}
		break;
	}
#endif
}
}

void Change_Voltage(void)
{
		if(VvvF_startPare.Count == 1)
		{
			StateContr.Duty = 100;
		}
		if(VvvF_startPare.Count == 2)
		{
			StateContr.Duty = 100;
		}
		if(VvvF_startPare.Count == 3)
		{
			StateContr.Duty = 100;
		}	
		if(VvvF_startPare.Count == 4)
		{
			StateContr.Duty = 100;
		}
		if(VvvF_startPare.Count == 5)
		{
			StateContr.Duty = 200;
		}
		if(VvvF_startPare.Count == 6)
		{
			StateContr.Duty = 220;
		}
		if(VvvF_startPare.Count == 7)
		{
			StateContr.Duty = 220;
		}
		if(VvvF_startPare.Count == 8)
		{
			StateContr.Duty = 250;
		}
		if(VvvF_startPare.Count == 9)
		{
			StateContr.Duty = 250;
		}
		if(VvvF_startPare.Count == 10)
		{
			StateContr.Duty = 250;
		}
		if(VvvF_startPare.Count == 11)
		{
			StateContr.Duty = 300;
		}
		if(VvvF_startPare.Count == 12)
		{
			StateContr.Duty = 500;
		}
		if(VvvF_startPare.Count == 13)
		{
			StateContr.Duty = 500;
		}
		if(VvvF_startPare.Count == 14)
		{
			StateContr.Duty = 500;
		}
		if(VvvF_startPare.Count == 15)
		{
			StateContr.Duty = 500;
		}
}
#endif
