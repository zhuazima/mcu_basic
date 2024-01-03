#ifndef _HAL_RFD_H
#define _HAL_RFD_H

#define RFD_GPIO_PORT       GPIOA
#define RFD_GPIO_PIN        GPIO_Pin_11


 //引导码脉宽允许误差范围
#define  RFD_TITLE_CLK_MINL  20
#define  RFD_TITLE_CLK_MAXL  44

 //数据码脉宽允许误差范围
#define  RFD_DATA_CLK_MINL   2
#define  RFD_DATA_CLK_MAXL   5


typedef void (*RFD_RcvCallBack_t)(unsigned char *pBuff);


void hal_RFDInit(void);
void hal_RFDProc(void);
void hal_RFCRcvCBSRegister(RFD_RcvCallBack_t pCBS);












#endif
