#include "OpenLoop.h"
#include "SEGGER_RTT.h"
#include "Control stage parameters.h"
#include "Drive parameters.h"

/*
 * 完全开环六步换相测试程序
 * - 按 RUN：开始强制换相（不依赖霍尔/反电动势，纯定时换相）
 * - 按 STOP：停止输出
 * - 按 UP/DOWN：调节占空比
 * - 按 DIR：切换正反转方向
 * - RTT 打印：运行状态、占空比、当前换相步、方向、母线电压/电位器采样值
 */

#define OL_STEP_MS          20      // 每步换相间隔(ms)，决定强制转速
#define OL_DUTY_MIN_PCT     15
#define OL_DUTY_MAX_PCT     50
#define OL_DUTY_STEP_PCT    5
#define OL_DUTY_DEFAULT_PCT 25
#define OL_DEBOUNCE_MS      200
#define OL_PRINT_MS         200

typedef struct {
	volatile uint8_t  running;
	volatile uint8_t  dir;        // 0 = 正转, 1 = 反转
	volatile uint8_t  duty_pct;
	volatile uint8_t  step;       // 0..5
} OpenLoopState_t;

static OpenLoopState_t OL = {0, 0, OL_DUTY_DEFAULT_PCT, 0};

static volatile uint32_t g_ms_ticks = 0;

/* ================= 按键 ================= */

void OpenLoop_KeyInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;

	GPIO_InitStructure.GPIO_Pin = RUN_GPIO_PIN;
	GPIO_Init(RUN_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = STOP_GPIO_PIN;
	GPIO_Init(STOP_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = UP_GPIO_PIN;
	GPIO_Init(UP_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = DOWN_GPIO_PIN;
	GPIO_Init(DOWN_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = DIR_GPIO_PIN;
	GPIO_Init(DIR_GPIO_PORT, &GPIO_InitStructure);
}

static void Bridge_Enable(void)
{
	TIM_CCxCmd(TIM1, TIM_Channel_1, TIM_CCx_Enable);
	TIM_CCxCmd(TIM1, TIM_Channel_2, TIM_CCx_Enable);
	TIM_CCxCmd(TIM1, TIM_Channel_3, TIM_CCx_Enable);
}

static void Bridge_Disable(void)
{
	TIM_CCxCmd(TIM1, TIM_Channel_1, TIM_CCx_Disable);
	TIM_CCxCmd(TIM1, TIM_Channel_2, TIM_CCx_Disable);
	TIM_CCxCmd(TIM1, TIM_Channel_3, TIM_CCx_Disable);
	TIM1->CCR1 = 0;
	TIM1->CCR2 = 0;
	TIM1->CCR3 = 0;
	GPIO_ResetBits(GPIOB, PHASE_UL_GPIO_PIN | PHASE_VL_GPIO_PIN | PHASE_WL_GPIO_PIN);
}

void OpenLoop_KeyScan(void)
{
	static uint32_t last_action_ms = 0;

	if ((g_ms_ticks - last_action_ms) < OL_DEBOUNCE_MS)
	{
		return;
	}

	if (RUN_STATUS == 0)
	{
		OL.step = 0;
		Bridge_Enable();
		OL.running = 1;
		last_action_ms = g_ms_ticks;
		SEGGER_RTT_printf(0, "[KEY] RUN, duty=%d%%, dir=%d\r\n", OL.duty_pct, OL.dir);
	}
	else if (STOP_STATUS == 0)
	{
		OL.running = 0;
		Bridge_Disable();
		last_action_ms = g_ms_ticks;
		SEGGER_RTT_printf(0, "[KEY] STOP\r\n");
	}
	else if (UP_STATUS == 0)
	{
		if (OL.duty_pct + OL_DUTY_STEP_PCT <= OL_DUTY_MAX_PCT)
		{
			OL.duty_pct += OL_DUTY_STEP_PCT;
		}
		last_action_ms = g_ms_ticks;
		SEGGER_RTT_printf(0, "[KEY] UP, duty=%d%%\r\n", OL.duty_pct);
	}
	else if (DOWN_STATUS == 0)
	{
		if (OL.duty_pct - OL_DUTY_STEP_PCT >= OL_DUTY_MIN_PCT)
		{
			OL.duty_pct -= OL_DUTY_STEP_PCT;
		}
		last_action_ms = g_ms_ticks;
		SEGGER_RTT_printf(0, "[KEY] DOWN, duty=%d%%\r\n", OL.duty_pct);
	}
	else if (DIR_STATUS == 0)
	{
		OL.dir = !OL.dir;
		last_action_ms = g_ms_ticks;
		SEGGER_RTT_printf(0, "[KEY] DIR, dir=%d\r\n", OL.dir);
	}
}

/* ================= PWM / 桥臂 ================= */

void OpenLoop_PWM_Init(void)
{
	GPIO_InitTypeDef        GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef       TIM_OCInitStructure;
	TIM_BDTRInitTypeDef     TIM_BDTRInitStructure;
	NVIC_InitTypeDef        NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

	/* 上桥：TIM1 CH1/2/3 复用推挽 */
	GPIO_InitStructure.GPIO_Pin   = PHASE_UH_GPIO_PIN | PHASE_VH_GPIO_PIN | PHASE_WH_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(PHASE_UH_GPIO_PORT, &GPIO_InitStructure);

	/* 下桥：普通GPIO推挽输出，开关型控制 */
	GPIO_InitStructure.GPIO_Pin  = PHASE_UL_GPIO_PIN | PHASE_VL_GPIO_PIN | PHASE_WL_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(PHASE_UL_GPIO_PORT, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, PHASE_UL_GPIO_PIN | PHASE_VL_GPIO_PIN | PHASE_WL_GPIO_PIN);

	TIM_DeInit(TIM1);

	TIM_TimeBaseStructure.TIM_Prescaler         = 0;
	TIM_TimeBaseStructure.TIM_CounterMode       = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period            = 72000 / PWM_FREQ - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision     = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode       = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState  = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
	TIM_OCInitStructure.TIM_Pulse        = 0;
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
	TIM_BDTRInitStructure.TIM_DeadTime        = 0x6A;             // 死区约1.8us
	TIM_BDTRInitStructure.TIM_Break           = TIM_Break_Disable;
	TIM_BDTRInitStructure.TIM_BreakPolarity   = TIM_BreakPolarity_Low;
	TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
	TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);

	TIM_ClearFlag(TIM1, TIM_FLAG_Update);
	TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);

	TIM_Cmd(TIM1, ENABLE);
	TIM_CtrlPWMOutputs(TIM1, ENABLE);   // MOE=1，PA8/9/10才有物理输出

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	Bridge_Disable();  // 上电先保持关断，等按RUN才输出
}

/* 六步换相表：step 0..5 依次为 B+C-, B+A-, C+A-, C+B-, A+B-, A+C- */
static void OpenLoop_ApplyStep(uint8_t step, uint16_t duty)
{
	uint16_t ccr_u = 0, ccr_v = 0, ccr_w = 0;
	uint16_t low_on = 0;

	switch (step)
	{
		case 0: ccr_v = duty; low_on = PHASE_WL_GPIO_PIN; break; // B+ C-
		case 1: ccr_v = duty; low_on = PHASE_UL_GPIO_PIN; break; // B+ A-
		case 2: ccr_w = duty; low_on = PHASE_UL_GPIO_PIN; break; // C+ A-
		case 3: ccr_w = duty; low_on = PHASE_VL_GPIO_PIN; break; // C+ B-
		case 4: ccr_u = duty; low_on = PHASE_VL_GPIO_PIN; break; // A+ B-
		default: ccr_u = duty; low_on = PHASE_WL_GPIO_PIN; break; // A+ C-
	}

	TIM1->CCR1 = ccr_u;
	TIM1->CCR2 = ccr_v;
	TIM1->CCR3 = ccr_w;

	GPIO_ResetBits(GPIOB, (PHASE_UL_GPIO_PIN | PHASE_VL_GPIO_PIN | PHASE_WL_GPIO_PIN) & ~low_on);
	GPIO_SetBits(GPIOB, low_on);
}

/* ================= ADC（母线电压 + 电位器，调试打印用） ================= */

void OpenLoop_ADC_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef  ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;

	GPIO_InitStructure.GPIO_Pin = BusVolt_GPIO_PIN;
	GPIO_Init(BusVolt_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = POT_GPIO_PIN;
	GPIO_Init(POT_GPIO_PORT, &GPIO_InitStructure);

	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode       = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel       = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
}

static uint16_t OpenLoop_ReadAdc(uint8_t channel)
{
	ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_55Cycles5);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
	return ADC_GetConversionValue(ADC1);
}

