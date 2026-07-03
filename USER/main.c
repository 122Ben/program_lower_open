
#include "stm32f10x.h"
#include "GPIO_int.h"
#include "Timer.h"
#include "ADC_int.h"
#include "OLED.h"
#include "Tim1_PWM.h"
#include "PI_Cale.h"
#include "State_Machine.h"
#include "PMSM motor parameters.h"
#include "Power stage parameters.h"
#include "Control stage parameters.h"
#include "Drive parameters.h"
#include "Test_PWM.h"

#ifdef HALL
#include "Hall.h"
#endif
#ifdef HALLLESS
#include "VvvfControl.h"
#include "Hallless.h"
#endif
PIDREG_T     pi_spd = PIDREG_T_DEFAULTS;
PIDREG_T     pi_ICurr = PIDREG_T_DEFAULTS;
State        StateContr=State_DEFAULTS;
ADCSamp      ADCSampPare=ADCSamp_DEFAULTS;
#ifdef HALLLESS
Hallless     Hallless_Three=Hallless_DEFAULTS;
VvvF_start   VvvF_startPare=VvvF_start_DEFAULTS;
#endif
#ifdef HALL
Hall         Hall_Three=Hall_DEFAULTS;
#endif
#ifdef HALLLESS
extern       Hallless   Hallless_Three;
#endif
volatile uint16_t ADC_DualConvertedValueTab[5];  //电位器接口函数
int main(void)
{
	Delay(10000);
#if HARDWARE_PWM_TEST
	Test_PWM_Init();
	while(1)
	{
	}
#endif
	SysTickConfig();
	StateContr.Control_Mode = LOOP;
	PID_init();
	if(	StateContr.Control_Mode == 1)
	{
	StateContr.Aim_Speed = 20;
	StateContr.Aim_Duty = 72000 / PWM_FREQ * StateContr.Aim_Speed / 100;
	}
	if(StateContr.Control_Mode == 2 || StateContr.Control_Mode == 3)
	{
		pi_spd.Ref = 800;
	}
	Delay(10000);	
	Key_Gpio();
#ifdef HALL
	ThreeHallPara_init();
	Hall_Gpio();
#endif
#ifdef HALLLESS
	ThreeHalllessPara_init();
	Hallless_Gpio(); 
#endif	
	Delay(10000);	
	Adc_Configuration();
	Delay(10000);	
	Tim1_PWM_Init();	
	Delay(10000);	
	OLED_Init();
	while(1)
	{
		OLED_Display();
		Key_Scanning();
	}
}

