#ifndef _HAL_EEPROM_H
#define _HAL_EEPROM_H

#define I2C_SCL_PORT GPIOB
#define I2C_SCL_PIN GPIO_Pin_8

#define I2C_SDA_PORT GPIOB
#define I2C_SDA_PIN GPIO_Pin_9


void hal_EepromInit(void);
void I2C_PageWrite(unsigned short address,unsigned char *pDat, unsigned short num);
void I2C_Read(unsigned short address,unsigned char *pBuffer, unsigned short len);


#endif
