#ifndef _HAL_KEY_H
#define _HAL_KEY_H

//上键
#define K1_PORT	GPIOB
#define K1_PIN	GPIO_Pin_3


//下键
#define K2_PORT	GPIOB
#define K2_PIN	GPIO_Pin_5

//左键
#define K3_PORT	GPIOB
#define K3_PIN	GPIO_Pin_6

//右键
#define K4_PORT	GPIOB
#define K4_PIN	GPIO_Pin_7
 

//取消/返回
#define K5_PORT GPIOB	
#define K5_PIN	GPIO_Pin_10

//确定/设置
#define K6_PORT GPIOB
#define K6_PIN	GPIO_Pin_11

typedef enum
{
	KEY_S1,		//上键
	KEY_S2,		//下键
	KEY_S3,		//左
	KEY_S4,		//右
	KEY_S5,		//取消
	KEY_S6,		//确定/菜单
	KEYNUM
}KEY_TYPEDEF;			//按键定义

 

// 按键检测过程
typedef enum
{
	KEY_STEP_WAIT,			//等待按键
	KEY_STEP_CLICK,				//按键按下
	KEY_STEP_LONG_PRESS,				//长按
	KEY_STEP_CONTINUOUS_PRESS,  			//持续按下
}KEY_STEP_TYPEDEF;




typedef enum
{	
	KEY_IDLE,       	 		 							//按键空闲
	KEY_CLICK,          								//单击确认
	KEY_CLICK_RELEASE,            			//单击释放
	KEY_LONG_PRESS,			   						 	//长按确认
	KEY_LONG_PRESS_CONTINUOUS,							//长按持续
	KEY_LONG_PRESS_RELEASE								//长按释放
	 
}KEY_EVENT_TYPEDEF;

typedef enum
{
	KEY_IDLE_VAL,	//0
	KEY1_CLICK,		//1
	KEY1_CLICK_RELEASE,		//2
	KEY1_LONG_PRESS,			//3
	KEY1_LONG_PRESS_CONTINUOUS,	//4
	KEY1_LONG_PRESS_RELEASE,		//5
	
	KEY2_CLICK,								//6
	KEY2_CLICK_RELEASE,
	KEY2_LONG_PRESS,
	KEY2_LONG_PRESS_CONTINUOUS,
	KEY2_LONG_PRESS_RELEASE,
	
	KEY3_CLICK,							//11
	KEY3_CLICK_RELEASE,
	KEY3_LONG_PRESS,
	KEY3_LONG_PRESS_CONTINUOUS,
	KEY3_LONG_PRESS_RELEASE,
	
	KEY4_CLICK,						//16
	KEY4_CLICK_RELEASE,
	KEY4_LONG_PRESS,
	KEY4_LONG_PRESS_CONTINUOUS,
	KEY4_LONG_PRESS_RELEASE,
	
	KEY5_CLICK,						//21
	KEY5_CLICK_RELEASE,
	KEY5_LONG_PRESS,
	KEY5_LONG_PRESS_CONTINUOUS,
	KEY5_LONG_PRESS_RELEASE,
	
	KEY6_CLICK,						//26
	KEY6_CLICK_RELEASE,
	KEY6_LONG_PRESS,
	KEY6_LONG_PRESS_CONTINUOUS,
	KEY6_LONG_PRESS_RELEASE,
	
	
}KEY_VALUE_TYPEDEF;


typedef void (*KeyEvent_CallBack_t)(KEY_VALUE_TYPEDEF keys);

//扫描按键的定时器Tick,以系统Tick(1ms)为单位,10=10ms
#define KEY_SCANT_TICK		10		//10ms

//按键消抖时间,以10ms为Tick,2=20ms
#define KEY_SCANTIME	2		//20ms

//连续长按时间
#define	KEY_PRESS_LONG_TIME	200	//2s

//持续长按间隔时间
#define KEY_PRESS_CONTINUE_TIME	15	//150ms 

void hal_KeyInit(void);
void hal_KeyProc(void);
void hal_KeyScanCBSRegister(KeyEvent_CallBack_t pCBS);


#endif 
