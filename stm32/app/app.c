
#include "hal_led.h"
#include "hal_key.h"
#include "hal_oled.h"
#include "hal_Uart.h"
#include "OS_System.h"
#include "hal_rfd.h"
#include "hal_eeprom.h"
#include "para.h"
#include "hal_beep.h"
#include "string.h"
#include "mcu_api.h"
#include "wifi.h"
#include "protocol.h"
#include "app.h"


Queue8 RFDRcvMsg;	//RFD���ն���
Queue8 DtcTriggerIDMsg;	//������̽����ID����

LINK_STATE_TYPEDEF link_state;

stu_system_time stuSystemtime;		//ϵͳʱ��
stu_mode_menu *pModeMenu;		//ϵͳ��ǰ�˵�
stu_system_mode *pStuSystemMode;		//��ǰϵͳ����ģʽ
unsigned char wifiWorkState,bwifiWorkState;	//WiFiģ�鹤��״̬

unsigned short SetupMenuTimeOutCnt;

unsigned short PutoutScreenTiemr;		//����ʱ��
unsigned char ScreenState;				//��Ļ״̬,0������1����


unsigned char *pMcuVersions = "v2.8";		//������д���ַ���ֻ�ܸ�ֵ��ָ��
unsigned char *pHardVersions = "v2.0";


static void KeyEventHandle(KEY_VALUE_TYPEDEF keys);
static void RfdRcvHandle(unsigned char *pBuff);
void Menu_Reset(void);
void Menu_Init(void);
static void gnlMenu_DesktopCBS(void);
static void stgMenu_MainMenuCBS(void);
static void stgMenu_FactorySettingsCBS(void);
static void stgMenu_MachineInfoCBS(void);
static void stgMenu_WifiCBS(void);
static void stgMenu_DTCListCBS(void);
static void stgMenu_LearnSensorCBS(void);
static void stgMenu_dl_EditCBS(void);
static void stgMenu_dl_DeleteCBS(void);
static void stgMenu_dl_ReviewCBS(void);
static void stgMenu_dl_ReviewMainCBS(void);
static void HexToAscii(unsigned char *pHex, unsigned char *pAscii, int nLen);

static void ScreeControl(unsigned char cmd);

static void S_ENArmModeProc(void);
static void S_AlarmModeProc(void);
static void S_HomeArmModeProc(void);
static void S_DisArmModeProc(void);

static void ServerEventHandle(WIFI_MSG_TYPE type,unsigned char *pData);

stu_system_mode stu_Sysmode[SYSTEM_MODE_SUM] =
{
	{SYSTEM_MODE_ENARM,SCREEN_CMD_RESET,0xFF,S_ENArmModeProc},
	{SYSTEM_MODE_HOMEARM,SCREEN_CMD_RESET,0xFF,S_HomeArmModeProc},
	{SYSTEM_MODE_DISARM,SCREEN_CMD_RESET,0xFF,S_DisArmModeProc},
	{SYSTEM_MODE_ALARM,SCREEN_CMD_RESET,0xFF,S_AlarmModeProc},
};

//��ʼ������˵�
stu_mode_menu generalModeMenu[GNL_MENU_SUM] =
{
	{GNL_MENU_DESKTOP,DESKTOP_MENU_POS,"Desktop",gnlMenu_DesktopCBS,1,0,0xFF,0,0,0,0},
	
};	

//��ʼ�����ò˵�
stu_mode_menu settingModeMenu[STG_MENU_SUM] = 
{
	{STG_MENU_MAIN_SETTING,STG_MENU_POS,"Main Menu",stgMenu_MainMenuCBS,1,0,0xFF,0,0,0,0},		//������ҳ��
	{STG_MENU_LEARNING_SENSOR,STG_SUB_2_MENU_POS,"1. Learning Dtc",stgMenu_LearnSensorCBS,1,0,0xFF,0,0,0,0},	//̽����ѧϰ
	{STG_MENU_DTC_LIST,STG_SUB_2_MENU_POS,"2. Dtc List",stgMenu_DTCListCBS,1,0,0xFF,0,0,0,0},			//����


	{STG_MENU_WIFI,STG_WIFI_MENU_POS,"3. WiFi",stgMenu_WifiCBS,1,0,0xFF,0,0,0,0},
	//{STG_MENU_TIME,STG_SUB_2_MENU_POS,"4. Time Set",stgMenu_TimeCBS,1,0,0xFF,0,0,0,0},
	{STG_MENU_MACHINE_INFO,STG_SUB_2_MENU_POS,"4. Mac Info",stgMenu_MachineInfoCBS,1,0,0xFF,0,0,0,0},
	{STG_MENU_FACTORY_SETTINGS,STG_SUB_2_MENU_POS,"5. Default Setting",stgMenu_FactorySettingsCBS,1,0,0xFF,0,0,0,0},
};

stu_mode_menu DL_ZX_Review[STG_MENU_DL_ZX_SUM] = 
{
	{STG_MENU_DL_ZX_REVIEW_MAIN,STG_SUB_2_MENU_POS,"View",stgMenu_dl_ReviewMainCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0},	
	{STG_MENU_DL_ZX_REVIEW,STG_SUB_3_MENU_POS,"View",stgMenu_dl_ReviewCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0},		 
	{STG_MENU_DL_ZX_EDIT,STG_SUB_3_MENU_POS,"Edit",stgMenu_dl_EditCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0},
	{STG_MENU_DL_ZX_DELETE,STG_SUB_3_MENU_POS,"Delete",stgMenu_dl_DeleteCBS,SCREEN_CMD_RESET,0,0xFF,0,0,0,0},
};


void AppInit(void)
{
	
	hal_Oled_Init();
	hal_EepromInit();
	Para_Init();
	hal_BeepInit();
	Menu_Init();
	hal_KeyScanCBSRegister(KeyEventHandle);
	hal_RFCRcvCBSRegister(RfdRcvHandle);
	ServerEventCBSRegister(ServerEventHandle);

	QueueEmpty(RFDRcvMsg);
	QueueEmpty(DtcTriggerIDMsg);

	stuSystemtime.year = 2021;
	stuSystemtime.mon = 12;
	stuSystemtime.day = 04;
	stuSystemtime.hour = 21;
	stuSystemtime.min = 23;
	stuSystemtime.week = 6;

	pStuSystemMode = &stu_Sysmode[SYSTEM_MODE_ENARM];


}

static void showSystemTime(void)
{
//2021-04-09 18:10 Sun
	hal_Oled_ShowChar(4,54,(stuSystemtime.year/1000)+'0',8,1);
	hal_Oled_ShowChar(10,54,((stuSystemtime.year%1000)/100)+'0',8,1);
	hal_Oled_ShowChar(16,54,((stuSystemtime.year%1000%100)/10)+'0',8,1);
	hal_Oled_ShowChar(22,54,(stuSystemtime.year%1000%100%10)+'0',8,1);
  
	//hal_Oled_Refresh();
	//4+4*6=28
	hal_Oled_ShowString(28,54,"-",8,1);
	//hal_Oled_Refresh();
	//04
	//28+6=34
 
	hal_Oled_ShowChar(34,54,(stuSystemtime.mon/10)+'0',8,1);
	hal_Oled_ShowChar(40,54,(stuSystemtime.mon%10)+'0',8,1);
	//hal_Oled_Refresh();
	
	//hal_Oled_Refresh();
	///34+2*6=46
	hal_Oled_ShowString(46,54,"-",8,1);
	//hal_Oled_Refresh();
	//46+6=52

	hal_Oled_ShowChar(52,54,(stuSystemtime.day/10)+'0',8,1);
	hal_Oled_ShowChar(58,54,(stuSystemtime.day%10)+'0',8,1);
	
	//hal_Oled_Refresh();
	//52+12=64
	hal_Oled_ShowString(64,54," ",8,1);
	//hal_Oled_Refresh();
	//64+6=70
	
	hal_Oled_ShowChar(70,54,(stuSystemtime.hour/10)+'0',8,1);
	hal_Oled_ShowChar(76,54,(stuSystemtime.hour%10)+'0',8,1);
		
	//hal_Oled_Refresh();
	//70+12=82
	hal_Oled_ShowString(82,54,":",8,1);
	//hal_Oled_Refresh();
	//82+6
 
	hal_Oled_ShowChar(88,54,(stuSystemtime.min/10)+'0',8,1);
	hal_Oled_ShowChar(94,54,(stuSystemtime.min%10)+'0',8,1);
		
	//hal_Oled_Refresh();
	//88+12
	hal_Oled_ShowString(100,54," ",8,1);
	//hal_Oled_Refresh();
	switch(stuSystemtime.week)
	{
		case 1:	//����1
			hal_Oled_ShowString(106,54,"Mon",8,1);
		break;
		case 2:	//����2
			hal_Oled_ShowString(106,54,"Tue",8,1);
		break;
		case 3:	//����3
			hal_Oled_ShowString(106,54,"Wed",8,1);
		break;
		case 4:	//����4
			hal_Oled_ShowString(106,54,"Thu",8,1);
		break;
		case 5:	//����5
			hal_Oled_ShowString(106,54,"Fir",8,1);
		break;
		case 6:	//����6
			hal_Oled_ShowString(106,54,"Sat",8,1);
		break;
		case 7:	//����7
			hal_Oled_ShowString(106,54,"Sun",8,1);
		break;
		
	}
	hal_Oled_Refresh();
}



