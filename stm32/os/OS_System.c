#include "OS_System.h"

volatile OS_TaskTypeDef OS_Task[OS_TASK_SUM];

CPUInterrupt_CallBack_t CPUInterrupptCtrlCBS;


/********************************************************************************************************
*  @函数名   OS_CPUInterruptCBSRegister						                                                           
*  @描述     注册CPU中断控制函数								                                     
*  @参数     pCPUInterruptCtrlCBS-CPU中断控制回调函数地址
*  @返回值   无
*  @注意     无
********************************************************************************************************/
void OS_CPUInterruptCBSRegister(CPUInterrupt_CallBack_t pCPUInterruptCtrlCBS)
{
	if(CPUInterrupptCtrlCBS == 0)
	{
		CPUInterrupptCtrlCBS = pCPUInterruptCtrlCBS;
	}
}

/********************************************************************************************************
*  @函数名   OS_TaskInit					                                                           
*  @描述     系统任务初始化							                                     
*  @参数     无
*  @返回值   无
*  @注意     无
********************************************************************************************************/
void OS_TaskInit(void)
{
	unsigned char i;
	for(i=0; i<OS_TASK_SUM; i++)
	{
		OS_Task[i].task = 0;
		OS_Task[i].RunFlag = OS_SLEEP;
		OS_Task[i].RunPeriod = 0;
		OS_Task[i].RunTimer = 0;
	}
}


/*******************************************************************************
* Function Name  : void OS_CreatTask(unsigned char ID, void (*proc)(void), OS_TIME_TYPEDEF TimeDly, bool flag)
* Description    : 创建任务 
* Input          : - ID：任务ID
*					- (*proc)() 用户函数入口地址 
*					- TimeDly 任务执行频率，单位ms
* 					- flag 任务就绪状态  OS_SLEEP-休眠 OS_RUN-运行 
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void OS_CreatTask(unsigned char ID, void (*proc)(void), unsigned short Period, OS_TaskStatusTypeDef flag)
{	
	if(!OS_Task[ID].task)
	{
		OS_Task[ID].task = proc;
		OS_Task[ID].RunFlag = OS_SLEEP;
		OS_Task[ID].RunPeriod = Period;
		OS_Task[ID].RunTimer = 0;
	}
}


/********************************************************************************************************
*  @函数名   OS_ClockInterruptHandle						                                                           
*  @描述     系统任务调度函数								                                     
*  @参数     无
*  @返回值   无   
*  @注意     为了保证任务实时性，这个必须放在10ms的定时器或系统时钟中断函数里
********************************************************************************************************/
void OS_ClockInterruptHandle(void)
{
	unsigned char i;
	for(i=0; i<OS_TASK_SUM; i++)	//这个循环是对所有的任务执行一次以下操作。
	{
		if(OS_Task[i].task)	//通过task函数指针指向不等于0来判断任务是否被创建
		{					
			OS_Task[i].RunTimer++;
			if(OS_Task[i].RunTimer >= OS_Task[i].RunPeriod)	//判断计时器值是否到达任务需要执行的时间
			{
				OS_Task[i].RunTimer = 0;
				OS_Task[i].RunFlag = OS_RUN;//把任务的状态设置成执行，任务调度函数会一直判断这个变量的值，如果是OS_RUN就会执行task指向的函数。
			}
			
		}
	}
	
}

