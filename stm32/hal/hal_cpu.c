#include "stm32f10x.h"
#include "OS_System.h"
#include "hal_led.h"
#include "hal_cpu.h"

static void hal_CoreClockInit(void);
static void hal_CPU_Critical_Control(CPU_EA_TYPEDEF cmd,unsigned char *pSta);


void hal_CPUInit(void)
{
	hal_CoreClockInit();
	OS_CPUInterruptCBSRegister(hal_CPU_Critical_Control);
	 
}


static void hal_CoreClockInit(void)
{	
	SysTick_Config(SystemCoreClock / 100);			//ʹ��48M��Ϊϵͳʱ�ӣ���ô��������1����1/48M(ms), (1/48000000hz)*(48000000/100)=0.01S=10ms
}


/********************************************************************************************************
*  @������   hal_getprimask						                                                           
*  @����     ��ȡCPU���ж�״̬							                                     
*  @����     ��
*  @����ֵ   0-���жϹر� 1-���жϴ�
*  @ע��     ��
********************************************************************************************************/
static unsigned char hal_getprimask(void)
{
	return (!__get_PRIMASK());		//0���жϴ򿪣�1���жϹرգ�����Ҫȡ��
}


/********************************************************************************************************
*  @������   hal_CPU_Critical_Control						                                                           
*  @����     CPU�ٽ紦�����						                                     
*  @����     cmd-��������  *pSta-���ж�״̬
*  @����ֵ   ��
*  @ע��     ��
********************************************************************************************************/
static void hal_CPU_Critical_Control(CPU_EA_TYPEDEF cmd,unsigned char *pSta)
{
	if(cmd == CPU_ENTER_CRITICAL)
	{
		*pSta = hal_getprimask();	//�����ж�״̬
		__disable_irq();		//��CPU���ж�
	}else if(cmd == CPU_EXIT_CRITICAL)
	{
		if(*pSta)
		{
			__enable_irq();		//���ж�
		}else 
		{
			__disable_irq();	//�ر��ж�
		}
	}
}


void SysTick_Handler(void)
{
	OS_ClockInterruptHandle();
	 //��ʱ�жϴ���
}
