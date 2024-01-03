#ifndef _HAL_BEEP_H
#define _HAL_BEEP_H



#define BEEP_PORT			GPIOB
#define BEEP_PIN			GPIO_Pin_4

#define BEEP_EN_PORT	GPIOA
#define BEEP_EN_PIN	GPIO_Pin_0


void hal_BeepInit(void);
void hal_BeepPwmCtrl(unsigned char cmd);







#endif