void AppProc(void)
{
	wifi_uart_service();
	pModeMenu->action();

	if((pModeMenu->menuPos!=DESKTOP_MENU_POS) 
	&&(pModeMenu->menuPos!=STG_WIFI_MENU_POS))
	{
		SetupMenuTimeOutCnt++;
		if(SetupMenuTimeOutCnt > SETUPMENU_TIMEOUT_PERIOD)
		{
			SetupMenuTimeOutCnt = 0;
			pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];	//�����ϵ���ʾ�Ĳ˵�����Ϊ������ʾ
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;	//����ˢ�½����־����������ˢ��ȫ����UI

		}
	}
			
	if((pStuSystemMode->ID!=SYSTEM_MODE_ALARM)
	&& (pModeMenu->menuPos!=STG_WIFI_MENU_POS))
	{
		PutoutScreenTiemr++;
		if(PutoutScreenTiemr > PUTOUT_SCREEN_PERIOD)
		{
			PutoutScreenTiemr = 0;
			
			//30��û�κβ����Զ�Ϩ��
			ScreeControl(0);
			 
		}
	}
}



void Desk_Menu_ActionCBS(void)
{
	if(pModeMenu ->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pModeMenu ->refreshScreenCmd = SCREEN_CMD_NULL;

		hal_Oled_Clear();
		
		hal_Oled_ShowString(0,0,"w",8,1);	//8 *16
		hal_Oled_ShowString(16,20,"Away arm",24,1); //12*24
		showSystemTime();

		hal_Oled_Refresh();
	}

}

void Menu_Init(void)
{

	unsigned char i;
	//�����ò˵���ʼ��,�Ѳ˵��б��γ�������ʽ���������
	settingModeMenu[1].pLase = &settingModeMenu[STG_MENU_SUM-1];
	settingModeMenu[1].pNext = &settingModeMenu[2];
	settingModeMenu[1].pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];
	
	for(i=2; i<STG_MENU_SUM-1; i++)
	{
		settingModeMenu[i].pLase = &settingModeMenu[i-1];
		settingModeMenu[i].pNext = &settingModeMenu[i+1];
		settingModeMenu[i].pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];
	}
	settingModeMenu[STG_MENU_SUM-1].pLase = &settingModeMenu[i-1];
	settingModeMenu[STG_MENU_SUM-1].pNext = &settingModeMenu[1];
	settingModeMenu[STG_MENU_SUM-1].pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];


	DL_ZX_Review[1].pLase = &DL_ZX_Review[STG_MENU_DL_ZX_SUM-1];
	DL_ZX_Review[1].pNext = &DL_ZX_Review[2];
	DL_ZX_Review[1].pParent = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
	for(i=2; i<STG_MENU_DL_ZX_SUM-1; i++)
	{
		DL_ZX_Review[i].pLase = &DL_ZX_Review[i-1];
		DL_ZX_Review[i].pNext = &DL_ZX_Review[i+1];
		DL_ZX_Review[i].pParent = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
	}
	DL_ZX_Review[STG_MENU_DL_ZX_SUM-1].pLase = &DL_ZX_Review[i-1];
	DL_ZX_Review[STG_MENU_DL_ZX_SUM-1].pNext = &DL_ZX_Review[1];
	DL_ZX_Review[STG_MENU_DL_ZX_SUM-1].pParent = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
	

	pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];	//�����ϵ���ʾ�Ĳ˵�����Ϊ������ʾ
	pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;	//����ˢ�½����־����������ˢ��ȫ����UI

}

//�л�ϵͳ����ģʽ
static void SystemMode_Change(SYSTEMMODE_TYPEDEF sysMode)
{
	if(sysMode < SYSTEM_MODE_SUM)	//������β�(ģʽ)�Ƿ���ȷ
	{
		ScreeControl(1);
		PutoutScreenTiemr = 0;
		pStuSystemMode = &stu_Sysmode[sysMode];
		pStuSystemMode->refreshScreenCmd = SCREEN_CMD_RESET;

		mcu_dp_enum_update(DPID_MASTER_MODE,sysMode,STR_GATEWAY_ITSELF_ID,my_strlen(STR_GATEWAY_ITSELF_ID)); //�ϱ�����ģʽ
		
		if(sysMode == SYSTEM_MODE_ALARM)
		{
			hal_BeepPwmCtrl(1);
		}else
		{
			hal_BeepPwmCtrl(0);
			if(sysMode == SYSTEM_MODE_DISARM)
			{
				LedMsgInput(BUZ,LED_LIGHT_100MS,1);
			}
			LedMsgInput(BUZ,LED_LIGHT_100MS,0);
		}
	}
}

//��Ҳ���ģʽ����
static void S_ENArmModeProc(void)
{
	unsigned char tBuff[3],id,dat;
	Stu_DTC tStuDtc;
	if(pStuSystemMode->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pStuSystemMode->refreshScreenCmd = SCREEN_CMD_NULL;
		pStuSystemMode->keyVal = 0xFF;
		
	 
		hal_Oled_ClearArea(0,20,128,24);		//�����ģʽ�İ�
		hal_Oled_ClearArea(0,12,128,8);		//�屨��̽�����İ�
		hal_Oled_ShowString(16,20,"Away arm",24,1);
		 
		hal_Oled_Refresh();
	}
	
	
	if(QueueDataLen(RFDRcvMsg))
	{
		QueueDataOut(RFDRcvMsg,&dat);
		if(dat == '#')
		{
			QueueDataOut(RFDRcvMsg,&tBuff[2]);	//��ַ���8λ
			QueueDataOut(RFDRcvMsg,&tBuff[1]);	//��ַ���8λ
			QueueDataOut(RFDRcvMsg,&tBuff[0]);	//������/������
			//EV1527���뷽ʽ:1111 0000,��4λ1111�ǵ�ַ��0000������/������
			//2262���뷽ʽ��1111 0000����������/������
			id = DtcMatching(tBuff);		//̽����ƥ��
			if(id != 0xFF)
			{
				//ƥ�䵽̽����
				GetDtcStu(&tStuDtc,id-1);
				if(tStuDtc.DTCType == DTC_REMOTE)
				{
					if(tBuff[0] == SENSOR_CODE_REMOTE_DISARM)
					{
						SystemMode_Change(SYSTEM_MODE_DISARM);	//ң�������Ƴ���
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_HOMEARM)
					{
						SystemMode_Change(SYSTEM_MODE_HOMEARM);	//ң���������ڼҲ���
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_SOS)
					{
						SystemMode_Change(SYSTEM_MODE_ALARM);	//ң��������SOS����
						QueueDataIn(DtcTriggerIDMsg, &id, 1); 
					}
					else if(tBuff[0] == SENSOR_CODE_REMOTE_ENARM)
					{
						SystemMode_Change(SYSTEM_MODE_ENARM);	//ң�������Ƴ���
					}
				}if(tBuff[0]==SENSOR_CODE_DOOR_OPEN)	//((tBuff[0]==0x0A)	
				{
					SystemMode_Change(SYSTEM_MODE_ALARM);
					QueueDataIn(DtcTriggerIDMsg, &id, 1);
					//�л���Alarming(������)
					//SystemMode_Change(SYSTEM_MODE_ALARM);	//̽������������
				
				}
			}
			
		}
	}
}

