

#include "hal_led.h"
#include "hal_key.h"
#include "hal_cpu.h"
#include "hal_time.h"
#include "OS_System.h"
#include "hal_Uart.h"
#include "hal_rfd.h"
#include "app.h"


int main(void)
{

	hal_CPUInit();
	OS_TaskInit();
	hal_TimeInit();
	
	hal_LedInit();
	OS_CreatTask(OS_TASK1,hal_LedProc,1,OS_RUN);

	hal_KeyInit();
	OS_CreatTask(OS_TASK2,hal_KeyProc,1,OS_RUN);
	
	hal_RFDInit();
	OS_CreatTask(OS_TASK3,hal_RFDProc,1,OS_RUN);

	hal_Uart_Init();
	OS_CreatTask(OS_TASK4,hal_Uart_Proc,1,OS_RUN);
	
	AppInit();
	OS_CreatTask(OS_TASK5,AppProc,1,OS_RUN);


	OS_Start();
	
	
	
}