void OpenLoop_DebugTask(void)
{
	static uint32_t last_print_ms = 0;
	uint16_t bus_v, pot;

	if ((g_ms_ticks - last_print_ms) < OL_PRINT_MS)
	{
		return;
	}
	last_print_ms = g_ms_ticks;

	bus_v = OpenLoop_ReadAdc(BusVolt_CHANNEL);
	pot   = OpenLoop_ReadAdc(POT_CHANNEL);

	SEGGER_RTT_printf(0, "run=%d,dir=%d,duty=%d%%,step=%d,bus_adc=%d,pot_adc=%d,tick=%d\r\n",
		OL.running, OL.dir, OL.duty_pct, OL.step, bus_v, pot, (int)g_ms_ticks);
}

/* ================= 中断：定时强制换相 ================= */

void TIM1_UP_IRQHandler(void)
{
	static uint32_t sub_ms_acc  = 0;
	static uint32_t step_ms_acc = 0;
	uint16_t duty_ticks;

	TIM_ClearFlag(TIM1, TIM_FLAG_Update);

	/* PWM_FREQ 是 kHz，所以每 PWM_FREQ 次中断累计 1ms */
	if (++sub_ms_acc >= PWM_FREQ)
	{
		sub_ms_acc = 0;
		g_ms_ticks++;
		step_ms_acc++;
	}

	if (!OL.running)
	{
		return;
	}

	if (step_ms_acc >= OL_STEP_MS)
	{
		step_ms_acc = 0;
		if (OL.dir == 0)
		{
			OL.step = (OL.step + 1) % 6;
		}
		else
		{
			OL.step = (OL.step + 5) % 6;   // 等效于 -1 mod 6
		}
	}

	duty_ticks = (uint16_t)(((uint32_t)PWM_PERIOD_TICKS * OL.duty_pct) / 100);
	OpenLoop_ApplyStep(OL.step, duty_ticks);
}
