#include "Test_PWM.h"
#include "Control stage parameters.h"
#include "Drive parameters.h"

/* Standalone PWM test:
 * - PA8/PA9/PA10 (TIM1 CH1/2/3, high side): fixed 20% PWM, all in phase.
 * - PB13/PB14/PB15 (low side, plain GPIO in this board's driver scheme): held LOW (off).
 * No commutation, no ADC, no interrupt. If PA8/9/10 still show nothing here,
 * the issue is not in the motor-control logic - it is in TIM1's output
 * enable path or the driver board's high-side input/gate circuit.
 */
void Test_PWM_Init(void)
{
	GPIO_InitTypeDef        GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef       TIM_OCInitStructure;
	TIM_BDTRInitTypeDef     TIM_BDTRInitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

	/* High side: TIM1 CH1/2/3 alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin   = PHASE_UH_GPIO_PIN | PHASE_VH_GPIO_PIN | PHASE_WH_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(PHASE_UH_GPIO_PORT, &GPIO_InitStructure);

	/* Low side: plain GPIO output, held low (bottom switches stay off) */
	GPIO_InitStructure.GPIO_Pin  = PHASE_UL_GPIO_PIN | PHASE_VL_GPIO_PIN | PHASE_WL_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(PHASE_UL_GPIO_PORT, &GPIO_InitStructure);
	GPIO_ResetBits(PHASE_UL_GPIO_PORT, PHASE_UL_GPIO_PIN | PHASE_VL_GPIO_PIN | PHASE_WL_GPIO_PIN);

	TIM_DeInit(TIM1);

	TIM_TimeBaseStructure.TIM_Prescaler         = 0;
	TIM_TimeBaseStructure.TIM_CounterMode       = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period            = 72000 / PWM_FREQ - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision     = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode       = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState  = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;   /* low side not driven by TIM here */
	TIM_OCInitStructure.TIM_Pulse        = ((72000 / PWM_FREQ) * 20) / 100;  /* fixed 20% duty */
	TIM_OCInitStructure.TIM_OCPolarity   = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCNPolarity  = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState  = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;

	TIM_OC1Init(TIM1, &TIM_OCInitStructure);
	TIM_OC2Init(TIM1, &TIM_OCInitStructure);
	TIM_OC3Init(TIM1, &TIM_OCInitStructure);

	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);

	TIM_BDTRInitStructure.TIM_OSSRState       = TIM_OSSRState_Enable;
	TIM_BDTRInitStructure.TIM_OSSIState       = TIM_OSSIState_Enable;
	TIM_BDTRInitStructure.TIM_LOCKLevel       = TIM_LOCKLevel_OFF;
	TIM_BDTRInitStructure.TIM_DeadTime        = 0x6A;
	TIM_BDTRInitStructure.TIM_Break           = TIM_Break_Disable;
	TIM_BDTRInitStructure.TIM_BreakPolarity   = TIM_BreakPolarity_Low;
	TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
	TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);

	TIM_Cmd(TIM1, ENABLE);
	TIM_CtrlPWMOutputs(TIM1, ENABLE);   /* MOE = 1, required for PA8/9/10 to actually output */
}