static void S_HomeArmModeProc(void)
{
	unsigned char tBuff[3],id,dat;
	Stu_DTC tStuDtc;
	if(pStuSystemMode->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pStuSystemMode->refreshScreenCmd = SCREEN_CMD_NULL;
		pStuSystemMode->keyVal = 0xFF;
		
		 
		hal_Oled_ClearArea(0,20,128,24);		//�����ģʽ�İ�
		hal_Oled_ClearArea(0,12,128,8);		//�屨��̽�����İ�
		hal_Oled_ShowString(16,20,"Home Arm",24,1);
		 
		hal_Oled_Refresh();
	}
	
	if(QueueDataLen(RFDRcvMsg))
	{
		QueueDataOut(RFDRcvMsg,&dat);
		if(dat == '#')
		{
			QueueDataOut(RFDRcvMsg,&tBuff[2]);
			QueueDataOut(RFDRcvMsg,&tBuff[1]);
			QueueDataOut(RFDRcvMsg,&tBuff[0]);
			id = DtcMatching(tBuff);		//̽����ƥ��
			if(id != 0xFF)
			{
				//ƥ�䵽̽����
				GetDtcStu(&tStuDtc,id-1);
				 
				if(tStuDtc.DTCType == DTC_REMOTE)
				{
					if(tBuff[0] == SENSOR_CODE_REMOTE_ENARM)
					{
						SystemMode_Change(SYSTEM_MODE_ENARM);	//ң����������Ҳ���
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_DISARM)
					{
						SystemMode_Change(SYSTEM_MODE_DISARM);	//ң�������Ƴ���
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_HOMEARM)
					{
						SystemMode_Change(SYSTEM_MODE_HOMEARM);	//ң���������ڼҲ���
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_SOS)
					{
						SystemMode_Change(SYSTEM_MODE_ALARM);	//ң��������SOS����
						QueueDataIn(DtcTriggerIDMsg, &id, 1);
					}
				}else if((tBuff[0]==SENSOR_CODE_DOOR_OPEN)
				|| (tBuff[0]==SENSOR_CODE_DOOR_TAMPER)
				|| (tBuff[0]==SENSOR_CODE_PIR)
				|| (tBuff[0]==SENSOR_CODE_PIR_TAMPER))
				{
					if(tStuDtc.ZoneType==ZONE_TYP_24HOURS)
					{
						SystemMode_Change(SYSTEM_MODE_ALARM);	//̽����������������
						QueueDataIn(DtcTriggerIDMsg, &id, 1);
					}else if(tStuDtc.ZoneType != ZONE_TYP_2ND)	//�ж�̽�����Ƿ��Ϊ�ڼҷ���ģʽ
					{
						SystemMode_Change(SYSTEM_MODE_ALARM);	//̽����������������
						QueueDataIn(DtcTriggerIDMsg, &id, 1);
					}
				}
				
				 
			}
			
		}
		
		
	}
}
static void S_DisArmModeProc(void)
{
	unsigned char tBuff[3],id,dat;
	Stu_DTC tStuDtc;
	if(pStuSystemMode->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pStuSystemMode->refreshScreenCmd = SCREEN_CMD_NULL;
		pStuSystemMode->keyVal = 0xFF;
		
		 
		hal_Oled_ClearArea(0,20,128,24);		//�����ģʽ�İ�
		hal_Oled_ClearArea(0,12,128,8);		//�屨��̽�����İ�
		hal_Oled_ShowString(28,20,"Disarm",24,1);
		 
		hal_Oled_Refresh();
	}
	
	if(QueueDataLen(RFDRcvMsg))
	{
		QueueDataOut(RFDRcvMsg,&dat);
		if(dat == '#')
		{
			QueueDataOut(RFDRcvMsg,&tBuff[2]);
			QueueDataOut(RFDRcvMsg,&tBuff[1]);
			QueueDataOut(RFDRcvMsg,&tBuff[0]);
			id = DtcMatching(tBuff);		//̽����ƥ��
			if(id != 0xFF)
			{
				//ƥ�䵽̽����
				GetDtcStu(&tStuDtc,id-1);
				 
				if(tStuDtc.DTCType == DTC_REMOTE)
				{
					if(tBuff[0] == SENSOR_CODE_REMOTE_ENARM)
					{
						SystemMode_Change(SYSTEM_MODE_ENARM);	//ң����������Ҳ���
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_DISARM)
					{
						SystemMode_Change(SYSTEM_MODE_DISARM);	//ң�������Ƴ���
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_HOMEARM)
					{
						SystemMode_Change(SYSTEM_MODE_HOMEARM);	//ң���������ڼҲ���
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_SOS)
					{
						SystemMode_Change(SYSTEM_MODE_ALARM);	//ң��������SOS����
						QueueDataIn(DtcTriggerIDMsg, &id, 1);
					}
				}else if((tBuff[0]==SENSOR_CODE_DOOR_OPEN)
				|| (tBuff[0]==SENSOR_CODE_DOOR_TAMPER)
				|| (tBuff[0]==SENSOR_CODE_PIR)
				|| (tBuff[0]==SENSOR_CODE_PIR_TAMPER))
				{
					if(tStuDtc.ZoneType==ZONE_TYP_24HOURS)
					{
						SystemMode_Change(SYSTEM_MODE_ALARM);	//̽����������������
						QueueDataIn(DtcTriggerIDMsg, &id, 1);
					} 
				}
				 
			}
			
		}
		
		
	}
}
static void S_AlarmModeProc(void)
{
	static unsigned short timer = 0;
	static unsigned short timer2 = 0;
	static unsigned char displayAlarmFlag = 1;
	unsigned char tBuff[3],id,dat;
	Stu_DTC tStuDtc;
	if(pStuSystemMode->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pStuSystemMode->refreshScreenCmd = SCREEN_CMD_NULL;
		pStuSystemMode->keyVal = 0xFF;
		
		hal_Oled_ClearArea(0,20,128,24);		//�����ģʽ�İ�
		hal_Oled_ClearArea(0,12,128,8);		//�屨��̽�����İ�
		hal_Oled_ShowString(16,20,"Alarming",24,1);
		 
		hal_Oled_Refresh();
		displayAlarmFlag = 1;
		timer = 0;
		timer2 = 0;
		
	}
	if(QueueDataLen(RFDRcvMsg))
	{
		QueueDataOut(RFDRcvMsg,&dat);
		if(dat == '#')
		{
			QueueDataOut(RFDRcvMsg,&tBuff[2]);
			QueueDataOut(RFDRcvMsg,&tBuff[1]);
			QueueDataOut(RFDRcvMsg,&tBuff[0]);
			id = DtcMatching(tBuff);		//̽����ƥ��
			if(id != 0xFF)
			{
				//ƥ�䵽̽����
				GetDtcStu(&tStuDtc,id-1);
				
				if(tStuDtc.DTCType == DTC_REMOTE)
				{
					if(tBuff[0] == SENSOR_CODE_REMOTE_DISARM)
					{
						SystemMode_Change(SYSTEM_MODE_DISARM);	//ң�������Ƴ���
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_SOS)
					{
						QueueDataIn(DtcTriggerIDMsg, &id, 1);
					}
				}else if((tBuff[0]==SENSOR_CODE_DOOR_OPEN)
				|| (tBuff[0]==SENSOR_CODE_DOOR_TAMPER)
				|| (tBuff[0]==SENSOR_CODE_PIR)
				|| (tBuff[0]==SENSOR_CODE_PIR_TAMPER))						//����ģʽ�£������ȼ���ߣ�����̽��������������
				{
					QueueDataIn(DtcTriggerIDMsg, &id, 1);
				} 	
			}
		}
	}
	
	
	if(QueueDataLen(DtcTriggerIDMsg) && (!timer))
	{
		QueueDataOut(DtcTriggerIDMsg,&id);
		if(id > 0)
		{
			GetDtcStu(&tStuDtc,id-1); 
			if(tStuDtc.DTCType == DTC_REMOTE)
			{
				//Remote:sos
				hal_Oled_ClearArea(0,12,128,8);		//�屨��̽�����İ�
				hal_Oled_ShowString(34,12,"Remote:",8,1);
				hal_Oled_ShowString(76,12,"sos",8,1);
				mcu_dp_string_update(DPID_ALARM_ACTIVE,"Remote:sos",sizeof("Remote:sos"),STR_GATEWAY_ITSELF_ID,my_strlen(STR_GATEWAY_ITSELF_ID)); //STRING�������ϱ�;	
					
			}
			else if(tStuDtc.DTCType == DTC_DOOR)
			{
			//Door:Zone-001
				hal_Oled_ClearArea(0,12,128,8);		//�屨��̽�����İ�
				hal_Oled_ShowString(25,12,"Door:",8,1);
				hal_Oled_ShowString(55,12,tStuDtc.Name,8,1);	
				mcu_dp_string_update(DPID_ALARM_ACTIVE,tStuDtc.Name,sizeof(tStuDtc.Name),STR_GATEWAY_ITSELF_ID,my_strlen(STR_GATEWAY_ITSELF_ID)); //STRING�������ϱ�
			}
			
			hal_Oled_Refresh();
			timer = 1;
		}
	}

	if(timer)
	{
		timer++;
		if(timer > 500)	//500*10ms=5000ms=5s
		{
			timer = 0;
			hal_Oled_ClearArea(0,12,128,8);		//�屨��̽�����İ�
			hal_Oled_Refresh();
		}
	}
	
	timer2++;
	if(timer2 > 49)	//50*10ms=500ms
	{
		timer2 = 0;
		displayAlarmFlag = !displayAlarmFlag;
		
		 if(displayAlarmFlag)
		 {
			 //ˢ��������Ҫ̫�죬������������������Ҫ��Լ20msʱ��
			 hal_Oled_ShowString(16,20,"Alarming",24,1);	
			 hal_Oled_Refresh();				
			 
		 }else
		 {
			 hal_Oled_ClearArea(0,20,128,24);		//�����ģʽ�İ�
			 hal_Oled_Refresh();
		 }
	}
}



static void gnlMenu_DesktopCBS(void)
{
	unsigned char keys;
	static unsigned short timer = 0;
	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		pModeMenu->keyVal = 0xFF;
		 
		hal_Oled_Clear();
		
	 
		hal_Oled_ShowString(0,0,"N",8,1);
		
		hal_Oled_ShowString(16,20,"Away arm",24,1);
		showSystemTime();
		
		QueueEmpty(RFDRcvMsg);
	

		hal_Oled_Refresh();
		
	}

	timer++;
	if(wifiWorkState == WIFI_CONN_CLOUD)
	{
		if(timer > 6000)
		{
			timer = 0;
		
			get_wifi_st();	//���ͻ�ȡģ������״ָ̬��
		
		}	

		if((timer%200)==0)	//2���ȡʱ��
		{
			mcu_get_system_time();	//ÿ2���ȡһ��ϵͳʱ��
			showSystemTime();		//������Ҫ�ķѼ�ʮms
		}

	}else
	{
		if(timer > 100)
		{
			timer = 0;
		
			get_wifi_st();	//���ͻ�ȡģ������״ָ̬��
		
		}
	}
	if(bwifiWorkState != wifiWorkState)
	{
		bwifiWorkState = wifiWorkState;
		switch(wifiWorkState)
		{
			case SMART_CONFIG_STATE:		//����smartlink����״̬
				
				hal_Oled_ShowString(0,0,"N",8,1);
				LedMsgInput(LED1,LED_BLINK1,1);
				hal_Oled_Refresh();
			break;	
			
			case WIFI_NOT_CONNECTED:		//WIFI���óɹ���δ����·����
				hal_Oled_ShowString(0,0,"W",8,1);
				LedMsgInput(LED1,LED_BLINK3,1);
				hal_Oled_Refresh();
			break;
			case WIFI_CONNECTED:			//WIFI���óɹ�������·����
				hal_Oled_ShowString(0,0,"R",8,1);
				LedMsgInput(LED1,LED_BLINK4,1);
				hal_Oled_Refresh();
			break;
			
			case WIFI_CONN_CLOUD: 			//WIFI�Ѿ��������Ʒ�����
				hal_Oled_ShowString(0,0,"S",8,1);
				LedMsgInput(LED1,LED_LIGHT,1);
				hal_Oled_Refresh();
				 
			break;
		}
	}

	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		 
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
		switch(keys)
		{
			case KEY6_LONG_PRESS:
				pModeMenu = &settingModeMenu[0];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
		}
	}
	pStuSystemMode->action(); //=S_ENArmModeProc();//ִ��ϵͳ����ģʽ


	

}


