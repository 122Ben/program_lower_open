
#include "SEGGER_RTT.h"
#include "Tim1_ISR_MCLoop.h"
#include "ADC_int.h"
#include "Hall.h"
#include "State_Machine.h"
#include "PI_Cale.h"
#include "VvvfControl.h"
#include "Hallless.h"
#include "Tim1_PWM.h"
#include "Drive parameters.h"
#ifdef HALL
extern    Hall          Hall_Three;
#endif
#ifdef HALLLESS
extern    Hallless      Hallless_Three;
#endif
extern    PIDREG_T      pi_spd;
extern    PIDREG_T      pi_ICurr;
extern    State         StateContr;
extern    ADCSamp       ADCSampPare;
extern    VvvF_start    VvvF_startPare;

//// ==== Soft start configs (torque-first, minimal) ====
//#define SS_ALIGN_DUTY        (PWM_PERIOD_TICKS*80/100)   // 40% 对准
//#define SS_ALIGN_TIME_MS     500                          // 80ms

//#define SS_KICK_DUTY         (PWM_PERIOD_TICKS*60/100)   // 50% 强踢
//#define SS_RAMP_TARGET_RPM   800                         // 到 800rpm 切闭环
//#define SS_RAMP_TIMEOUT_MS   3000                         // 最长开环时间
//#define SS_RAMP_STEP_MS      50                          // 每 50ms 评估加力
//#define SS_RAMP_STEP_TICKS   24                          // 每次加 24 ticks（适中）

//// 建议把 DUTY_MAX_TICKS 放宽到 85% 周期，启动更有力
//// #define DUTY_MAX_TICKS (PWM_PERIOD_TICKS*85/100)

//typedef enum { SS_ALIGN=0, SS_KICK, SS_RAMP, SS_DONE } ss_state_t;
//static ss_state_t ss = SS_ALIGN;
//static uint16_t ms_acc = 0, ss_ms = 0, step_ms = 0;
//static uint16_t ss_duty = 0;



