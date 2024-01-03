#ifndef _PARA_H
#define _PARA_H

#define PARA_DTC_SUM		20						//支持探测器的总数量

typedef enum
{
    DTC_DOOR,   //门磁探测器
    DTC_PIR_MOTION,     //红外
    DTC_REMOTE,         // 遥控
    DTC_TYP_SUM
}DTC_TYPE_TYPEDEF;

typedef enum 
{
    ZONE_TYP_24HOURS,       //24小时警戒
    ZONE_TYP_1ST,          //布防
    ZONE_TYP_2ND,           //在家布防
    STG_DEV_AT_SUM
}ZONE_TYPED_TYPEDEF;

typedef struct 
{
    unsigned char ID;
    unsigned char Mark;
    unsigned char NameNum;
    unsigned char Name[16];
    DTC_TYPE_TYPEDEF DTCType;
    ZONE_TYPED_TYPEDEF ZoneType;
    unsigned char Code[3];          //1527 // 2262   24bits 数据
}Stu_DTC;

#define STU_DTC_SIZE    (sizeof(Stu_DTC))
#define STU_DEVICEPARA_OFFSET			0


void Para_Init(void);
unsigned char AddDtc(Stu_DTC *pDevPara);
void GetDtcStu(Stu_DTC *pdDevPara, unsigned char idx);
unsigned char CheckPresenceofDtc(unsigned char i);
void SetDtcAbt(unsigned char id,Stu_DTC *psDevPara);
void FactoryReset(void);
unsigned char DtcMatching(unsigned char *pCode);

#endif