//�������˵�
static void stgMenu_MainMenuCBS(void)
{
	unsigned char keys;
	unsigned char i;
	unsigned char ClrScreenFlag;
	static stu_mode_menu *pMenu;		//�������浱ǰѡ�еĲ˵�
	static stu_mode_menu *bpMenu=0;		//����������һ�β˵�ѡ���Ҫ����ˢ���ж�
	static unsigned char stgMainMenuSelectedPos=0;	//������¼��ǰѡ�в˵���λ��
	static stu_mode_menu *MHead,*MTail;		//�������ṹ��ָ����Ϊ�������л��˵�ʱ����ҳ����
	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		pMenu = &settingModeMenu[0];
		hal_Oled_Clear();
		
		hal_Oled_ShowString(37,0,pMenu->pModeType,12,1);
		hal_Oled_Refresh();
	
		
		pMenu = &settingModeMenu[1];
		
		MHead = pMenu;			//��¼��ǰ��ʾ�˵���һ��
		MTail = pMenu+3;		//��¼��ǰ��ʾ�˵����һ��,һҳ��ʾ4��		
		bpMenu = 0;
 
		ClrScreenFlag = 1;
		stgMainMenuSelectedPos = 1;
		keys = 0xFF;
 
	}
	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RECOVER)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		//�ָ�֮ǰ��ѡ��λ����ʾ
		hal_Oled_Clear();
		
		hal_Oled_ShowString(37,0,settingModeMenu[0].pModeType,12,1);
		hal_Oled_Refresh();
		keys = 0xFF;
		ClrScreenFlag = 1;
		bpMenu = 0;
	}
	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
		switch(keys)
		{
			case KEY1_CLICK:		//��
				
				if(stgMainMenuSelectedPos ==1)
				{
					MHead = MHead->pLase;
					pMenu = pMenu->pLase;
					MTail = MTail->pLase;
					stgMainMenuSelectedPos = 1;
					ClrScreenFlag = 1;
				}else
				{
					hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//ȡ��ѡ�б��˵���ʾ
					hal_Oled_Refresh();
					pMenu = pMenu->pLase;
					stgMainMenuSelectedPos--;
				}
			break;
			case KEY2_CLICK:		//��
				if(stgMainMenuSelectedPos ==4)
				{
					MHead = MHead->pNext;	
					pMenu = pMenu->pNext;
					MTail = pMenu;
					stgMainMenuSelectedPos = 4;
					ClrScreenFlag = 1;
				}else
				{
					hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//ȡ��ѡ�б��˵���ʾ
					hal_Oled_Refresh();
					pMenu = pMenu->pNext;																			//�л���һ��ѡ��
					stgMainMenuSelectedPos++;
				}

			break;
			
			case KEY5_CLICK_RELEASE:	//ȡ��
				pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;


			case KEY6_CLICK_RELEASE:	//ȷ��
				pModeMenu->pChild = pMenu;
				pModeMenu = pModeMenu->pChild;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
			
		}
	}
	if(bpMenu != pMenu)		//ѡ�в˵��ļ�¼
	{
		bpMenu = pMenu;
		if(ClrScreenFlag)
		{
			
			ClrScreenFlag = 0;
			pMenu = MHead;
			hal_Oled_ClearArea(0,14,128,50);		//����
			hal_Oled_Refresh();
			for(i=1; i<5; i++)
			{
				hal_Oled_ShowString(0,14*i,pMenu->pModeType,8,1);
				hal_Oled_Refresh();
				pMenu = pMenu->pNext;
			} 
			pMenu = bpMenu;
			hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,0);
			hal_Oled_Refresh();
			 
		}else
		{ 
			hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,0);	
			hal_Oled_Refresh();
		}	
			
				 
	}
}

//̽������Բ˵�������
static void stgMenu_LearnSensorCBS(void)
{
	unsigned char keys,dat,tBuff[3];
	static unsigned char PairingComplete = 0;
	static unsigned short Timer = 0;
	Stu_DTC stuTempDevice; 		//��������̽��������ʱ��ʼ��̽������Ϣ
	 
	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		QueueEmpty(RFDRcvMsg);
		//pMenu = &settingModeMenu[0];
		hal_Oled_Clear();
		
		hal_Oled_ShowString(28,0,"Learning DTC",12,1);
		hal_Oled_ShowString(43,28,"Pairing...",8,1);
		
		hal_Oled_Refresh();
		
		keys = 0xFF;
		PairingComplete = 0; 
		Timer = 0;
		 
	}
	
	if(QueueDataLen(RFDRcvMsg) && (!PairingComplete))
	{
		QueueDataOut(RFDRcvMsg,&dat);
		if(dat == '#')
		{
			QueueDataOut(RFDRcvMsg,&tBuff[2]);
			QueueDataOut(RFDRcvMsg,&tBuff[1]);
			QueueDataOut(RFDRcvMsg,&tBuff[0]);
			hal_Oled_ClearArea(0,28,128,36);		//����
			
			
			stuTempDevice.Code[2] = tBuff[2];
			stuTempDevice.Code[1] = tBuff[1];
			stuTempDevice.Code[0] = tBuff[0];
			
			if((stuTempDevice.Code[0]==SENSOR_CODE_DOOR_OPEN) ||
			(stuTempDevice.Code[0]==SENSOR_CODE_DOOR_CLOSE) ||
			(stuTempDevice.Code[0]==SENSOR_CODE_DOOR_TAMPER)||
			(stuTempDevice.Code[0]==SENSOR_CODE_DOOR_LOWPWR))
			{
				//�������Ŵ���
				stuTempDevice.DTCType = DTC_DOOR;
				
			}else if((stuTempDevice.Code[0]==SENSOR_CODE_REMOTE_ENARM) ||
			(stuTempDevice.Code[0]==SENSOR_CODE_REMOTE_DISARM) ||
			(stuTempDevice.Code[0]==SENSOR_CODE_REMOTE_HOMEARM) ||
			(stuTempDevice.Code[0]==SENSOR_CODE_REMOTE_SOS))
			{
				//����ң������
				stuTempDevice.DTCType = DTC_REMOTE;
			}else if((stuTempDevice.Code[0]==SENSOR_CODE_PIR)
			|| (stuTempDevice.Code[0]==SENSOR_CODE_PIR_LOWPWR)
			|| (stuTempDevice.Code[0]==SENSOR_CODE_PIR_TAMPER))
			{
				//���ߺ�����
				stuTempDevice.DTCType = DTC_PIR_MOTION;
			}
			stuTempDevice.ZoneType = ZONE_TYP_1ST;
			if(AddDtc(&stuTempDevice) != 0xFF)
			{
				switch(stuTempDevice.DTCType)
				{
					case DTC_DOOR:
						hal_Oled_ShowString(34,28,"Success!",8,1);
						hal_Oled_ShowString(16,36,"Added door dtc..",8,1);
					break;
					case DTC_REMOTE:
						hal_Oled_ShowString(34,28,"Success!",8,1);
						hal_Oled_ShowString(7,36,"Added remote dtc..",8,1);
					break;
					case DTC_PIR_MOTION:
						hal_Oled_ShowString(34,28,"Success!",8,1);
						hal_Oled_ShowString(19,36,"Added pir dtc..",8,1);
					break;
				}
				
				hal_Oled_Refresh();
				PairingComplete = 1;		//�����ɱ�־
				Timer = 0;		 
			}else
			{
				hal_Oled_ShowString(34,28,"Fail...",8,1);
				hal_Oled_Refresh();
			}
			
		}
		
		
	}
	
	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
		switch(keys)
		{
			case KEY5_CLICK_RELEASE:	//ȡ��
				pModeMenu = pModeMenu->pParent;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
			case KEY5_LONG_PRESS:		//��������
				pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];;;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
		}
	}

	if(PairingComplete)
	{
		Timer++;
		if(Timer > 150)//+1=10ms,10*150=1500ms=1.5s
		{
			Timer = 0;
			pModeMenu = pModeMenu->pParent;			//1.5��ʱ�䵽���Զ����ظ����˵�
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
		}
	}
}