/*******************************************************************************
* Function Name  : void OS_Start(void)
* Description    : 开始任务 
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void OS_Start(void)
{
	unsigned char i;
	while(1)
	{
		for(i=0; i<OS_TASK_SUM; i++)
		{
			if(OS_Task[i].RunFlag == OS_RUN)
			{
				OS_Task[i].RunFlag = OS_SLEEP;
		 
				(*(OS_Task[i].task))();	
			}
		}	
	}
}

/*******************************************************************************
* Function Name  : void OS_TaskGetUp(OS_TaskIDTypeDef taskID)
* Description    : 唤醒一个任务
* Input          : - taskID：需要被唤醒任务的ID
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void OS_TaskGetUp(OS_TaskIDTypeDef taskID)
{	
	unsigned char IptStatus;
	if(CPUInterrupptCtrlCBS != 0)
	{
		CPUInterrupptCtrlCBS(CPU_ENTER_CRITICAL,&IptStatus);
	}
	OS_Task[taskID].RunFlag = OS_RUN;	
	if(CPUInterrupptCtrlCBS != 0)
	{
		CPUInterrupptCtrlCBS(CPU_EXIT_CRITICAL,&IptStatus);
	}
}

/*******************************************************************************
* Function Name  : void OS_TaskSleep(OS_TaskIDTypeDef taskID)
* Description    : 挂起一个任务，让一个任务进入睡眠状态，该函数暂时没用到
* Input          : - taskID：需要被挂起任务的ID
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void OS_TaskSleep(OS_TaskIDTypeDef taskID)
{
	unsigned char IptStatus;
	if(CPUInterrupptCtrlCBS != 0)
	{
		CPUInterrupptCtrlCBS(CPU_ENTER_CRITICAL,&IptStatus);
	}
	OS_Task[taskID].RunFlag = OS_SLEEP;
	if(CPUInterrupptCtrlCBS != 0)
	{
		CPUInterrupptCtrlCBS(CPU_EXIT_CRITICAL,&IptStatus);
	}
}

/********************************************************************************************************
*  @函数名   S_QueueEmpty						                                                           
*  @描述     清空一个队列								                                     
*  @参数     Head-队列头地址,  Tail-队列尾地址,   HBuff-队列缓存
*  @返回值   无   
*  @注意    无
********************************************************************************************************/
void S_QueueEmpty(unsigned char **Head, unsigned char **Tail, unsigned char *HBuff)
{
		*Head = HBuff;
		*Tail = HBuff;
}

/********************************************************************************************************
*  @函数名   S_QueueDataIn						                                                           
*  @描述     输入一个字节数据进队列								                                     
*  @参数     Head-队列头地址,  Tail-队列尾地址,   HBuff-队列缓存
*  @返回值   无   
*  @注意     无
********************************************************************************************************/
void S_QueueDataIn(unsigned char **Head, unsigned char **Tail, unsigned char *HBuff, unsigned short Len, unsigned char *HData, unsigned short DataLen)
{	
	unsigned short num;
	unsigned char IptStatus;
	
	if(CPUInterrupptCtrlCBS != 0)
	{
		CPUInterrupptCtrlCBS(CPU_ENTER_CRITICAL,&IptStatus);
	}
	for(num = 0; num < DataLen; num++, HData++)
	{
			**Tail = *HData;
			(*Tail)++;
			if(*Tail == HBuff+Len)
				*Tail = HBuff;
			if(*Tail == *Head)
			{
					if(++(*Head) == HBuff+Len)
						*Head = HBuff;		
			}
	}	
	if(CPUInterrupptCtrlCBS != 0)
	{
		CPUInterrupptCtrlCBS(CPU_EXIT_CRITICAL,&IptStatus);
	}
}

/********************************************************************************************************
*  @函数名   S_QueueDataOut						                                                           
*  @描述     从队列里取出一个数据								                                     
*  @参数     Head-队列头地址,  Tail-队列尾地址,   HBuff-队列缓存
*  @返回值   取出的数据   
*  @注意     无
********************************************************************************************************/
unsigned char S_QueueDataOut(unsigned char **Head, unsigned char **Tail, unsigned char *HBuff, unsigned short Len, unsigned char *Data)
{					   
	unsigned char back = 0;
	unsigned char IptStatus;
	if(CPUInterrupptCtrlCBS != 0)
	{
		CPUInterrupptCtrlCBS(CPU_ENTER_CRITICAL,&IptStatus);
	}
	*Data = 0;
	if(*Tail != *Head)
	{
			*Data = **Head;
			back = 1; 				
			if(++(*Head) == HBuff+Len)
				*Head = HBuff;
	}
	if(CPUInterrupptCtrlCBS != 0)
	{
		CPUInterrupptCtrlCBS(CPU_EXIT_CRITICAL,&IptStatus);
	}
	return back;	
}

/********************************************************************************************************
*  @函数名   S_QueueDataLen						                                                           
*  @描述     判断队列里数据的长度							                                     
*  @参数     Head-队列头地址,  Tail-队列尾地址,   HBuff-队列缓存
*  @返回值   队列里有数据个数
*  @注意     无
********************************************************************************************************/
unsigned short S_QueueDataLen(unsigned char **Head, unsigned char **Tail, unsigned short Len)
{
		if(*Tail > *Head)
			return *Tail-*Head;
		if(*Tail < *Head)
			return *Tail+Len-*Head;
		return 0;
}

