#include "hal_eeprom.h"
#include "stm32f10x.h"


#define EEPROM_PAGE_SIZE 64

static void hal_EepromConfig(void);
static void hal_I2C_SDA(unsigned char bVal);
static void hal_I2C_SCL(unsigned char bVal);
static void hal_I2C_SDA_IO_Set(unsigned char IOMode);
unsigned char hal_I2C_SDA_INPUT(void);

// static void I2C_delay(unsigned short t)
// {
// 	unsigned char i;
// 	while(t--)
// 	{
// 		i = 50;
// 		while(i--);
// 	}
// } 

static void I2C_delay(unsigned short t)
{	

   unsigned short i=50,j,c;
   c = t;
   for(j=0; j<c; j++)
   {
	   while(i) 
	   { 
			i--; 
	   } 
	}
}

static void I2C_Start(void)
{
	hal_I2C_SDA(1);
	I2C_delay(1);
	hal_I2C_SCL(1);
	I2C_delay(1);
	hal_I2C_SDA(0);
	I2C_delay(1);

}

static void I2C_Stop(void)
{
	hal_I2C_SDA(0);
	I2C_delay(1);
	hal_I2C_SCL(1);
	I2C_delay(1);
	hal_I2C_SDA(1);
	I2C_delay(1);
}

static void I2C_SendByte(unsigned char sendbyte)
{
	unsigned char i;

	for(i = 0;i < 8 ;i++)
	{	
		hal_I2C_SCL(0);
		I2C_delay(1);

		if(sendbyte & 0x80)
		{
			hal_I2C_SDA(1);
			I2C_delay(1);
		}
		else
		{
			hal_I2C_SDA(0);
			I2C_delay(1);
		}

		hal_I2C_SCL(1);
		I2C_delay(1);
		sendbyte <<= 1;

	}

	hal_I2C_SCL(0);
	I2C_delay(1);
	hal_I2C_SDA(1);		//这里拉高SDA避免影响ACk的判断
	I2C_delay(1);

}


static unsigned char I2C_ReceiveByte(void)  
{ 
	unsigned char i;
	unsigned char ReceiveByte=0;
	
	hal_I2C_SCL(0);
	I2C_delay(1);
	hal_I2C_SDA(1);
	hal_I2C_SDA_IO_Set(1);		 //SDA设置成输入
	for(i=0; i<8; i++)
	{
		ReceiveByte <<= 1;
		I2C_delay(1);
		hal_I2C_SCL(1);
		I2C_delay(1);
		if(hal_I2C_SDA_INPUT())
		{
			ReceiveByte|=0x01;
		}
		hal_I2C_SCL(0);
		
	}
	hal_I2C_SDA_IO_Set(0);		//SDA设置成输出
	I2C_delay(1);
	return ReceiveByte;
}

static void I2C_Ack(void)
{
	hal_I2C_SCL(0);
	I2C_delay(1);
	hal_I2C_SDA(0);
	I2C_delay(1);
	hal_I2C_SCL(1);
	I2C_delay(1);
	hal_I2C_SCL(0);
	I2C_delay(1);
}

static void I2C_NoAck(void)
{
	hal_I2C_SCL(0);
	I2C_delay(1);
	hal_I2C_SDA(1);
	I2C_delay(1);
	hal_I2C_SCL(1);
	I2C_delay(1);
	hal_I2C_SCL(0);
	I2C_delay(1);
}

static unsigned char I2C_WaitAck(void)
{

	hal_I2C_SDA(1);
	hal_I2C_SDA_IO_Set(1);		 
	hal_I2C_SCL(1);
	I2C_delay(1);
	if(hal_I2C_SDA_INPUT())
	{
		return 0;
	}
	hal_I2C_SCL(0);
	hal_I2C_SDA_IO_Set(0);		 
	I2C_delay(1); 
	return 1;

}

 
 
unsigned char hal_I2C_SDA_INPUT(void)
{
	return GPIO_ReadInputDataBit(I2C_SDA_PORT, I2C_SDA_PIN);		
}


static void hal_I2C_SDA_IO_Set(unsigned char IOMode)
{
	if(IOMode == 0)					//输出
	{
		GPIO_InitTypeDef  GPIO_InitStructure; 
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		GPIO_InitStructure.GPIO_Pin =   I2C_SDA_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;  
		GPIO_Init(I2C_SDA_PORT, &GPIO_InitStructure);
	}else if(IOMode == 1)			//输入	
	{
		GPIO_InitTypeDef  GPIO_InitStructure; 
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		GPIO_InitStructure.GPIO_Pin =   I2C_SDA_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  
		GPIO_Init(I2C_SDA_PORT, &GPIO_InitStructure);

	}
	 
}


void hal_EepromInit(void)
{
	hal_EepromConfig();
}

static void hal_EepromConfig(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure; 
	/* Configure I2C2 pins: PB8->SCL and PB9->SDA */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin =  I2C_SCL_PIN | I2C_SDA_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;  
	GPIO_Init(I2C_SCL_PORT, &GPIO_InitStructure);

	hal_I2C_SDA(1);
	hal_I2C_SCL(1);
}