//̽�����б�˵�������
static void stgMenu_DTCListCBS(void)
{
	unsigned char keys;
	unsigned char i,j;
	unsigned char ClrScreenFlag;
	Stu_DTC tStuDtc;
	 
	//static Stu_DTC StuDTCtemp[PARA_DTC_SUM];
	static unsigned char DtcNameBuff[PARA_DTC_SUM][16];
	static stu_mode_menu settingMode_DTCList_Sub_Menu[PARA_DTC_SUM];
	static stu_mode_menu *pMenu;
	static stu_mode_menu *bpMenu=0;		//����������һ�β˵�ѡ���Ҫ����ˢ���ж�
	static unsigned char stgMainMenuSelectedPos=0;	//������¼��ǰѡ�в˵���λ��
	static stu_mode_menu *MHead,*MTail;		//�������ṹ��Ϊ�������л��˵�ʱ����ҳ����
	static unsigned char pMenuIdx=0;		//������ָ̬ʾ�˵��±�,�������������ѧϰ̽������������	
	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		
		pMenuIdx = 0;
		stgMainMenuSelectedPos = 1;
		bpMenu = 0;
		ClrScreenFlag = 1;
		 
		keys = 0xFF;
		
		pMenu = settingMode_DTCList_Sub_Menu;
		
		hal_Oled_Clear();
		hal_Oled_ShowString(40,0,"Dtc List",12,1);
		hal_Oled_Refresh();
		
		//����жϣ�����Ե�̽�������ҳ���
		for(i=0; i<PARA_DTC_SUM; i++)
		{
			if(CheckPresenceofDtc(i))
			{
				GetDtcStu(&tStuDtc,i);
				(pMenu+pMenuIdx)->ID = pMenuIdx;
				//(pMenu+0)->ID = 0;
				
				(pMenu+pMenuIdx)->menuPos = STG_SUB_3_MENU_POS;
				(pMenu+pMenuIdx)->reserved = tStuDtc.ID-1;
				//StuDTCtemp[pMenuIdx].ID = tStuDtc.ID;
				for(j=0; j<16; j++)
				{
				//	StuDTCtemp[pMenuIdx].Name[j] = tStuDtc.Name[j];
					DtcNameBuff[pMenuIdx][j] = tStuDtc.Name[j];
				}
			 
				//(pMenu+pMenuIdx)->pModeType = StuDTCtemp[pMenuIdx].Name;
				
				(pMenu+pMenuIdx)->pModeType = DtcNameBuff[pMenuIdx];
				pMenuIdx++;
			}
		}
		
		if(pMenuIdx != 0)
		{
			//��̽�������ڵ����
			if(pMenuIdx > 1)
			{
				pMenu->pLase =  pMenu+(pMenuIdx-1);
				pMenu->pNext =  pMenu+1;
				pMenu->pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];
				for(i=1; i<pMenuIdx-1; i++)
				{
					(pMenu+i)->pLase =  pMenu+(i-1);
					(pMenu+i)->pNext = pMenu+(i+1);
					(pMenu+i)->pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];
				}
				(pMenu+(pMenuIdx-1))->pLase =  pMenu+(i-1);
				(pMenu+(pMenuIdx-1))->pNext = pMenu;
				(pMenu+(pMenuIdx-1))->pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];
			}else if(pMenuIdx == 1)
			{

				pMenu->pLase = pMenu;
				pMenu->pNext = pMenu;
				pMenu->pParent = &settingModeMenu[STG_MENU_MAIN_SETTING];
			}
		}else
		{
			//û��̽����
			bpMenu = pMenu;
			hal_Oled_ShowString(0,14," No detectors.",8,1);
			hal_Oled_Refresh();
		}

		MHead = pMenu;			//��¼��ǰ��ʾ�˵���һ��
		if(pMenuIdx < 2)
		{
			MTail = pMenu;
		}else if(pMenuIdx < 5)
		{
			MTail = pMenu+(pMenuIdx-1);
		}else
		{
			MTail = pMenu+3;		//��¼��ǰ��ʾ�˵����һ��,һҳ��ʾ4��
		}
		
	}else if(pModeMenu->refreshScreenCmd==SCREEN_CMD_RECOVER)
	{	
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		//�ָ�֮ǰ��ѡ��λ����ʾ
		hal_Oled_Clear();
		hal_Oled_ShowString(40,0,"Dtc List",12,1);
		hal_Oled_Refresh();
		keys = 0xFF;
		ClrScreenFlag = 1;
		bpMenu = 0;
		
	}
	
	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
		switch(keys)
		{
			case KEY1_CLICK_RELEASE:		//��
				if(pMenuIdx < 2)
				{
					//ֻ��һ��̽������������
				}else if(pMenuIdx < 5)
				{
					//ֻ��һҳ��Ҳ����ֻ��4��̽������ʱ��
					if(stgMainMenuSelectedPos ==1)	//�ж��Ƿ�ѡ�е��ǵ�һ��
					{
						//ͷβָ�벻�䣬ֻ�ѵ�ǰ�˵�ָ����һ��
						stgMainMenuSelectedPos = pMenuIdx;
						ClrScreenFlag = 1;
						pMenu = pMenu->pLase;
						
					}else 
					{
						//��������ֱ��ˢ�¾ֲ���ʾ
						hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//ȡ��ѡ�б��˵���ʾ
						hal_Oled_Refresh();
						pMenu = pMenu->pLase;
						stgMainMenuSelectedPos--;
					}
				}else if(pMenuIdx > 4)	//��ǰ̽��������4��
				{
					if(stgMainMenuSelectedPos ==1)	//�ж��Ƿ�ѡ�е��ǵ�һ��
					{
						MHead = MHead->pLase;
						pMenu = pMenu->pLase;
						MTail = MTail->pLase;
						stgMainMenuSelectedPos = 1;
						ClrScreenFlag = 1;
					}else
					{
						hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//ȡ��ѡ�б��˵���ʾ
						hal_Oled_Refresh();
						pMenu = pMenu->pLase;
						stgMainMenuSelectedPos--;
					}
				}
			break;
			
			case KEY2_CLICK_RELEASE:		//�� 
				if(pMenuIdx < 2)
				{
					//ֻ��һ��̽������������
				}else if(pMenuIdx < 5)
				{
					//ֻ��һҳ��Ҳ����ֻ��4��̽������ʱ��
					if(stgMainMenuSelectedPos ==pMenuIdx)	//�ж��Ƿ�ѡ�е��ǵ�4��
					{
						//ͷβָ�벻�䣬ֻ�ѵ�ǰ�˵�ָ���¸�
						pMenu = pMenu->pNext;
						stgMainMenuSelectedPos = 1;
						ClrScreenFlag = 1;
						
					}else 
					{
						//��������ֱ��ˢ�¾ֲ���ʾ
						hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//ȡ��ѡ�б��˵���ʾ
						hal_Oled_Refresh();
						pMenu = pMenu->pNext;																			//�л���һ��ѡ��
						stgMainMenuSelectedPos++;
					}
				}else if(pMenuIdx > 4)	//��ǰ̽��������4��
				{
					if(stgMainMenuSelectedPos ==4)	//�ж��Ƿ�ѡ�е��ǵ�һ��
					{
						MHead = MHead->pNext;	
						pMenu = pMenu->pNext;
						MTail = pMenu;
						stgMainMenuSelectedPos = 4;
						ClrScreenFlag = 1;
					}else
					{
						hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//ȡ��ѡ�б��˵���ʾ
						hal_Oled_Refresh();
						pMenu = pMenu->pNext;																			//�л���һ��ѡ��
						stgMainMenuSelectedPos++;
					}
				}
			break;
			
			case KEY6_CLICK_RELEASE:			//ȷ��
				if(pMenuIdx>0)
				{
					pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN]; 
					pModeMenu->reserved = pMenu->reserved;	//�������ڴ��ݺ���Ҫ�鿴���޸ġ�ɾ��̽������ID��
					pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
				}
			break;
			
			case KEY5_CLICK_RELEASE:	//ȡ��
				pModeMenu = pModeMenu->pParent;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
			break;
			case KEY5_LONG_PRESS:		//��������
				pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
			
		}
	}
	
	if(bpMenu != pMenu)
	{
		bpMenu = pMenu;
		if(ClrScreenFlag)
		{
			ClrScreenFlag = 0;
			pMenu = MHead;
			hal_Oled_ClearArea(0,14,128,50);		//����
			hal_Oled_Refresh();
			if(pMenuIdx <4)
			{
				for(i=0; i<pMenuIdx; i++)
				{
					hal_Oled_ShowString(0,14*(i+1),pMenu->pModeType,8,1);
					hal_Oled_Refresh();
					pMenu = pMenu->pNext;
				}
			}else
			{
				for(i=1; i<5; i++)
				{
					hal_Oled_ShowString(0,14*i,pMenu->pModeType,8,1);
					hal_Oled_Refresh();
					pMenu = pMenu->pNext;
				} 
			}
			pMenu = bpMenu;
			hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,0);
			hal_Oled_Refresh();
			 
		}else
		{ 
			hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,0);	
			hal_Oled_Refresh();
			
		}	
	} 

}