void TIM1_UP_IRQHandler(void)
{
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);
	Offset_CurrentReading();
	if(StateContr.drive_car == 1)
	{	
#ifdef HALLLESS
	//开环起动
	if(StateContr.Start_order == 1)
	{
		Anwerfen();
		StateContr.Start_order = 2;	
	}
	if(StateContr.Start_order == 2)
	{	
		HALLLESS_ADC_Sample();
		Hallless_SW();

	//开环运行
	if(StateContr.Control_Mode == 1)
	{
		StateContr.Duty = StateContr.Aim_Duty;
	}
	//速度环
	if(StateContr.Control_Mode == 2)
	{
		StateContr.Speed_Count++;
		if(StateContr.Speed_Count > 2)
		{
			 pi_spd.Fdb = Hallless_Three.Speed_RPMF ;   
			 PID_CALC(pi_spd);
			 FirstOrder_LPF_Cacl(pi_spd.Out,pi_spd.OutF,0.08379f);
			 StateContr.Speed_Count = 0;
			 StateContr.Duty= pi_spd.OutF;
		}
	}
	//电流环+速度环
	if(StateContr.Control_Mode == 3)
	{
		StateContr.Speed_Count++;
		StateContr.Current_Count++;
		if(StateContr.Speed_Count > 2)
		{
			 pi_spd.Fdb = Hallless_Three.Speed_RPMF ;   
			 PID_CALC(pi_spd);
			 FirstOrder_LPF_Cacl(pi_spd.Out,pi_spd.OutF,0.08379f);
			 StateContr.Speed_Count = 0;
		}
		if(StateContr.Current_Count > 1)
		{
			 pi_ICurr.Ref = Torque;                                     //空载默认0.2A电流
			 pi_ICurr.Fdb = ADCSampPare.BUS_Curr;   
			 PID_CALC(pi_ICurr);
			 FirstOrder_LPF_Cacl(pi_ICurr.Out,pi_ICurr.OutF,0.08379f);
			 StateContr.Current_Count = 0;
		}
		if(pi_ICurr.OutF>pi_spd.OutF)	 
		{
			 StateContr.Duty= pi_spd.OutF;              
			 pi_ICurr.Ui  = pi_spd.Ui;
		} 
		else 
		{
			 StateContr.Duty= pi_ICurr.OutF;          
			 pi_spd.Ui  = pi_ICurr.Ui;
		}
	}
	}
	
#endif	
	
#ifdef HALL

/************************************************************
 * 【新启动策略：静态定位 -> 开环强拖 -> 闭环切换】
 *
 * 适用：带流体负载的磁悬浮离心泵
 * 原理：
 * 1. SS_ALIGN: 强行锁死在一个已知位置 (100ms)
 * 2. SS_OPEN_RAMP: 不管霍尔，强制按六步相序“盲转”3个电周期，
 * 利用电磁力拖着叶轮跑起来，积累惯性。
 * 3. SS_DONE: 带着初速度和积分预加载，平滑切入闭环。
 ************************************************************/

#define ALIGN_DUTY        (PWM_PERIOD_TICKS*30/100)  // 定位占空比 30%
#define RAMP_DUTY         (PWM_PERIOD_TICKS*35/100)  // 强拖占空比 35% (可以稍大一点克服流体阻力)
#define RAMP_STEP_MS      15                         // 强拖每步耗时 (15ms 约等于电转速 666 RPM)
#define RAMP_TARGET_CYCLE 3                          // 盲转 3 个电周期后切闭环

typedef enum { SS_ALIGN = 0, SS_OPEN_RAMP, SS_DONE } ss_state_t;
static ss_state_t ss          = SS_ALIGN;
static uint16_t   ss_ms       = 0;
static uint16_t   ms_acc      = 0;

static uint8_t    ramp_step   = 0;  // 记录开环强拖到哪一步了
static uint8_t    ramp_cycle  = 0;  // 记录盲转了几个周期

// ===== Debug print control =====
static uint16_t dbg_ms = 0;


// ===== (1) 采样 + 霍尔换相（正常调用，但在前两个状态会被强制覆盖） =====
HALL_ADC_Sample();
Hall_SW();

// ===== (2) 合成 1ms 定时 =====
if (++ms_acc >= 18) {
    ms_acc = 0;
    ss_ms++;
    dbg_ms++;
}

// ===== (3) 新型启动逻辑 =====
if (ss != SS_DONE)
{
    // === 阶段 1：静态定位 (强行吸住) ===
    if (ss == SS_ALIGN)
    {
        StateContr.Duty = ALIGN_DUTY;
        MOS_Q16PWM();               // 强制定位在 A+ C-

        if (ss_ms >= 100)           // 锁 100ms 足够让水中的叶轮稳住
        {
            ss = SS_OPEN_RAMP;
            ss_ms = 0;
            ramp_step = 0;          // 准备进入强拖的第一步
            ramp_cycle = 0;
        }
        return;  // 不进入速度环
    }

    // === 阶段 2：开环强拖（调试模式：只要 RUN 按下就持续强制换相，不再切换到闭环） ===
    if (ss == SS_OPEN_RAMP)
    {
        StateContr.Duty = RAMP_DUTY;

        // 接续上面的 A+ C- (Q16)，按六步正交顺时针方向强制换相
        switch(ramp_step) {
            case 0: MOS_Q26PWM(); break; // B+ C-
            case 1: MOS_Q24PWM(); break; // B+ A-
            case 2: MOS_Q34PWM(); break; // C+ A-
            case 3: MOS_Q35PWM(); break; // C+ B-
            case 4: MOS_Q15PWM(); break; // A+ B-
            case 5: MOS_Q16PWM(); break; // A+ C-
        }

        // 每隔 RAMP_STEP_MS 强制走下一步
        if (ss_ms >= RAMP_STEP_MS)
        {
            ss_ms = 0;
            ramp_step++;

            if (ramp_step >= 6)
            {
                ramp_step = 0;
                ramp_cycle++; // 完成一个电周期（仅计数，开环调试模式下不再切闭环）
            }
        }
        if (dbg_ms >= 50)   // 每 50ms 打印一次，方便确认开环输出是否正常
        {
            dbg_ms = 0;
            SEGGER_RTT_printf(0,
                "OPEN_LOOP duty=%d,step=%d,cycle=%d,hall=0x%02X,bus_v=%d,bus_i=%d\r\n",
                (int)StateContr.Duty,
                (int)ramp_step,
                (int)ramp_cycle,
                Hall_Three.Hall_State,
                (int)ADCSampPare.BUS_Voltage,
                (int)ADCSampPare.BUS_Curr
            );
        }

        return; // 开环调试模式：一直强制换相，不进入霍尔闭环
    }
}
// =====（4）多步对准完成 → 正常闭环控制 =====

//开环
if(StateContr.Control_Mode == 1)
{
	StateContr.Duty = StateContr.Aim_Duty;
}

//速度环
if(StateContr.Control_Mode == 2)
{
	StateContr.Speed_Count++;
	if(StateContr.Speed_Count > 2)
	{
		 pi_spd.Fdb = Hall_Three.Speed_RPMF ;   
		 PID_CALC(pi_spd);
		 FirstOrder_LPF_Cacl(pi_spd.Out,pi_spd.OutF,0.08379f);
		 StateContr.Speed_Count = 0;
		 StateContr.Duty= pi_spd.OutF;
	}
}

/*************** Debug print (20 Hz) ****************/
if (ss == SS_DONE)
{
    if (dbg_ms >= 50)   // 每 50ms 打印一次
    {
        dbg_ms = 0;

        SEGGER_RTT_printf(0,
            "rpm=%d,duty=%d,spd_ref=%d,spd_out=%d,bus_v=%d,bus_i=%d,hall=0x%02X\r\n",
            (int)Hall_Three.Speed_RPMF,
            (int)StateContr.Duty,
            (int)pi_spd.Ref,
            (int)pi_spd.OutF,
            (int)ADCSampPare.BUS_Voltage,
            (int)ADCSampPare.BUS_Curr,
            Hall_Three.Hall_State
        );
    }
}


//电流环+速度环
if(StateContr.Control_Mode == 3)
{
	StateContr.Speed_Count++;
	StateContr.Current_Count++;
	if(StateContr.Speed_Count > 2)
	{
		 pi_spd.Fdb = Hall_Three.Speed_RPMF ;   
		 PID_CALC(pi_spd);
		 FirstOrder_LPF_Cacl(pi_spd.Out,pi_spd.OutF,0.08379f);
		 StateContr.Speed_Count = 0;
	}
	if(StateContr.Current_Count > 1)
	{
		 pi_ICurr.Ref = Torque;    
		 pi_ICurr.Fdb = ADCSampPare.BUS_Curr ;   
		 PID_CALC(pi_ICurr);
		 FirstOrder_LPF_Cacl(pi_ICurr.Out,pi_ICurr.OutF,0.08379f);
		 StateContr.Current_Count = 0;
	}
	if(pi_ICurr.OutF > pi_spd.OutF)	 
	{
		 StateContr.Duty= pi_spd.OutF;              
		 pi_ICurr.Ui  = pi_spd.Ui;
	} 
	else 
	{
		 StateContr.Duty= pi_ICurr.OutF;          
		 pi_spd.Ui  = pi_ICurr.Ui;
	}
}

#endif

}
}
