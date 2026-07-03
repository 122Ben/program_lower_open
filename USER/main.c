#include "stm32f10x.h"
#include "OpenLoop.h"
#include "SEGGER_RTT.h"

/*
 * 无刷电机开环（强制六步换相）测试主程序
 * - 按 RUN 键开始输出，按 STOP 键停止
 * - UP/DOWN 调占空比，DIR 切正反转
 * - 全部调试信息通过 SEGGER RTT 打印，用 J-Link RTT Viewer 查看
 */
int main(void)
{
	SEGGER_RTT_printf(0, "\r\n==== motor_openloop start ====\r\n");

	OpenLoop_KeyInit();
	OpenLoop_PWM_Init();
	OpenLoop_ADC_Init();

	SEGGER_RTT_printf(0, "Ready. Press RUN to start.\r\n");

	while (1)
	{
		OpenLoop_KeyScan();
		OpenLoop_DebugTask();
	}
}