static void stgMenu_dl_ReviewMainCBS(void)
{
	unsigned char keys = 0xFF;
	unsigned char i,ClrScreenFlag=0;
	Stu_DTC tStuDtc;
	 
	static stu_mode_menu *MHead;		//�������ṹ����������ʾ��ҳ���ͷ��β
	static stu_mode_menu *pMenu,*bpMenu=0;	//������¼��ǰѡ�еĲ˵�
	
	static unsigned char stgMainMenuSelectedPos=0;				 

	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{	//ִ��ҳ���л�ʱ��Ļˢ����ʾ 
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		
		if(CheckPresenceofDtc(pModeMenu->reserved))
		{
			GetDtcStu(&tStuDtc,pModeMenu->reserved);	//��ȡ̽������Ϣ
		}
		 
		 
		hal_Oled_Clear();
		hal_Oled_ShowString(40,0,tStuDtc.Name,12,1);
		
		hal_Oled_Refresh();
		

		pMenu = &DL_ZX_Review[1];
		stgMainMenuSelectedPos = 1;
		MHead = pMenu;			//��¼��ǰ��ʾ�˵���һ��
		ClrScreenFlag = 1;
		bpMenu = 0;
		keys = 0xFF;
	}else if(pModeMenu->refreshScreenCmd==SCREEN_CMD_RECOVER)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		//�ָ�֮ǰ��ѡ��λ����ʾ
		if(CheckPresenceofDtc(pModeMenu->reserved))
		{
			GetDtcStu(&tStuDtc,pModeMenu->reserved);	//��ȡ̽������Ϣ
		}
		 
		 
		hal_Oled_Clear();
		hal_Oled_ShowString(40,0,tStuDtc.Name,12,1);
		
		hal_Oled_Refresh();
		keys = 0xFF;
		ClrScreenFlag = 1;
		bpMenu = 0;
	}
	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
		switch(keys)
		{
			case KEY1_CLICK:		//��
				if(stgMainMenuSelectedPos ==1)
				{
					pMenu = pMenu->pLase;
					stgMainMenuSelectedPos = 3;
					ClrScreenFlag = 1;
				}else
				{
					hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//ȡ��ѡ�б��˵���ʾ
					hal_Oled_Refresh();
					pMenu = pMenu->pLase;
					stgMainMenuSelectedPos--;
				}
			break;
			case KEY2_CLICK:		//��
				if(stgMainMenuSelectedPos ==3)
				{
					pMenu = pMenu->pNext;
					stgMainMenuSelectedPos = 1;
					ClrScreenFlag = 1;
				}else
				{
					hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//ȡ��ѡ�б��˵���ʾ
					hal_Oled_Refresh();
					pMenu = pMenu->pNext;																			//�л���һ��ѡ��
					stgMainMenuSelectedPos++;
				}
			
				
			break;
			
			case KEY6_CLICK_RELEASE:
				pMenu->reserved = pModeMenu->reserved;	//������ָ��̽�����ṹ�������±괫����ȥ
				pModeMenu = pMenu;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
			
			case KEY5_CLICK_RELEASE:
				pModeMenu = &settingModeMenu[STG_MENU_DTC_LIST];	//���ﲻ��ֱ�ӷ��ظ�������Ϊ̽���������Ƕ�̬�ģ�����û��ʼ��
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
			break;
			case KEY5_LONG_PRESS:
				pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
		}
	}
	
	if(bpMenu != pMenu)
	{
		bpMenu = pMenu;
		if(ClrScreenFlag)
		{
			ClrScreenFlag = 0;
			pMenu = MHead;
			hal_Oled_ClearArea(0,14,128,50);		//����
			hal_Oled_Refresh();
			for(i=0; i<3; i++)
			{
				hal_Oled_ShowString(0,14*(i+1),pMenu->pModeType,8,1);
				hal_Oled_Refresh();
				pMenu = pMenu->pNext;
			} 
			pMenu = bpMenu;
			hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,0);
			hal_Oled_Refresh();
			 
		}else
		{ 
			hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,0);	
			hal_Oled_Refresh();
		}		 
	}
}

static void stgMenu_dl_ReviewCBS(void)
{
	unsigned char keys = 0xFF;
	Stu_DTC tStuDtc;
	unsigned char temp[6];
	
	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{	//ִ��ҳ���л�ʱ��Ļˢ����ʾ 
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		 
		if(CheckPresenceofDtc(pModeMenu->reserved))
		{
			GetDtcStu(&tStuDtc,pModeMenu->reserved);
		}
		 
 
		hal_Oled_Clear();
		hal_Oled_ShowString(40,0,pModeMenu->pModeType,12,1);
		
		hal_Oled_ShowString(0,16,"<Name>: ",8,1); 
		//<Name>: 8���ַ���8*6=48
		hal_Oled_ShowString(48,16,tStuDtc.Name,8,1);
		
		hal_Oled_ShowString(0,28,"<Type>: ",8,1);
		
		if(tStuDtc.DTCType == DTC_DOOR)
		{
			hal_Oled_ShowString(48,28,"door dtc",8,1);
		}else if(tStuDtc.DTCType == DTC_PIR_MOTION)
		{
			hal_Oled_ShowString(48,28,"pir dtc",8,1);
		}else if(tStuDtc.DTCType == DTC_REMOTE)
		{
			hal_Oled_ShowString(48,28,"remote",8,1);
		}
		
		hal_Oled_ShowString(0,40,"<ZoneType>: ",8,1);
		
		if(tStuDtc.ZoneType == ZONE_TYP_24HOURS)
		{
			hal_Oled_ShowString(72,40,"24 hrs",8,1);
		}else if(tStuDtc.ZoneType == ZONE_TYP_1ST)
		{
			hal_Oled_ShowString(72,40,"1ST",8,1);
		}else if(tStuDtc.ZoneType == ZONE_TYP_2ND)
		{
			hal_Oled_ShowString(72,40,"2ND",8,1);
		}
		
		hal_Oled_ShowString(0,52,"<RFCode>: ",8,1);
		HexToAscii(tStuDtc.Code,temp,3);
		
		hal_Oled_ShowChar(60,52,temp[4],8,1);
		hal_Oled_ShowChar(66,52,temp[5],8,1);
		
		hal_Oled_ShowChar(72,52,' ',8,1);
		
		hal_Oled_ShowChar(80,52,temp[2],8,1);
		hal_Oled_ShowChar(86,52,temp[3],8,1);
		
		hal_Oled_Refresh();
		
	}
	
	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
		switch(keys)
		{
			case KEY5_CLICK_RELEASE:
				pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
				 
			break;
			case KEY6_CLICK_RELEASE:
				pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
			break;
			case KEY5_LONG_PRESS:

				pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
			
			
		}
	}

}

static void HexToAscii(unsigned char *pHex, unsigned char *pAscii, int nLen)
{
    unsigned char Nibble[2];
    unsigned int i,j;
    for (i = 0; i < nLen; i++){
        Nibble[0] = (pHex[i] & 0xF0) >> 4;
        Nibble[1] = pHex[i] & 0x0F;
        for (j = 0; j < 2; j++){
            if (Nibble[j] < 10){            
                Nibble[j] += 0x30;
            }
            else{
                if (Nibble[j] < 16)
                    Nibble[j] = Nibble[j] - 10 + 'A';
            }
            *pAscii++ = Nibble[j];
        }             
    }          
}


static void stgMenu_dl_EditCBS(void)
{
	unsigned char keys = 0xFF;
	static Stu_DTC tStuDtc;
	static unsigned short timer = 0;
	static unsigned char editComplete = 0;

	static unsigned char *pDL_ZX_Edit_DTCType_Val[DTC_TYP_SUM] =
	{
		"door dtc",
		"pir dtc",
		"remote",
	};

	static unsigned char *pDL_ZX_Edit_ZoneType_Val[STG_DEV_AT_SUM] =
	{
		"24 hrs",
		"1ST",
		"2ND",
	};
	static unsigned char stgMainMenuSelectedPos=0;
	static unsigned char setValue = DTC_DOOR;

	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{	//ִ��ҳ���л�ʱ��Ļˢ����ʾ 
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		stgMainMenuSelectedPos = 0; 
		if(CheckPresenceofDtc(pModeMenu->reserved))
		{
			GetDtcStu(&tStuDtc,pModeMenu->reserved);
		}
		 
		stgMainMenuSelectedPos = 0;
		setValue = tStuDtc.DTCType;
		hal_Oled_Clear();
		hal_Oled_ShowString(40,0,pModeMenu->pModeType,12,1);
		hal_Oled_Refresh();
		
		hal_Oled_ShowString(0,16,"<Name>: ",8,1); 
		hal_Oled_ShowString(48,16,tStuDtc.Name,8,1);
		
		hal_Oled_ShowString(0,28,"<Type>: ",8,1);
		hal_Oled_Refresh();
		
		if(tStuDtc.DTCType == DTC_DOOR)
		{
			hal_Oled_ShowString(48,28,"door dtc",8,0);
		}else if(tStuDtc.DTCType == DTC_PIR_MOTION)
		{
			hal_Oled_ShowString(48,28,"pir dtc",8,0);
		}else if(tStuDtc.DTCType == DTC_REMOTE)
		{
			hal_Oled_ShowString(48,28,"remote",8,0);
		}
		hal_Oled_Refresh();
		
		hal_Oled_ShowString(0,40,"<ZoneType>: ",8,1);
		hal_Oled_Refresh();
		
		if(tStuDtc.ZoneType == ZONE_TYP_24HOURS)
		{
			hal_Oled_ShowString(72,40,"24 hrs",8,1);
		}else if(tStuDtc.ZoneType == ZONE_TYP_1ST)
		{
			hal_Oled_ShowString(72,40,"1ST",8,1);
		}else if(tStuDtc.ZoneType == ZONE_TYP_2ND)
		{
			hal_Oled_ShowString(72,40,"2ND",8,1);
		}
		
		hal_Oled_Refresh();
		 
		editComplete = 0;
		timer = 0;
	}
	

	if(editComplete == 0)
	{
		if(pModeMenu->keyVal != 0xff)
		{
			keys = pModeMenu->keyVal;
			pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
			if(keys == KEY3_CLICK_RELEASE)
			{
				if(stgMainMenuSelectedPos == 0)		
				{
					//����Type
					if(setValue == DTC_DOOR)
					{
						setValue = DTC_TYP_SUM-1;
					}else
					{
						setValue--;
					}
					tStuDtc.DTCType = (DTC_TYPE_TYPEDEF)setValue;			//����̽��������
					hal_Oled_ClearArea(48,28,80,8);		//����
					hal_Oled_ShowString(48,28,pDL_ZX_Edit_DTCType_Val[setValue],8,0);
					hal_Oled_Refresh();

				}else if(stgMainMenuSelectedPos == 1)
				{
					//����Zone type
					if(setValue == ZONE_TYP_24HOURS)
					{
						setValue = STG_DEV_AT_SUM-1;
					}else
					{
						setValue--;
					}
					tStuDtc.ZoneType = (ZONE_TYPED_TYPEDEF)setValue;			////����̽��������
					hal_Oled_ClearArea(72,40,56,8);		//����
					hal_Oled_ShowString(72,40,pDL_ZX_Edit_ZoneType_Val[setValue],8,0);
					hal_Oled_Refresh();
				}
			}else if(keys == KEY4_CLICK_RELEASE)
			{
				if(stgMainMenuSelectedPos == 0)
				{
					//����Type
					if(setValue == (DTC_TYP_SUM-1))
					{
						setValue = 0;
					}else
					{
						setValue++;
					}
					tStuDtc.DTCType = (DTC_TYPE_TYPEDEF)setValue;			//����̽��������
					hal_Oled_ClearArea(48,28,80,8);		//����
					hal_Oled_ShowString(48,28,pDL_ZX_Edit_DTCType_Val[setValue],8,0);
					hal_Oled_Refresh();
				}else if(stgMainMenuSelectedPos == 1)
				{
					//����Zone type
					if(setValue == (STG_DEV_AT_SUM-1))
					{
						setValue = 0;
					}else
					{
						setValue++;
					}
					tStuDtc.ZoneType = (ZONE_TYPED_TYPEDEF)setValue;			////����̽��������
					hal_Oled_ClearArea(72,40,56,8);		//����
					hal_Oled_ShowString(72,40,pDL_ZX_Edit_ZoneType_Val[setValue],8,0);
					hal_Oled_Refresh();
				}
				
			}else if((keys==KEY1_CLICK_RELEASE) || (keys==KEY2_CLICK_RELEASE))
			{
				if(stgMainMenuSelectedPos == 0)
				{
					stgMainMenuSelectedPos = 1;
					setValue = tStuDtc.ZoneType;
					hal_Oled_ClearArea(48,28,80,8);		//����
					hal_Oled_ShowString(48,28,pDL_ZX_Edit_DTCType_Val[tStuDtc.DTCType],8,1);	//�ָ�̽��������δѡ����ʾ

					hal_Oled_ClearArea(72,40,56,8);		//����

					hal_Oled_ShowString(72,40,pDL_ZX_Edit_ZoneType_Val[setValue],8,0);			//�л�ѡ�в˵�����������
					hal_Oled_Refresh();
				}else
				{
					stgMainMenuSelectedPos = 0;
					setValue = tStuDtc.DTCType;
					hal_Oled_ClearArea(48,28,80,8);		//����
					hal_Oled_ShowString(48,28,pDL_ZX_Edit_DTCType_Val[setValue],8,0);		//�л�ѡ�в˵���̽��������
					
					
					hal_Oled_ClearArea(72,40,56,8);		//����
					hal_Oled_ShowString(72,40,pDL_ZX_Edit_ZoneType_Val[tStuDtc.ZoneType],8,1);	//�ָ�̽������������δѡ����ʾ
					
					hal_Oled_Refresh();
				}
			}else if(keys == KEY5_CLICK_RELEASE)
			{
				pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
					
			}else if(keys == KEY6_CLICK_RELEASE)
			{
				timer = 0;
				SetDtcAbt(tStuDtc.ID-1,&tStuDtc);		//����̽�������ԣ���д��EEPROM����
				editComplete = 1;
				hal_Oled_Clear();
				hal_Oled_ShowString(16,20,"Update..",24,1);
				hal_Oled_Refresh();	 
			}else if(keys == KEY5_LONG_PRESS)
			{
				pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			}
			
			
		}
	}
	if(editComplete)
	{
		timer++;
		if(timer > 150)		//1.5���Զ��˳�
		{
			timer = 0;
			editComplete = 0;
			pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
		}
	}

}

