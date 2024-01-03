#include "stm32f10x.h"
#include "os_system.h"
#include "hal_time.h"
#include "hal_led.h"


Queue4 	LedCmdBuff[LED_SUM];		//LED信箱

static void hal_Led1Drive(unsigned char sta);
static void hal_BuzDrive(unsigned char sta);


void (*hal_LedDrive[LED_SUM])(unsigned char) = {	hal_Led1Drive,
																hal_BuzDrive,
														//		hal_Led3Drive,
														//		hal_Led4Drive,
										};

									
unsigned short Led_Dark[] = {0,10,LED_EFFECT_END};
unsigned short Led_Light[] = {1,10,LED_EFFECT_END};
unsigned short Led_Light100ms[] = {1,10,0,10,LED_EFFECT_END};
unsigned short Led_Blink1[] = {1,10,0,10,LED_EFFECT_AGN,2};
unsigned short Led_Blink2[] = {1,10,0,10,1,10,0,10,1,10,0,200,LED_EFFECT_AGN,6};
unsigned short Led_Blink3[] = {1,30,0,30,LED_EFFECT_AGN,2};
unsigned short Led_Blink4[] = {1,50,0,50,LED_EFFECT_AGN,2};


unsigned short LedTimer[LED_SUM];
unsigned short *pLed[LED_SUM];


unsigned char LedLoadFlag[LED_SUM];



static void hal_ledConfig(void);
static void hal_LedHandle(void);


void hal_LedInit(void)
{
	unsigned char i;
	hal_ledConfig();
	hal_CreatTimer(T_LED,hal_LedHandle,200,T_STA_START);
	
	for(i=0; i<LED_SUM; i++)
	{
		LedLoadFlag[i] = 0;
		pLed[i] = (unsigned short *)Led_Dark;
		LedTimer[i] = *(pLed[i]+1);
		QueueEmpty(LedCmdBuff[i]);
	}
	

	LedMsgInput(LED1,LED_DARK,1);
	LedMsgInput(BUZ,LED_DARK,1);
}

void hal_LedProc(void)
{
	unsigned char i;
	unsigned char cmd;
	for(i=0; i<LED_SUM; i++)
	{
		
		if((QueueDataLen(LedCmdBuff[i])>0) && (LedLoadFlag[i]==0))
		{
				QueueDataOut(LedCmdBuff[i], &cmd);
				LedLoadFlag[i] = 1;
				switch(cmd)
				{
					case LED_DARK:
						pLed[i] = (unsigned short *)Led_Dark;	//时间指针指向特效数组
						LedTimer[i] = *(pLed[i]+1);				//取出当前特效数组的下一位，也就特效时间
					break;
					
					case LED_LIGHT:
						pLed[i] = (unsigned short *)Led_Light;
						LedTimer[i] = *(pLed[i]+1);
					break;
					
					case LED_LIGHT_100MS:
						pLed[i] = (unsigned short *)Led_Light100ms;
						LedTimer[i] = *(pLed[i]+1);
					break;
					
					case LED_BLINK1:
						pLed[i] = (unsigned short *)Led_Blink1;
						LedTimer[i] = *(pLed[i]+1);
					break;
					
					case LED_BLINK2:
						pLed[i] = (unsigned short *)Led_Blink2;
						LedTimer[i] = *(pLed[i]+1);
					break;
					
					case LED_BLINK3:
						pLed[i] = (unsigned short *)Led_Blink3;
						LedTimer[i] = *(pLed[i]+1);
					break;
					
					case LED_BLINK4:
						pLed[i] = (unsigned short *)Led_Blink4;
						LedTimer[i] = *(pLed[i]+1);
					break;

					default :
					break;
					
					 
				}
		}
	}
}


void LedMsgInput(unsigned char type,LED_EFFECT_TEPEDEF cmd,unsigned char clr)
{	 
	unsigned char bLedCMD;
	if(type >= LED_SUM)
	{
		return;
	}
	bLedCMD = cmd;
	if(clr)			//这个特效是否要立即执行
	{
		QueueEmpty(LedCmdBuff[type]);
		LedLoadFlag[type] = 0;
		 
	}
	QueueDataIn(LedCmdBuff[type],&bLedCMD,1);	//特效入列
	
}


static void hal_ledConfig(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA , ENABLE); 						 
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB , ENABLE); 	


	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; ; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,GPIO_Pin_1);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; ; 
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB,GPIO_Pin_0);	
	
}


static void hal_LedHandle(void)
{
	unsigned char i;

	for(i=0; i<LED_SUM; i++)
	{
		if(LedTimer[i])
		{
			LedTimer[i]--;
		}
		if(!LedTimer[i])
		{
			//定时时间到
			if(*(pLed[i]+2) == LED_EFFECT_END)
			{
					LedLoadFlag[i] = 0;
			}else
			{
				pLed[i] += 2;
				if(*pLed[i] == LED_EFFECT_AGN)
				{
					pLed[i] = pLed[i] - (*(pLed[i]+1) * 2);
					//unsigned short Led_Blink3[] = {1,20,0,20,1,20,0,20,LED_EFFECT_AGN};
				}
				LedTimer[i] = *(pLed[i]+1);
			}
		
				
		}
		hal_LedDrive[i](*pLed[i]);
	}
 
 
	hal_ResetTimer(T_LED,T_STA_START);
 
}

static void hal_Led1Drive(unsigned char sta)
{
	if(sta)
	{
		GPIO_SetBits(GPIOA,GPIO_Pin_1);
	}else
	{
		GPIO_ResetBits(GPIOA,GPIO_Pin_1);
	}
}


void hal_LedTurn(void)
{
	GPIO_WriteBit(GPIOA,GPIO_Pin_1,(BitAction)(1-GPIO_ReadOutputDataBit(GPIOA,GPIO_Pin_1)));
}
 
static void hal_BuzDrive(unsigned char sta)
{
	if(sta)
	{
		GPIO_SetBits(BUZ_PORT,BUZ_PIN);
	}else
	{
		GPIO_ResetBits(BUZ_PORT,BUZ_PIN);
	}
}

 
