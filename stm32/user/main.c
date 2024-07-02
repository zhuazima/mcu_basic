

#include "hal_led.h"
#include "hal_key.h"
#include "hal_cpu.h"
#include "hal_time.h"
#include "OS_System.h"
#include "hal_Uart.h"
#include "hal_rfd.h"
#include "app.h"


#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_examples.h"



int main(void)
{

	// hal_CPUInit();
	// OS_TaskInit();
	// hal_TimeInit();
	
	// hal_LedInit();
	// OS_CreatTask(OS_TASK1,hal_LedProc,1,OS_RUN);

	// hal_KeyInit();
	// OS_CreatTask(OS_TASK2,hal_KeyProc,1,OS_RUN);

	// AppInit();

	// OS_Start();



    //lvgl test
	// lv_init();          	//lv 系统初始化
    // lv_port_disp_init();    //lvgl 显示接口初始化，放在lv_init后面
	// lv_port_indev_init();   //lvgl 输入接口初始化，放在 lv_init后面
	// lv_port_disp_init();

	// lv_example_btn_1();
	
	
	
}