static void stgMenu_dl_DeleteCBS(void)
{
	unsigned char keys = 0xFF;
	static Stu_DTC tStuDtc;
	static unsigned short timer = 0;
	static unsigned char DelComplete = 0;
	
	static unsigned char stgMainMenuSelectedPos=0;

	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{	//ִ��ҳ���л�ʱ��Ļˢ����ʾ 
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		stgMainMenuSelectedPos = 0; 
		if(CheckPresenceofDtc(pModeMenu->reserved))
		{
			GetDtcStu(&tStuDtc,pModeMenu->reserved);
		}
		 
		stgMainMenuSelectedPos = 0;
		
		hal_Oled_Clear();
		hal_Oled_ShowString(46,0,pModeMenu->pModeType,12,1);
		
		//del zone-001
		hal_Oled_ShowString(28,14,"Del ",12,1); 
		hal_Oled_ShowString(52,14,tStuDtc.Name,12,1); 
		
		
		hal_Oled_ShowString(25,28,"Are you sure?",12,1); 
		//yes   no
		hal_Oled_ShowString(40,48,"Yes",12,1); 
		hal_Oled_ShowString(88,48,"No",12,0); 
		
		
		hal_Oled_Refresh();
		
 
		 
		DelComplete = 0;
		timer = 0;
	}
	
	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
		
		if((keys == KEY3_CLICK_RELEASE) || (keys == KEY4_CLICK_RELEASE))
		{
			if(stgMainMenuSelectedPos == 0)		
			{
				stgMainMenuSelectedPos = 1;
				hal_Oled_ClearArea(40,48,88,16);		//����
				hal_Oled_ShowString(40,48,"Yes",12,0); 
				hal_Oled_ShowString(88,48,"No",12,1); 
				hal_Oled_Refresh();

			}else if(stgMainMenuSelectedPos == 1)
			{
				stgMainMenuSelectedPos = 0;
				hal_Oled_ClearArea(40,48,88,16);		//����
				hal_Oled_ShowString(40,48,"Yes",12,1); 
				hal_Oled_ShowString(88,48,"No",12,0); 
				hal_Oled_Refresh();
 
			}
		}else if(keys == KEY6_CLICK_RELEASE)
		{
			if(stgMainMenuSelectedPos)
			{
				//ȷ��ɾ��
				DelComplete = 1;
				timer = 0;
				tStuDtc.Mark = 0;
				SetDtcAbt(tStuDtc.ID-1,&tStuDtc);		//����̽�������ԣ���д��EEPROM����
				hal_Oled_Clear();
				hal_Oled_ShowString(16,20,"Update..",24,1);
				hal_Oled_Refresh();	
			}else
			{
				pModeMenu = pModeMenu->pParent;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER; 
			}
		}else if(keys == KEY5_CLICK_RELEASE)
		{
			pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
		}else if(keys == KEY5_LONG_PRESS)
		{
			pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];;
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
		}
	}
	if(DelComplete)
	{
		timer++;
		if(timer > 150)		//1.5���Զ��˳�
		{
			timer = 0;
			DelComplete = 0;
			pModeMenu = &settingModeMenu[STG_MENU_DTC_LIST];
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
		}
	}
}

//wifi�����˵�������
static void stgMenu_WifiCBS(void)
{
	static unsigned char APStep = 0;
	static unsigned char ConnectSuccess = 0;
	static unsigned char stgMainMenuSelectedPos = 0;
	static unsigned short timer = 0;
	unsigned char keys;
	unsigned char wifiWorkState = 0;


	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
	
		hal_Oled_Clear();	 
		hal_Oled_ShowString(52,0,"Wifi",12,1);
		
		 
		hal_Oled_ShowString(0,20,"Are you sure to",8,1); 
		hal_Oled_ShowString(0,30,"enter ap mode?",8,1);
		 
		
		//yes   no
		hal_Oled_ShowString(40,48,"Yes",12,1); 
		hal_Oled_ShowString(88,48,"No",12,0); 
		
		hal_Oled_Refresh();
		
		keys = 0xFF;
		APStep = 0;
		ConnectSuccess = 0;
		timer = 0;

		stgMainMenuSelectedPos = 0;
	}
	
	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
		
		if((keys == KEY3_CLICK_RELEASE) 
		|| (keys == KEY4_CLICK_RELEASE))
		{
			if(stgMainMenuSelectedPos == 0)		
			{
				stgMainMenuSelectedPos = 1;
				hal_Oled_ClearArea(40,48,88,16);		//����
				hal_Oled_ShowString(40,48,"Yes",12,0); 
				hal_Oled_ShowString(88,48,"No",12,1); 

				hal_Oled_Refresh();

			}else if(stgMainMenuSelectedPos == 1)
			{
				stgMainMenuSelectedPos = 0;
				hal_Oled_ClearArea(40,48,88,16);		//����
				hal_Oled_ShowString(40,48,"Yes",12,1); 
				hal_Oled_ShowString(88,48,"No",12,0); 
				hal_Oled_Refresh();
 
			}
		}
		else if((keys == KEY6_CLICK_RELEASE)
		&& (!APStep)
		&& (!ConnectSuccess))
		{
			if(stgMainMenuSelectedPos)
			{
				APStep = 1;
				mcu_set_wifi_mode(1);		//��wifi����AP����ģʽ
				hal_Oled_ClearArea(0,20,128,44);		//����
				hal_Oled_ShowString(16,30,"Please wait..",8,1);
				hal_Oled_Refresh();	
			}else
			{
				pModeMenu = pModeMenu->pParent;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
			}
		}
		else if(keys == KEY5_CLICK_RELEASE)
		{
			pModeMenu = pModeMenu->pParent;
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
		}else if(keys == KEY5_LONG_PRESS)
		{
			pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];;
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
		}
		 
	}

	if(APStep)
	{
		wifiWorkState = mcu_get_wifi_work_state();
	}
	
	if(APStep == 1)
	{
		if(wifiWorkState == AP_STATE)
		{
			hal_Oled_ClearArea(0,20,128,44);		//����
			hal_Oled_ShowString(0,30,"Enter ap mode.",8,1);
			hal_Oled_Refresh();
			LedMsgInput(LED1,LED_BLINK1,1);
			APStep = 2;
		}
	}else if(APStep == 2)
	{
		if(wifiWorkState == WIFI_NOT_CONNECTED)
		{
			hal_Oled_ClearArea(0,20,128,44);		//����
			hal_Oled_ShowString(0,30,"Connect to wifi ok.",8,1);
			hal_Oled_Refresh();
			LedMsgInput(LED1,LED_BLINK3,1);
			APStep = 3;	
		}
	}else if(APStep == 3)
	{
		if(wifiWorkState == WIFI_CONNECTED)
		{
			hal_Oled_ClearArea(0,20,128,44);		//����
			hal_Oled_ShowString(0,30,"Connect to router ok.",8,1);
			hal_Oled_Refresh();
			LedMsgInput(LED1,LED_BLINK4,1);
			APStep = 4;
		}
	}else if(APStep == 4)
	{
		if(wifiWorkState == WIFI_CONN_CLOUD)
		{
			hal_Oled_ClearArea(0,20,128,44);		//����
			hal_Oled_ShowString(0,30,"Connect to server ok.",8,1);
			hal_Oled_ShowString(0,40,"Connect success!",8,1);
			hal_Oled_Refresh();
			LedMsgInput(LED1,LED_LIGHT,1);
			APStep = 0;
			ConnectSuccess = 1;
			timer = 0;
		}
	}
	
	if(ConnectSuccess)
	{
		timer++;
		if(timer >200)
		{
			timer = 0;
			ConnectSuccess = 0;
			 
			pModeMenu = pModeMenu->pParent;
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
		}
	}
}

