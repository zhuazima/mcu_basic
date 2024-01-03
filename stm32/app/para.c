#include "para.h"
#include "hal_eeprom.h"

Stu_DTC dERP[PARA_DTC_SUM];	//EEPROM设备参数数据结构

void FactoryReset(void);
static unsigned char ParaCheck(void);


void Para_Init(void)
{
    I2C_Read(STU_DEVICEPARA_OFFSET,(unsigned char*)(dERP),sizeof(dERP));

    if(ParaCheck())		//eeprom 中数据不合规
    {
        FactoryReset();
    }

}

//检查DTC是否存在，0不存在，1存在
unsigned char CheckPresenceofDtc(unsigned char i)
{
	unsigned char result;
	result = 0;
	if(i < PARA_DTC_SUM)	//防溢出检测
	{
		if(dERP[i].Mark)
		{
			result = 1;
		}
	}
	
	return result;
}

//获取指定探测器的结构体数据,*pdDevPara-外部结构体指针，idx-要获取的探测器结构体数组下标
void GetDtcStu(Stu_DTC *pdDevPara, unsigned char idx)
{
	unsigned char i;
 
	if(idx >= PARA_DTC_SUM)		
	{
		return;			//id异常
	}
	
	pdDevPara->ID = dERP[idx].ID;
	
	pdDevPara->Mark = dERP[idx].Mark ;
	pdDevPara->NameNum = dERP[idx].NameNum;
	
	for(i=0; i<16; i++)
	{
		pdDevPara->Name[i] = dERP[idx].Name[i];
	}
	pdDevPara->DTCType = dERP[idx].DTCType;
	pdDevPara->ZoneType = dERP[idx].ZoneType;
	
	
	 
	pdDevPara->Code[0] = dERP[idx].Code[0];
	pdDevPara->Code[1] = dERP[idx].Code[1];
	pdDevPara->Code[2] = dERP[idx].Code[2];
}



void FactoryReset(void)
{
	unsigned short i;
	unsigned char j;
	//所有传感器初始化
	for(i=0; i<PARA_DTC_SUM; i++)
	{
		dERP[i].ID = 0;
		dERP[i].Mark = 0;
		dERP[i].NameNum = 0;
		for(j=0; j<16; j++)
		{
			dERP[i].Name[j] = 0;
		}
		dERP[i].DTCType = DTC_DOOR;
		dERP[i].ZoneType = ZONE_TYP_1ST;
		dERP[i].Code[0] = 0;
		dERP[i].Code[1] = 0;
		dERP[i].Code[2] = 0;
	}
	
	// CreatDtc(20);
	
	I2C_PageWrite(STU_DEVICEPARA_OFFSET,(unsigned char*)(dERP),sizeof(dERP));
	
	
	I2C_Read(STU_DEVICEPARA_OFFSET,(unsigned char*)(dERP),sizeof(dERP));

}


static unsigned char ParaCheck(void)
{
    unsigned char i;
    unsigned char error = 0;
	if(dERP[0].ID != 1)
	{
		error = 1;
	}
	for(i=0; i<PARA_DTC_SUM; i++)
	{
		if(dERP[i].ID >= PARA_DTC_SUM)
		{
			error = 1;
		}
		if(dERP[i].Mark > 1)
		{
			error = 1;
		}
		if(dERP[i].NameNum > PARA_DTC_SUM)
		{
			error = 1;
		}
		 
		if(dERP[i].DTCType>= DTC_TYP_SUM)
		{
			error = 1;
		}
		
		if(dERP[i].ZoneType>= STG_DEV_AT_SUM)
		{
			error = 1;
		}
		 
	}
	 
	
	return error;
}



unsigned char DtcMatching(unsigned char *pCode)
{
	unsigned char i=0;

	for(i=0; i<PARA_DTC_SUM; i++)
	{
		if(dERP[i].Mark && 
		(dERP[i].Code[1]==pCode[1]) &&
		(dERP[i].Code[2]==pCode[2]))			//判断探测器是否存在
		{				
		 
			return (dERP[i].ID);
		} 
	}
	return 0xFF;
}

unsigned char AddDtc(Stu_DTC *pDevPara)
{
	Stu_DTC DevPara;
	unsigned char i,j,Temp,NameStrIndex;

	NameStrIndex = 0;
	Temp = 0;
	
	for(i=0; i<PARA_DTC_SUM; i++)
	{
		if(dERP[i].Mark && 
		(dERP[i].Code[1]==pDevPara->Code[1]) &&
		(dERP[i].Code[2]==pDevPara->Code[2]))			//判断探测器是否存在
		{				
			return (i);									//探测器已存在，也提示配对成功
		}
		
	}
	for(i=0; i<PARA_DTC_SUM; i++)
	{
		if(!dERP[i].Mark)
		{
			DevPara.Name[0] = 'Z';
			DevPara.Name[1] = 'o';
			DevPara.Name[2] = 'n';
			DevPara.Name[3] = 'e';
			DevPara.Name[4] = '-';
			NameStrIndex = 5;
			Temp = 	i+1;	
			
			DevPara.Name[NameStrIndex++] = '0'+(Temp/100);
			DevPara.Name[NameStrIndex++] = '0'+((Temp%100)/10);
			DevPara.Name[NameStrIndex++] = '0'+((Temp%100)%10);
			
			for(j=NameStrIndex; j<16; j++)
			{
				DevPara.Name[j] = 0;					//把没用到的名字字节清零
			}
			DevPara.ID = i+1;
			DevPara.Mark = 1;
			DevPara.NameNum = Temp;
			
			
			DevPara.DTCType = pDevPara->DTCType;
			DevPara.ZoneType = pDevPara->ZoneType;
			
			
			DevPara.Code[0] = pDevPara->Code[0];
			DevPara.Code[1] = pDevPara->Code[1];
			DevPara.Code[2] = pDevPara->Code[2];

			I2C_PageWrite(STU_DEVICEPARA_OFFSET+i*STU_DTC_SIZE,(unsigned char*)(&DevPara),sizeof(DevPara)); //新设备信息写入EEPROM
			I2C_Read(STU_DEVICEPARA_OFFSET+i*STU_DTC_SIZE,(unsigned char*)&dERP[i],STU_DTC_SIZE);

			return (i);							//学习成功，返回探测器的存储下标
		}
	}
	return 0xFF;			//学习失败
}



//修改探测器属性,id->指定探测器idx psDevPara->探测器属性结构体
void SetDtcAbt(unsigned char id,Stu_DTC *psDevPara)
{
	unsigned char i;
	if(id >= PARA_DTC_SUM)		
	{
		return;			//id异常
	}
	dERP[id].ID = psDevPara->ID;
	dERP[id].Mark = psDevPara->Mark ;
	dERP[id].NameNum =  psDevPara->NameNum;
	
	for(i=0; i<16; i++)
	{
		dERP[id].Name[i] = psDevPara->Name[i];
	}
	dERP[id].DTCType = psDevPara->DTCType;
	dERP[id].ZoneType = psDevPara->ZoneType;
	
	

	dERP[id].Code[0] = psDevPara->Code[0];
	dERP[id].Code[1] = psDevPara->Code[1];
	dERP[id].Code[2] = psDevPara->Code[2];
	
	I2C_PageWrite(STU_DEVICEPARA_OFFSET+id*STU_DTC_SIZE,(unsigned char*)psDevPara,STU_DTC_SIZE); //新设备信息写入EEPROM
	I2C_Read(STU_DEVICEPARA_OFFSET+id*STU_DTC_SIZE,(unsigned char*)&dERP[id],STU_DTC_SIZE);
}
