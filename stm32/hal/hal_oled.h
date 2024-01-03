/*
 * @Author: your name
 * @Date: 2021-11-22 18:05:09
 * @LastEditTime: 2021-11-22 18:42:28
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: \Wifi Host\hal\hal_oled.h
 */
#ifndef __HAL_OLED_H
#define __HAL_OLED_H 





#define OLED_CLK_PIN    GPIO_Pin_5
#define OLED_SDA_PIN    GPIO_Pin_7
#define OLED_RES_PIN    GPIO_Pin_4
#define OLED_DC_PIN     GPIO_Pin_6



//-----------------OLED端口定义---------------- 

#define OLED_SCL_Clr() GPIO_ResetBits(GPIOA,OLED_CLK_PIN)//SCL
#define OLED_SCL_Set() GPIO_SetBits(GPIOA,OLED_CLK_PIN)

#define OLED_SDA_Clr() GPIO_ResetBits(GPIOA,OLED_SDA_PIN)//SDA
#define OLED_SDA_Set() GPIO_SetBits(GPIOA,OLED_SDA_PIN)

#define OLED_RES_Clr() GPIO_ResetBits(GPIOA,OLED_RES_PIN)//RES
#define OLED_RES_Set() GPIO_SetBits(GPIOA,OLED_RES_PIN)

#define OLED_DC_Clr()  GPIO_ResetBits(GPIOA,OLED_DC_PIN)//DC
#define OLED_DC_Set()  GPIO_SetBits(GPIOA,OLED_DC_PIN)
 		     
#define OLED_CS_Clr()
#define OLED_CS_Set()


#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据

void hal_Oled_ClearPoint(unsigned char x,unsigned char y);
void hal_Oled_ColorTurn(unsigned char i);
void hal_Oled_DisplayTurn(unsigned char i);
void hal_Oled_WR_Byte(unsigned char dat,unsigned char mode);
void hal_Oled_Refresh(void);
void hal_Oled_Clear(void);
void hal_Oled_DrawPoint(unsigned char x,unsigned char y,unsigned char t);
void hal_Oled_DrawLine(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,unsigned char mode);
void hal_Oled_DrawCircle(unsigned char x,unsigned char y,unsigned char r);
void hal_Oled_ShowChar(unsigned char x,unsigned char y,unsigned char chr,unsigned char size1,unsigned char mode);
void hal_Oled_ShowChar6x8(unsigned char x,unsigned char y,unsigned char chr,unsigned char mode);
void hal_Oled_ShowString(unsigned char x,unsigned char y,unsigned char *chr,unsigned char size1,unsigned char mode);
void hal_Oled_ShowNum(unsigned char x,unsigned char y,unsigned int num,unsigned char len,unsigned char size1,unsigned char mode);
void hal_Oled_ScrollDisplay(unsigned char num,unsigned char space,unsigned char mode);
void hal_Oled_Init(void);
void hal_Oled_ClearArea(unsigned char x,unsigned char y,unsigned char sizex,unsigned char sizey);

void hal_Oled_DisPlay_On(void);
void hal_Oled_DisPlay_Off(void);







#endif