//�豸��Ϣ�˵�������
static void stgMenu_MachineInfoCBS(void)
{
	unsigned char keys;
 
	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
	
		hal_Oled_Clear();	 //mac info
		hal_Oled_ShowString(40,0,"Mac info",12,1);
		
		 
		//v1.7
		hal_Oled_ShowString(0,16,"<mcu ver>: ",12,1);
		hal_Oled_ShowString(66,16,pMcuVersions,12,1);
		
		hal_Oled_ShowString(0,32,"<hard ver>: ",12,1);
		hal_Oled_ShowString(72,32,pHardVersions,12,1);
		hal_Oled_Refresh();
		
		keys = 0xFF;
		 
	}
	
	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
		if(keys == KEY5_CLICK_RELEASE)
		{
			pModeMenu = pModeMenu->pParent;
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
		}else if(keys == KEY5_LONG_PRESS)
		{
			pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];;
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
		}
	}
}

//�ָ��������ò˵�������
static void stgMenu_FactorySettingsCBS(void)
{
	unsigned char keys = 0xFF;

	static unsigned short timer = 0;
	static unsigned char Complete = 0;
	
	static unsigned char stgMainMenuSelectedPos=0;


	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{	//ִ��ҳ���л�ʱ��Ļˢ����ʾ 
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		stgMainMenuSelectedPos = 0; 
	
	
		hal_Oled_Clear();
		hal_Oled_ShowString(19,0,"Default setting",12,1);
		
		
		hal_Oled_ShowString(25,28,"Are you sure?",12,1); 
		//yes   no
		hal_Oled_ShowString(40,48,"Yes",12,1); 
		hal_Oled_ShowString(88,48,"No",12,0); 
		
		
		hal_Oled_Refresh();
		
 
		 
		Complete = 0;
		timer = 0;
	}
	
	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
		if(!Complete)
		{
			if((keys == KEY3_CLICK_RELEASE) || (keys == KEY4_CLICK_RELEASE))
			{
				if(stgMainMenuSelectedPos == 0)		
				{
					stgMainMenuSelectedPos = 1;
					hal_Oled_ClearArea(40,48,88,16);		//����
					hal_Oled_ShowString(40,48,"Yes",12,0); 
					hal_Oled_ShowString(88,48,"No",12,1); 
					hal_Oled_Refresh();

				}else if(stgMainMenuSelectedPos == 1)
				{
					stgMainMenuSelectedPos = 0;
					hal_Oled_ClearArea(40,48,88,16);		//����
					hal_Oled_ShowString(40,48,"Yes",12,1); 
					hal_Oled_ShowString(88,48,"No",12,0); 
					hal_Oled_Refresh();
	 
				}
			}else if(keys == KEY6_CLICK_RELEASE)
			{
				if(stgMainMenuSelectedPos)
				{
					//ȷ�� 
					Complete = 1;
					timer = 0;
					FactoryReset();		//���ø�λEEPROM���ݺ���
					hal_Oled_Clear();
					hal_Oled_ShowString(16,20,"Update..",24,1);
					hal_Oled_Refresh();	
				}else
				{
					pModeMenu = pModeMenu->pParent;
					pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER; 
				}
			}else if(keys == KEY5_CLICK_RELEASE)
			{
				pModeMenu = pModeMenu->pParent;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER; 
			}else if(keys == KEY5_LONG_PRESS)
			{
				pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			}
		}
	}
	if(Complete)
	{
		timer++;
		if(timer > 150)		//1.5���Զ��˳�
		{
			timer = 0;
			Complete = 0;
			pModeMenu = pModeMenu->pParent;
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
		}
	}
}

//-----------------������ص�������------------------------
 
//�����ص�����
static void KeyEventHandle(KEY_VALUE_TYPEDEF keys)
{
	if(!ScreenState)
	{
		ScreeControl(1);
	}else
	{
		pModeMenu->keyVal = keys;
		if((pModeMenu->menuPos!=DESKTOP_MENU_POS) 
			&&(pModeMenu->menuPos!=STG_WIFI_MENU_POS))
			{
				SetupMenuTimeOutCnt = 0;
			}
		PutoutScreenTiemr = 0;
	}


/////////////////////////////////////////////
	// if(keys == KEY1_CLICK_RELEASE)
	// {
	// 	LedMsgInput(LED1,LED_LIGHT_100MS,1);
	// }
		
/////////////////////////////////////////////
}


//RFD���ջص�����
static void RfdRcvHandle(unsigned char *pBuff)
{
	unsigned char temp;
	temp = '#';
	QueueDataIn(RFDRcvMsg, &temp, 1);
	QueueDataIn(RFDRcvMsg, &pBuff[0], 3);
	
	 
}

//�Ʒ��������ݽ��ջص�����
static void ServerEventHandle(WIFI_MSG_TYPE type,unsigned char *pData)
{
	switch(type)
	{
		case WF_HOST_STATE:	//����״̬
			if(*pData == 0)
			{
				SystemMode_Change(SYSTEM_MODE_ENARM);
			}else if(*pData == 1)
			{
				SystemMode_Change(SYSTEM_MODE_HOMEARM);
			}else if(*pData == 2)
			{
				SystemMode_Change(SYSTEM_MODE_DISARM);
			}
		break;
		case WF_TIME:
		/*
		time[0]Ϊ�Ƿ��ȡʱ��ɹ���־��Ϊ 0 ��ʾʧ�ܣ�Ϊ 1��ʾ�ɹ�
		time[1] Ϊ �� �� , 0x00 �� ʾ2000 ��
		time[2]Ϊ�·ݣ��� 1 ��ʼ��12 ����
		time[3]Ϊ���ڣ��� 1 ��ʼ��31 ����
		time[4]Ϊʱ�ӣ��� 0 ��ʼ��23 ����
		time[5]Ϊ���ӣ��� 0 ��ʼ��59 ����
		time[6]Ϊ���ӣ��� 0 ��ʼ��59 ����
		time[7]Ϊ���ڣ��� 1 ��ʼ�� 7 ������1��������һ
		*/
		//2021-04-09 wes 17:22
			stuSystemtime.year=2000+pData[1];
			stuSystemtime.mon=pData[2];
			stuSystemtime.day=pData[3];
			stuSystemtime.hour=pData[4];
			stuSystemtime.min=pData[5];
			stuSystemtime.sec=pData[6];
			stuSystemtime.week=pData[7];
			
		break;
		case WF_CONNECT_STATE:
			wifiWorkState = *pData;
						/**
			 * @brief  ��ȡ WIFI ״̬���
			 * @param[in] {result} ָʾ WIFI ����״̬
			 * @ref         0x00: wifi״̬ 1 smartconfig ����״̬
			 * @ref         0x01: wifi״̬ 2 AP ����״̬
			 * @ref         0x02: wifi״̬ 3 WIFI �����õ�δ����·����
			 * @ref         0x03: wifi״̬ 4 WIFI ������������·����
			 * @ref         0x04: wifi״̬ 5 ������·���������ӵ��ƶ�
			 * @ref         0x05: wifi״̬ 6 WIFI �豸���ڵ͹���ģʽ
			 * @ref         0x06: wifi״̬ 7 Smartconfig&AP����״̬
			 */	
		break;

	}
}

void mcu_all_dp_update()
{

  mcu_dp_enum_update(DPID_MASTER_MODE,pStuSystemMode->ID,STR_GATEWAY_ITSELF_ID,my_strlen(STR_GATEWAY_ITSELF_ID)); //ö���������ϱ�;
  mcu_dp_string_update(DPID_ALARM_ACTIVE," ",sizeof(" "),STR_GATEWAY_ITSELF_ID,my_strlen(STR_GATEWAY_ITSELF_ID)); //STRING�������ϱ�;

}

static void ScreeControl(unsigned char cmd)
{
	if(cmd)
	{
		if(!ScreenState)
		{
			ScreenState = 1;
			hal_Oled_DisPlay_On();
			PutoutScreenTiemr = 0;
		}
	}else
	{
		if(ScreenState)
		{
			ScreenState = 0;
			hal_Oled_DisPlay_Off();
			PutoutScreenTiemr = 0;
		}
	}
}



