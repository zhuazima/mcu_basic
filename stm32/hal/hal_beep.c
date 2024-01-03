#include "hal_beep.h"
#include "stm32f10x.h"
#include "hal_time.h"


#define BEEP_110_STEP_FREQ 58

#define BP_FRQ1	260
#define BP_FRQ2	BP_FRQ1+49
#define BP_FRQ3	BP_FRQ2+49
#define BP_FRQ4	BP_FRQ3+49
#define BP_FRQ5	BP_FRQ4+49
#define BP_FRQ6	BP_FRQ5+49
#define BP_FRQ7	BP_FRQ6+49
#define BP_FRQ8	BP_FRQ7+49
#define BP_FRQ9	BP_FRQ8+49
#define BP_FRQ10	BP_FRQ9+49
#define BP_FRQ11	BP_FRQ10+49
#define BP_FRQ12	BP_FRQ11+49
#define BP_FRQ13	BP_FRQ12+49
#define BP_FRQ14	BP_FRQ13+49
#define BP_FRQ15	BP_FRQ14+49

unsigned short NoteFreqAry[30] = {
		
		BP_FRQ1,BP_FRQ2,BP_FRQ3,BP_FRQ4,BP_FRQ5,BP_FRQ6,BP_FRQ7,BP_FRQ8,BP_FRQ9,BP_FRQ10,BP_FRQ11,BP_FRQ12,BP_FRQ13,BP_FRQ14,BP_FRQ15,
		BP_FRQ15,BP_FRQ14,BP_FRQ13,BP_FRQ12,BP_FRQ11,BP_FRQ10,BP_FRQ9,BP_FRQ8,BP_FRQ7,BP_FRQ6,BP_FRQ5,BP_FRQ4,BP_FRQ3,BP_FRQ2
};


void hal_BeepPwmCtrl(unsigned char cmd);
static void hal_BeepPwmHandle(void);



static void hal_BeepConfig(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOA , ENABLE); 	
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	
	
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);	//使能定时器3时钟
 //	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB  | RCC_APB2Periph_AFIO, ENABLE);  //使能GPIO外设和AFIO复用功能模块时钟
	
	GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE); //Timer3部分重映射  TIM3_CH2->PB4    
 
   //设置该引脚为复用输出功能,输出TIM3 CH1的PWM脉冲波形	PB4	
	GPIO_InitStructure.GPIO_Pin = BEEP_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; ; 
	GPIO_Init(BEEP_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = BEEP_EN_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; ; 
	GPIO_Init(BEEP_EN_PORT, &GPIO_InitStructure);
	GPIO_SetBits(BEEP_EN_PORT,BEEP_EN_PIN);
	
 
   //初始化TIM3
	TIM_TimeBaseStructure.TIM_Period = 100 ; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler =SystemCoreClock/1000000 - 1; //设置用来作为TIMx时钟频率除数的预分频值 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
	
	//初始化TIM3 Channel1 PWM模式	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //选择定时器模式:TIM脉冲宽度调制模式2
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性:TIM输出比较极性高
	
	//输出比较通道1
	TIM_OCInitStructure.TIM_Pulse = 50;     //占空比
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM3 OC2
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIM3在CCR2上的预装载寄存器

	TIM_Cmd(TIM3, ENABLE);  //使能TIM3
 
}

void hal_BeepInit(void)
{
	hal_BeepConfig();
	hal_CreatTimer(T_BEEP,hal_BeepPwmHandle,120,T_STA_START);
	hal_BeepPwmCtrl(0);
}


void hal_BeepPwmCtrl(unsigned char cmd)
{
 
	if(cmd)
	{
		GPIO_ResetBits(BEEP_EN_PORT,BEEP_EN_PIN);
	}else
	{
		GPIO_SetBits(BEEP_EN_PORT,BEEP_EN_PIN);
	}

}

static void hal_BeepPwmHandle(void)
{
	static unsigned char i=0;
	//hal_SetBeepFreq(NoteFreqAry[i]);
	TIM_SetAutoreload(TIM3,NoteFreqAry[i]);     //设置 ARR ， 改变脉冲频率
	TIM_SetCompare1(TIM3,NoteFreqAry[i]/2);         //设置 CCR1 ， 设置比较值， 这里 / 2 ，是占空比 50%
	TIM_SetCounter(TIM3,0);

	i++;
	if(i>28)
	{
		i=0;
	}
	
	hal_ResetTimer(T_BEEP,T_STA_START);     //重新开启定时矩阵
	 
}