static void hal_I2C_SDA(unsigned char bVal)
{
	if(bVal)
	{
		GPIO_SetBits(I2C_SDA_PORT,I2C_SDA_PIN);
	}
	else
	{
		GPIO_ResetBits(I2C_SDA_PORT,I2C_SDA_PIN);

	}
}

static void hal_I2C_SCL(unsigned char bVal)
{
	if(bVal)
	{
		GPIO_SetBits(I2C_SCL_PORT,I2C_SCL_PIN);
	}
	else
	{
		GPIO_ResetBits(I2C_SCL_PORT,I2C_SCL_PIN);

	}
}

void I2C_WriteByte(unsigned short address,unsigned char dat)
{
	I2C_Start();

	I2C_SendByte(0xA0);
	I2C_WaitAck();

	I2C_SendByte((address>>8) & 0xFF);		//高字节地址
	I2C_WaitAck();

	I2C_SendByte( address & 0xFF);		//低字节地址
	I2C_WaitAck();

	I2C_SendByte(dat);
	I2C_WaitAck();

	I2C_Stop();
}

void I2C_PageWrite(unsigned short address,unsigned char *pDat, unsigned short num)
{
	unsigned char *pBuffer,j;
	unsigned short len,i,page,remainder,addr,temp;
	pBuffer = pDat;
	len = num;		
	addr = address;
	temp = 0;
	if(addr%EEPROM_PAGE_SIZE)	//判断要写的地址
	{
		temp = EEPROM_PAGE_SIZE-(addr%EEPROM_PAGE_SIZE);			//32-7=25 //计算出当前地址还差多少字节满1页
		if(len<=temp)
		{
			temp = len;
		}
	}
	
	//先填满当前页
	if(temp)
	{
		I2C_Start();
		
		I2C_SendByte(0xA0);
		I2C_WaitAck();
		
		I2C_SendByte((addr>>8)&0xFF);
		I2C_WaitAck();
		
		I2C_SendByte(addr&0xFF);
		I2C_WaitAck();
		
		for(j=0; j<temp; j++)		 
		{
			I2C_SendByte(pBuffer[j]);
			I2C_WaitAck();	
		}
		I2C_Stop();
		I2C_delay(20000);
	}
		
	
	len -= temp;			//长度减去已经写入的字节
	addr += temp;			//地址加上已经写入的字节
 
	page = len/EEPROM_PAGE_SIZE;			//计算页数
	remainder = len%EEPROM_PAGE_SIZE;		//计算另起一页还有多少字节
	for(i=0; i<page; i++)					//页写
	{
		I2C_Start();
		I2C_SendByte(0xA0);
		I2C_WaitAck();
		
		I2C_SendByte((addr>>8)&0xFF);
		I2C_WaitAck();
		
		I2C_SendByte(addr&0xFF);
		I2C_WaitAck();
		
		
		for(j=0;j<EEPROM_PAGE_SIZE;j++)
		{
			I2C_SendByte(pBuffer[temp+j]);
			I2C_WaitAck();
		}
		I2C_Stop();
		addr += EEPROM_PAGE_SIZE;
		temp += EEPROM_PAGE_SIZE;
		I2C_delay(20000);		
	}
	
	if(remainder)			//另起一页
	{
		I2C_Start();
		I2C_SendByte(0xA0);
		I2C_WaitAck();
		
		I2C_SendByte((addr>>8)&0xFF);
		I2C_WaitAck();
		
		I2C_SendByte(addr&0xFF);
		I2C_WaitAck();
		
		for(j=0; j<remainder; j++)		 
		{
			I2C_SendByte(pBuffer[temp+j]);
			I2C_WaitAck();	
		}
		I2C_Stop();
		I2C_delay(20000);		
	}
}


//读1个字节
unsigned char I2C_ReadByte(unsigned short address)
{
	unsigned char dat;
	I2C_Start();
	I2C_SendByte(0xA0);
	I2C_WaitAck();

	I2C_SendByte((address>>8)&0xFF);
	I2C_WaitAck();

	I2C_SendByte(address&0xFF);
	I2C_WaitAck();

	I2C_Start();
	I2C_SendByte(0xA1);
	I2C_WaitAck();
	
	dat = I2C_ReceiveByte();
	I2C_NoAck();
	I2C_Stop();
	return dat;
}

void I2C_Read(unsigned short address,unsigned char *pBuffer, unsigned short len)
{
	unsigned short length;
	length = len;
	I2C_Start();
	I2C_SendByte(0xA0);
	I2C_WaitAck();

	I2C_SendByte((address>>8)&0xFF);
	I2C_WaitAck();

	I2C_SendByte(address&0xFF);
	I2C_WaitAck();

	I2C_Start();
	I2C_SendByte(0xA1);
	I2C_WaitAck();
	
	//dat = I2C_ReceiveByte();
	 while(length)
    {
      *pBuffer = I2C_ReceiveByte();
      if(length == 1)I2C_NoAck();
      else I2C_Ack(); 
      pBuffer++;
      length--;
    }
	 
	I2C_Stop();
	 
}








