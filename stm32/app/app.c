
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


Queue8 RFDRcvMsg;	//RFD接收队列
Queue8 DtcTriggerIDMsg;	//触发的探测器ID队列

LINK_STATE_TYPEDEF link_state;

stu_system_time stuSystemtime;		//系统时间
stu_mode_menu *pModeMenu;		//系统当前菜单
stu_system_mode *pStuSystemMode;		//当前系统防御模式
unsigned char wifiWorkState,bwifiWorkState;	//WiFi模组工作状态

unsigned short SetupMenuTimeOutCnt;

unsigned short PutoutScreenTiemr;		//关屏时间
unsigned char ScreenState;				//屏幕状态,0关屏，1开屏


unsigned char *pMcuVersions = "v2.8";		//这样子写的字符串只能赋值给指针
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

//初始化桌面菜单
stu_mode_menu generalModeMenu[GNL_MENU_SUM] =
{
	{GNL_MENU_DESKTOP,DESKTOP_MENU_POS,"Desktop",gnlMenu_DesktopCBS,1,0,0xFF,0,0,0,0},
	
};	

//初始化设置菜单
stu_mode_menu settingModeMenu[STG_MENU_SUM] = 
{
	{STG_MENU_MAIN_SETTING,STG_MENU_POS,"Main Menu",stgMenu_MainMenuCBS,1,0,0xFF,0,0,0,0},		//设置主页面
	{STG_MENU_LEARNING_SENSOR,STG_SUB_2_MENU_POS,"1. Learning Dtc",stgMenu_LearnSensorCBS,1,0,0xFF,0,0,0,0},	//探测器学习
	{STG_MENU_DTC_LIST,STG_SUB_2_MENU_POS,"2. Dtc List",stgMenu_DTCListCBS,1,0,0xFF,0,0,0,0},			//防区


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
		case 1:	//星期1
			hal_Oled_ShowString(106,54,"Mon",8,1);
		break;
		case 2:	//星期2
			hal_Oled_ShowString(106,54,"Tue",8,1);
		break;
		case 3:	//星期3
			hal_Oled_ShowString(106,54,"Wed",8,1);
		break;
		case 4:	//星期4
			hal_Oled_ShowString(106,54,"Thu",8,1);
		break;
		case 5:	//星期5
			hal_Oled_ShowString(106,54,"Fir",8,1);
		break;
		case 6:	//星期6
			hal_Oled_ShowString(106,54,"Sat",8,1);
		break;
		case 7:	//星期7
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
			pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];	//设置上电显示的菜单界面为桌面显示
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;	//更新刷新界面标志，进入界面后刷新全界面UI

		}
	}
			
	if((pStuSystemMode->ID!=SYSTEM_MODE_ALARM)
	&& (pModeMenu->menuPos!=STG_WIFI_MENU_POS))
	{
		PutoutScreenTiemr++;
		if(PutoutScreenTiemr > PUTOUT_SCREEN_PERIOD)
		{
			PutoutScreenTiemr = 0;
			
			//30秒没任何操作自动熄屏
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
	//主设置菜单初始化,把菜单列表形成链表形式，方便调用
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
	

	pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];	//设置上电显示的菜单界面为桌面显示
	pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;	//更新刷新界面标志，进入界面后刷新全界面UI

}

//切换系统防御模式
static void SystemMode_Change(SYSTEMMODE_TYPEDEF sysMode)
{
	if(sysMode < SYSTEM_MODE_SUM)	//传入的形参(模式)是否正确
	{
		ScreeControl(1);
		PutoutScreenTiemr = 0;
		pStuSystemMode = &stu_Sysmode[sysMode];
		pStuSystemMode->refreshScreenCmd = SCREEN_CMD_RESET;

		mcu_dp_enum_update(DPID_MASTER_MODE,sysMode,STR_GATEWAY_ITSELF_ID,my_strlen(STR_GATEWAY_ITSELF_ID)); //上报主机模式
		
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

//离家布防模式处理
static void S_ENArmModeProc(void)
{
	unsigned char tBuff[3],id,dat;
	Stu_DTC tStuDtc;
	if(pStuSystemMode->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pStuSystemMode->refreshScreenCmd = SCREEN_CMD_NULL;
		pStuSystemMode->keyVal = 0xFF;
		
	 
		hal_Oled_ClearArea(0,20,128,24);		//清防御模式文案
		hal_Oled_ClearArea(0,12,128,8);		//清报警探测器文案
		hal_Oled_ShowString(16,20,"Away arm",24,1);
		 
		hal_Oled_Refresh();
	}
	
	
	if(QueueDataLen(RFDRcvMsg))
	{
		QueueDataOut(RFDRcvMsg,&dat);
		if(dat == '#')
		{
			QueueDataOut(RFDRcvMsg,&tBuff[2]);	//地址码高8位
			QueueDataOut(RFDRcvMsg,&tBuff[1]);	//地址码低8位
			QueueDataOut(RFDRcvMsg,&tBuff[0]);	//数据码/功能码
			//EV1527编码方式:1111 0000,高4位1111是地址，0000数据码/功能码
			//2262编码方式：1111 0000都是数据码/功能码
			id = DtcMatching(tBuff);		//探测器匹配
			if(id != 0xFF)
			{
				//匹配到探测器
				GetDtcStu(&tStuDtc,id-1);
				if(tStuDtc.DTCType == DTC_REMOTE)
				{
					if(tBuff[0] == SENSOR_CODE_REMOTE_DISARM)
					{
						SystemMode_Change(SYSTEM_MODE_DISARM);	//遥控器控制撤防
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_HOMEARM)
					{
						SystemMode_Change(SYSTEM_MODE_HOMEARM);	//遥控器控制在家布防
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_SOS)
					{
						SystemMode_Change(SYSTEM_MODE_ALARM);	//遥控器触发SOS报警
						QueueDataIn(DtcTriggerIDMsg, &id, 1); 
					}
					else if(tBuff[0] == SENSOR_CODE_REMOTE_ENARM)
					{
						SystemMode_Change(SYSTEM_MODE_ENARM);	//遥控器控制撤防
					}
				}if(tBuff[0]==SENSOR_CODE_DOOR_OPEN)	//((tBuff[0]==0x0A)	
				{
					SystemMode_Change(SYSTEM_MODE_ALARM);
					QueueDataIn(DtcTriggerIDMsg, &id, 1);
					//切换到Alarming(报警中)
					//SystemMode_Change(SYSTEM_MODE_ALARM);	//探测器触发报警
				
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
		
		 
		hal_Oled_ClearArea(0,20,128,24);		//清防御模式文案
		hal_Oled_ClearArea(0,12,128,8);		//清报警探测器文案
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
			id = DtcMatching(tBuff);		//探测器匹配
			if(id != 0xFF)
			{
				//匹配到探测器
				GetDtcStu(&tStuDtc,id-1);
				 
				if(tStuDtc.DTCType == DTC_REMOTE)
				{
					if(tBuff[0] == SENSOR_CODE_REMOTE_ENARM)
					{
						SystemMode_Change(SYSTEM_MODE_ENARM);	//遥控器控制离家布防
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_DISARM)
					{
						SystemMode_Change(SYSTEM_MODE_DISARM);	//遥控器控制撤防
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_HOMEARM)
					{
						SystemMode_Change(SYSTEM_MODE_HOMEARM);	//遥控器控制在家布防
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_SOS)
					{
						SystemMode_Change(SYSTEM_MODE_ALARM);	//遥控器触发SOS报警
						QueueDataIn(DtcTriggerIDMsg, &id, 1);
					}
				}else if((tBuff[0]==SENSOR_CODE_DOOR_OPEN)
				|| (tBuff[0]==SENSOR_CODE_DOOR_TAMPER)
				|| (tBuff[0]==SENSOR_CODE_PIR)
				|| (tBuff[0]==SENSOR_CODE_PIR_TAMPER))
				{
					if(tStuDtc.ZoneType==ZONE_TYP_24HOURS)
					{
						SystemMode_Change(SYSTEM_MODE_ALARM);	//探测器触发紧急报警
						QueueDataIn(DtcTriggerIDMsg, &id, 1);
					}else if(tStuDtc.ZoneType != ZONE_TYP_2ND)	//判断探测器是否非为在家防御模式
					{
						SystemMode_Change(SYSTEM_MODE_ALARM);	//探测器触发紧急报警
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
		
		 
		hal_Oled_ClearArea(0,20,128,24);		//清防御模式文案
		hal_Oled_ClearArea(0,12,128,8);		//清报警探测器文案
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
			id = DtcMatching(tBuff);		//探测器匹配
			if(id != 0xFF)
			{
				//匹配到探测器
				GetDtcStu(&tStuDtc,id-1);
				 
				if(tStuDtc.DTCType == DTC_REMOTE)
				{
					if(tBuff[0] == SENSOR_CODE_REMOTE_ENARM)
					{
						SystemMode_Change(SYSTEM_MODE_ENARM);	//遥控器控制离家布防
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_DISARM)
					{
						SystemMode_Change(SYSTEM_MODE_DISARM);	//遥控器控制撤防
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_HOMEARM)
					{
						SystemMode_Change(SYSTEM_MODE_HOMEARM);	//遥控器控制在家布防
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_SOS)
					{
						SystemMode_Change(SYSTEM_MODE_ALARM);	//遥控器触发SOS报警
						QueueDataIn(DtcTriggerIDMsg, &id, 1);
					}
				}else if((tBuff[0]==SENSOR_CODE_DOOR_OPEN)
				|| (tBuff[0]==SENSOR_CODE_DOOR_TAMPER)
				|| (tBuff[0]==SENSOR_CODE_PIR)
				|| (tBuff[0]==SENSOR_CODE_PIR_TAMPER))
				{
					if(tStuDtc.ZoneType==ZONE_TYP_24HOURS)
					{
						SystemMode_Change(SYSTEM_MODE_ALARM);	//探测器触发紧急报警
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
		
		hal_Oled_ClearArea(0,20,128,24);		//清防御模式文案
		hal_Oled_ClearArea(0,12,128,8);		//清报警探测器文案
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
			id = DtcMatching(tBuff);		//探测器匹配
			if(id != 0xFF)
			{
				//匹配到探测器
				GetDtcStu(&tStuDtc,id-1);
				
				if(tStuDtc.DTCType == DTC_REMOTE)
				{
					if(tBuff[0] == SENSOR_CODE_REMOTE_DISARM)
					{
						SystemMode_Change(SYSTEM_MODE_DISARM);	//遥控器控制撤防
					}else if(tBuff[0] == SENSOR_CODE_REMOTE_SOS)
					{
						QueueDataIn(DtcTriggerIDMsg, &id, 1);
					}
				}else if((tBuff[0]==SENSOR_CODE_DOOR_OPEN)
				|| (tBuff[0]==SENSOR_CODE_DOOR_TAMPER)
				|| (tBuff[0]==SENSOR_CODE_PIR)
				|| (tBuff[0]==SENSOR_CODE_PIR_TAMPER))						//警戒模式下，防御等级最高，所有探测器触发都报警
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
				hal_Oled_ClearArea(0,12,128,8);		//清报警探测器文案
				hal_Oled_ShowString(34,12,"Remote:",8,1);
				hal_Oled_ShowString(76,12,"sos",8,1);
				mcu_dp_string_update(DPID_ALARM_ACTIVE,"Remote:sos",sizeof("Remote:sos"),STR_GATEWAY_ITSELF_ID,my_strlen(STR_GATEWAY_ITSELF_ID)); //STRING型数据上报;	
					
			}
			else if(tStuDtc.DTCType == DTC_DOOR)
			{
			//Door:Zone-001
				hal_Oled_ClearArea(0,12,128,8);		//清报警探测器文案
				hal_Oled_ShowString(25,12,"Door:",8,1);
				hal_Oled_ShowString(55,12,tStuDtc.Name,8,1);	
				mcu_dp_string_update(DPID_ALARM_ACTIVE,tStuDtc.Name,sizeof(tStuDtc.Name),STR_GATEWAY_ITSELF_ID,my_strlen(STR_GATEWAY_ITSELF_ID)); //STRING型数据上报
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
			hal_Oled_ClearArea(0,12,128,8);		//清报警探测器文案
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
			 //刷屏尽量不要太快，下面两个函数调用需要大约20ms时间
			 hal_Oled_ShowString(16,20,"Alarming",24,1);	
			 hal_Oled_Refresh();				
			 
		 }else
		 {
			 hal_Oled_ClearArea(0,20,128,24);		//清防御模式文案
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
		
			get_wifi_st();	//发送获取模组连接状态指令
		
		}	

		if((timer%200)==0)	//2秒获取时间
		{
			mcu_get_system_time();	//每2秒获取一次系统时间
			showSystemTime();		//这里需要耗费几十ms
		}

	}else
	{
		if(timer > 100)
		{
			timer = 0;
		
			get_wifi_st();	//发送获取模组连接状态指令
		
		}
	}
	if(bwifiWorkState != wifiWorkState)
	{
		bwifiWorkState = wifiWorkState;
		switch(wifiWorkState)
		{
			case SMART_CONFIG_STATE:		//进入smartlink配网状态
				
				hal_Oled_ShowString(0,0,"N",8,1);
				LedMsgInput(LED1,LED_BLINK1,1);
				hal_Oled_Refresh();
			break;	
			
			case WIFI_NOT_CONNECTED:		//WIFI配置成功但未连上路由器
				hal_Oled_ShowString(0,0,"W",8,1);
				LedMsgInput(LED1,LED_BLINK3,1);
				hal_Oled_Refresh();
			break;
			case WIFI_CONNECTED:			//WIFI配置成功且连上路由器
				hal_Oled_ShowString(0,0,"R",8,1);
				LedMsgInput(LED1,LED_BLINK4,1);
				hal_Oled_Refresh();
			break;
			
			case WIFI_CONN_CLOUD: 			//WIFI已经连接上云服务器
				hal_Oled_ShowString(0,0,"S",8,1);
				LedMsgInput(LED1,LED_LIGHT,1);
				hal_Oled_Refresh();
				 
			break;
		}
	}

	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		 
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
		switch(keys)
		{
			case KEY6_LONG_PRESS:
				pModeMenu = &settingModeMenu[0];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
		}
	}
	pStuSystemMode->action(); //=S_ENArmModeProc();//执行系统防线模式


	

}


//设置主菜单
static void stgMenu_MainMenuCBS(void)
{
	unsigned char keys;
	unsigned char i;
	unsigned char ClrScreenFlag;
	static stu_mode_menu *pMenu;		//用来保存当前选中的菜单
	static stu_mode_menu *bpMenu=0;		//用来备份上一次菜单选项，主要用于刷屏判断
	static unsigned char stgMainMenuSelectedPos=0;	//用来记录当前选中菜单的位置
	static stu_mode_menu *MHead,*MTail;		//这两个结构体指针是为了上下切换菜单时做翻页处理
	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		pMenu = &settingModeMenu[0];
		hal_Oled_Clear();
		
		hal_Oled_ShowString(37,0,pMenu->pModeType,12,1);
		hal_Oled_Refresh();
	
		
		pMenu = &settingModeMenu[1];
		
		MHead = pMenu;			//记录当前显示菜单第一项
		MTail = pMenu+3;		//记录当前显示菜单最后一项,一页显示4行		
		bpMenu = 0;
 
		ClrScreenFlag = 1;
		stgMainMenuSelectedPos = 1;
		keys = 0xFF;
 
	}
	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RECOVER)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		//恢复之前的选择位置显示
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
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
		switch(keys)
		{
			case KEY1_CLICK:		//上
				
				if(stgMainMenuSelectedPos ==1)
				{
					MHead = MHead->pLase;
					pMenu = pMenu->pLase;
					MTail = MTail->pLase;
					stgMainMenuSelectedPos = 1;
					ClrScreenFlag = 1;
				}else
				{
					hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//取消选中本菜单显示
					hal_Oled_Refresh();
					pMenu = pMenu->pLase;
					stgMainMenuSelectedPos--;
				}
			break;
			case KEY2_CLICK:		//下
				if(stgMainMenuSelectedPos ==4)
				{
					MHead = MHead->pNext;	
					pMenu = pMenu->pNext;
					MTail = pMenu;
					stgMainMenuSelectedPos = 4;
					ClrScreenFlag = 1;
				}else
				{
					hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//取消选中本菜单显示
					hal_Oled_Refresh();
					pMenu = pMenu->pNext;																			//切换下一个选项
					stgMainMenuSelectedPos++;
				}

			break;
			
			case KEY5_CLICK_RELEASE:	//取消
				pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;


			case KEY6_CLICK_RELEASE:	//确定
				pModeMenu->pChild = pMenu;
				pModeMenu = pModeMenu->pChild;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
			
		}
	}
	if(bpMenu != pMenu)		//选中菜单的记录
	{
		bpMenu = pMenu;
		if(ClrScreenFlag)
		{
			
			ClrScreenFlag = 0;
			pMenu = MHead;
			hal_Oled_ClearArea(0,14,128,50);		//清屏
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

//探测器配对菜单处理函数
static void stgMenu_LearnSensorCBS(void)
{
	unsigned char keys,dat,tBuff[3];
	static unsigned char PairingComplete = 0;
	static unsigned short Timer = 0;
	Stu_DTC stuTempDevice; 		//用于设置探测器参数时初始化探测器信息
	 
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
			hal_Oled_ClearArea(0,28,128,36);		//清屏
			
			
			stuTempDevice.Code[2] = tBuff[2];
			stuTempDevice.Code[1] = tBuff[1];
			stuTempDevice.Code[0] = tBuff[0];
			
			if((stuTempDevice.Code[0]==SENSOR_CODE_DOOR_OPEN) ||
			(stuTempDevice.Code[0]==SENSOR_CODE_DOOR_CLOSE) ||
			(stuTempDevice.Code[0]==SENSOR_CODE_DOOR_TAMPER)||
			(stuTempDevice.Code[0]==SENSOR_CODE_DOOR_LOWPWR))
			{
				//是无线门磁码
				stuTempDevice.DTCType = DTC_DOOR;
				
			}else if((stuTempDevice.Code[0]==SENSOR_CODE_REMOTE_ENARM) ||
			(stuTempDevice.Code[0]==SENSOR_CODE_REMOTE_DISARM) ||
			(stuTempDevice.Code[0]==SENSOR_CODE_REMOTE_HOMEARM) ||
			(stuTempDevice.Code[0]==SENSOR_CODE_REMOTE_SOS))
			{
				//无线遥控器码
				stuTempDevice.DTCType = DTC_REMOTE;
			}else if((stuTempDevice.Code[0]==SENSOR_CODE_PIR)
			|| (stuTempDevice.Code[0]==SENSOR_CODE_PIR_LOWPWR)
			|| (stuTempDevice.Code[0]==SENSOR_CODE_PIR_TAMPER))
			{
				//无线红外码
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
				PairingComplete = 1;		//配对完成标志
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
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
		switch(keys)
		{
			case KEY5_CLICK_RELEASE:	//取消
				pModeMenu = pModeMenu->pParent;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
			case KEY5_LONG_PRESS:		//返回桌面
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
			pModeMenu = pModeMenu->pParent;			//1.5秒时间到，自动返回父级菜单
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
		}
	}
}


//探测器列表菜单处理函数
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
	static stu_mode_menu *bpMenu=0;		//用来备份上一次菜单选项，主要用于刷屏判断
	static unsigned char stgMainMenuSelectedPos=0;	//用来记录当前选中菜单的位置
	static stu_mode_menu *MHead,*MTail;		//这两个结构是为了上下切换菜单时做翻页处理
	static unsigned char pMenuIdx=0;		//用来动态指示菜单下标,最终这个就是已学习探测器的总数量	
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
		
		//逐个判断，把配对的探测器都找出来
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
			//有探测器存在的情况
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
			//没有探测器
			bpMenu = pMenu;
			hal_Oled_ShowString(0,14," No detectors.",8,1);
			hal_Oled_Refresh();
		}

		MHead = pMenu;			//记录当前显示菜单第一项
		if(pMenuIdx < 2)
		{
			MTail = pMenu;
		}else if(pMenuIdx < 5)
		{
			MTail = pMenu+(pMenuIdx-1);
		}else
		{
			MTail = pMenu+3;		//记录当前显示菜单最后一项,一页显示4行
		}
		
	}else if(pModeMenu->refreshScreenCmd==SCREEN_CMD_RECOVER)
	{	
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		//恢复之前的选择位置显示
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
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
		switch(keys)
		{
			case KEY1_CLICK_RELEASE:		//上
				if(pMenuIdx < 2)
				{
					//只有一个探测器不做处理
				}else if(pMenuIdx < 5)
				{
					//只有一页，也就是只有4个探测器的时候
					if(stgMainMenuSelectedPos ==1)	//判断是否选中的是第一行
					{
						//头尾指针不变，只把当前菜单指向上一个
						stgMainMenuSelectedPos = pMenuIdx;
						ClrScreenFlag = 1;
						pMenu = pMenu->pLase;
						
					}else 
					{
						//不清屏，直接刷新局部显示
						hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//取消选中本菜单显示
						hal_Oled_Refresh();
						pMenu = pMenu->pLase;
						stgMainMenuSelectedPos--;
					}
				}else if(pMenuIdx > 4)	//当前探测器超过4个
				{
					if(stgMainMenuSelectedPos ==1)	//判断是否选中的是第一行
					{
						MHead = MHead->pLase;
						pMenu = pMenu->pLase;
						MTail = MTail->pLase;
						stgMainMenuSelectedPos = 1;
						ClrScreenFlag = 1;
					}else
					{
						hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//取消选中本菜单显示
						hal_Oled_Refresh();
						pMenu = pMenu->pLase;
						stgMainMenuSelectedPos--;
					}
				}
			break;
			
			case KEY2_CLICK_RELEASE:		//下 
				if(pMenuIdx < 2)
				{
					//只有一个探测器不做处理
				}else if(pMenuIdx < 5)
				{
					//只有一页，也就是只有4个探测器的时候
					if(stgMainMenuSelectedPos ==pMenuIdx)	//判断是否选中的是第4行
					{
						//头尾指针不变，只把当前菜单指向下个
						pMenu = pMenu->pNext;
						stgMainMenuSelectedPos = 1;
						ClrScreenFlag = 1;
						
					}else 
					{
						//不清屏，直接刷新局部显示
						hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//取消选中本菜单显示
						hal_Oled_Refresh();
						pMenu = pMenu->pNext;																			//切换下一个选项
						stgMainMenuSelectedPos++;
					}
				}else if(pMenuIdx > 4)	//当前探测器超过4个
				{
					if(stgMainMenuSelectedPos ==4)	//判断是否选中的是第一行
					{
						MHead = MHead->pNext;	
						pMenu = pMenu->pNext;
						MTail = pMenu;
						stgMainMenuSelectedPos = 4;
						ClrScreenFlag = 1;
					}else
					{
						hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//取消选中本菜单显示
						hal_Oled_Refresh();
						pMenu = pMenu->pNext;																			//切换下一个选项
						stgMainMenuSelectedPos++;
					}
				}
			break;
			
			case KEY6_CLICK_RELEASE:			//确定
				if(pMenuIdx>0)
				{
					pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN]; 
					pModeMenu->reserved = pMenu->reserved;	//这里用于传递后面要查看、修改、删除探测器的ID号
					pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
				}
			break;
			
			case KEY5_CLICK_RELEASE:	//取消
				pModeMenu = pModeMenu->pParent;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
			break;
			case KEY5_LONG_PRESS:		//返回桌面
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
			hal_Oled_ClearArea(0,14,128,50);		//清屏
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
	 
	static stu_mode_menu *MHead;		//这两个结构用来方便显示，页面的头跟尾
	static stu_mode_menu *pMenu,*bpMenu=0;	//用来记录当前选中的菜单
	
	static unsigned char stgMainMenuSelectedPos=0;				 

	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{	//执行页面切换时屏幕刷新显示 
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		
		if(CheckPresenceofDtc(pModeMenu->reserved))
		{
			GetDtcStu(&tStuDtc,pModeMenu->reserved);	//读取探测器信息
		}
		 
		 
		hal_Oled_Clear();
		hal_Oled_ShowString(40,0,tStuDtc.Name,12,1);
		
		hal_Oled_Refresh();
		

		pMenu = &DL_ZX_Review[1];
		stgMainMenuSelectedPos = 1;
		MHead = pMenu;			//记录当前显示菜单第一项
		ClrScreenFlag = 1;
		bpMenu = 0;
		keys = 0xFF;
	}else if(pModeMenu->refreshScreenCmd==SCREEN_CMD_RECOVER)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		//恢复之前的选择位置显示
		if(CheckPresenceofDtc(pModeMenu->reserved))
		{
			GetDtcStu(&tStuDtc,pModeMenu->reserved);	//读取探测器信息
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
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
		switch(keys)
		{
			case KEY1_CLICK:		//上
				if(stgMainMenuSelectedPos ==1)
				{
					pMenu = pMenu->pLase;
					stgMainMenuSelectedPos = 3;
					ClrScreenFlag = 1;
				}else
				{
					hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//取消选中本菜单显示
					hal_Oled_Refresh();
					pMenu = pMenu->pLase;
					stgMainMenuSelectedPos--;
				}
			break;
			case KEY2_CLICK:		//下
				if(stgMainMenuSelectedPos ==3)
				{
					pMenu = pMenu->pNext;
					stgMainMenuSelectedPos = 1;
					ClrScreenFlag = 1;
				}else
				{
					hal_Oled_ShowString(0,14*stgMainMenuSelectedPos,pMenu->pModeType,8,1);		//取消选中本菜单显示
					hal_Oled_Refresh();
					pMenu = pMenu->pNext;																			//切换下一个选项
					stgMainMenuSelectedPos++;
				}
			
				
			break;
			
			case KEY6_CLICK_RELEASE:
				pMenu->reserved = pModeMenu->reserved;	//继续把指定探测器结构体数组下标传递下去
				pModeMenu = pMenu;
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
			
			case KEY5_CLICK_RELEASE:
				pModeMenu = &settingModeMenu[STG_MENU_DTC_LIST];	//这里不能直接返回父级，因为探测器个数是动态的，父级没初始化
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
			hal_Oled_ClearArea(0,14,128,50);		//清屏
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
	{	//执行页面切换时屏幕刷新显示 
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		 
		if(CheckPresenceofDtc(pModeMenu->reserved))
		{
			GetDtcStu(&tStuDtc,pModeMenu->reserved);
		}
		 
 
		hal_Oled_Clear();
		hal_Oled_ShowString(40,0,pModeMenu->pModeType,12,1);
		
		hal_Oled_ShowString(0,16,"<Name>: ",8,1); 
		//<Name>: 8个字符，8*6=48
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
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
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
	{	//执行页面切换时屏幕刷新显示 
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
			pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
			if(keys == KEY3_CLICK_RELEASE)
			{
				if(stgMainMenuSelectedPos == 0)		
				{
					//设置Type
					if(setValue == DTC_DOOR)
					{
						setValue = DTC_TYP_SUM-1;
					}else
					{
						setValue--;
					}
					tStuDtc.DTCType = (DTC_TYPE_TYPEDEF)setValue;			//更新探测器参数
					hal_Oled_ClearArea(48,28,80,8);		//清屏
					hal_Oled_ShowString(48,28,pDL_ZX_Edit_DTCType_Val[setValue],8,0);
					hal_Oled_Refresh();

				}else if(stgMainMenuSelectedPos == 1)
				{
					//设置Zone type
					if(setValue == ZONE_TYP_24HOURS)
					{
						setValue = STG_DEV_AT_SUM-1;
					}else
					{
						setValue--;
					}
					tStuDtc.ZoneType = (ZONE_TYPED_TYPEDEF)setValue;			////更新探测器参数
					hal_Oled_ClearArea(72,40,56,8);		//清屏
					hal_Oled_ShowString(72,40,pDL_ZX_Edit_ZoneType_Val[setValue],8,0);
					hal_Oled_Refresh();
				}
			}else if(keys == KEY4_CLICK_RELEASE)
			{
				if(stgMainMenuSelectedPos == 0)
				{
					//设置Type
					if(setValue == (DTC_TYP_SUM-1))
					{
						setValue = 0;
					}else
					{
						setValue++;
					}
					tStuDtc.DTCType = (DTC_TYPE_TYPEDEF)setValue;			//更新探测器参数
					hal_Oled_ClearArea(48,28,80,8);		//清屏
					hal_Oled_ShowString(48,28,pDL_ZX_Edit_DTCType_Val[setValue],8,0);
					hal_Oled_Refresh();
				}else if(stgMainMenuSelectedPos == 1)
				{
					//设置Zone type
					if(setValue == (STG_DEV_AT_SUM-1))
					{
						setValue = 0;
					}else
					{
						setValue++;
					}
					tStuDtc.ZoneType = (ZONE_TYPED_TYPEDEF)setValue;			////更新探测器参数
					hal_Oled_ClearArea(72,40,56,8);		//清屏
					hal_Oled_ShowString(72,40,pDL_ZX_Edit_ZoneType_Val[setValue],8,0);
					hal_Oled_Refresh();
				}
				
			}else if((keys==KEY1_CLICK_RELEASE) || (keys==KEY2_CLICK_RELEASE))
			{
				if(stgMainMenuSelectedPos == 0)
				{
					stgMainMenuSelectedPos = 1;
					setValue = tStuDtc.ZoneType;
					hal_Oled_ClearArea(48,28,80,8);		//清屏
					hal_Oled_ShowString(48,28,pDL_ZX_Edit_DTCType_Val[tStuDtc.DTCType],8,1);	//恢复探测器类型未选中显示

					hal_Oled_ClearArea(72,40,56,8);		//清屏

					hal_Oled_ShowString(72,40,pDL_ZX_Edit_ZoneType_Val[setValue],8,0);			//切换选中菜单到防区类型
					hal_Oled_Refresh();
				}else
				{
					stgMainMenuSelectedPos = 0;
					setValue = tStuDtc.DTCType;
					hal_Oled_ClearArea(48,28,80,8);		//清屏
					hal_Oled_ShowString(48,28,pDL_ZX_Edit_DTCType_Val[setValue],8,0);		//切换选中菜单到探测器类型
					
					
					hal_Oled_ClearArea(72,40,56,8);		//清屏
					hal_Oled_ShowString(72,40,pDL_ZX_Edit_ZoneType_Val[tStuDtc.ZoneType],8,1);	//恢复探测器防区类型未选中显示
					
					hal_Oled_Refresh();
				}
			}else if(keys == KEY5_CLICK_RELEASE)
			{
				pModeMenu = &DL_ZX_Review[STG_MENU_DL_ZX_REVIEW_MAIN];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
					
			}else if(keys == KEY6_CLICK_RELEASE)
			{
				timer = 0;
				SetDtcAbt(tStuDtc.ID-1,&tStuDtc);		//更新探测器属性，带写入EEPROM功能
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
		if(timer > 150)		//1.5秒自动退出
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
	{	//执行页面切换时屏幕刷新显示 
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
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
		
		if((keys == KEY3_CLICK_RELEASE) || (keys == KEY4_CLICK_RELEASE))
		{
			if(stgMainMenuSelectedPos == 0)		
			{
				stgMainMenuSelectedPos = 1;
				hal_Oled_ClearArea(40,48,88,16);		//清屏
				hal_Oled_ShowString(40,48,"Yes",12,0); 
				hal_Oled_ShowString(88,48,"No",12,1); 
				hal_Oled_Refresh();

			}else if(stgMainMenuSelectedPos == 1)
			{
				stgMainMenuSelectedPos = 0;
				hal_Oled_ClearArea(40,48,88,16);		//清屏
				hal_Oled_ShowString(40,48,"Yes",12,1); 
				hal_Oled_ShowString(88,48,"No",12,0); 
				hal_Oled_Refresh();
 
			}
		}else if(keys == KEY6_CLICK_RELEASE)
		{
			if(stgMainMenuSelectedPos)
			{
				//确认删除
				DelComplete = 1;
				timer = 0;
				tStuDtc.Mark = 0;
				SetDtcAbt(tStuDtc.ID-1,&tStuDtc);		//更新探测器属性，带写入EEPROM功能
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
		if(timer > 150)		//1.5秒自动退出
		{
			timer = 0;
			DelComplete = 0;
			pModeMenu = &settingModeMenu[STG_MENU_DTC_LIST];
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
		}
	}
}

//wifi配网菜单处理函数
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
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
		
		if((keys == KEY3_CLICK_RELEASE) 
		|| (keys == KEY4_CLICK_RELEASE))
		{
			if(stgMainMenuSelectedPos == 0)		
			{
				stgMainMenuSelectedPos = 1;
				hal_Oled_ClearArea(40,48,88,16);		//清屏
				hal_Oled_ShowString(40,48,"Yes",12,0); 
				hal_Oled_ShowString(88,48,"No",12,1); 

				hal_Oled_Refresh();

			}else if(stgMainMenuSelectedPos == 1)
			{
				stgMainMenuSelectedPos = 0;
				hal_Oled_ClearArea(40,48,88,16);		//清屏
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
				mcu_set_wifi_mode(1);		//让wifi进入AP配网模式
				hal_Oled_ClearArea(0,20,128,44);		//清屏
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
			hal_Oled_ClearArea(0,20,128,44);		//清屏
			hal_Oled_ShowString(0,30,"Enter ap mode.",8,1);
			hal_Oled_Refresh();
			LedMsgInput(LED1,LED_BLINK1,1);
			APStep = 2;
		}
	}else if(APStep == 2)
	{
		if(wifiWorkState == WIFI_NOT_CONNECTED)
		{
			hal_Oled_ClearArea(0,20,128,44);		//清屏
			hal_Oled_ShowString(0,30,"Connect to wifi ok.",8,1);
			hal_Oled_Refresh();
			LedMsgInput(LED1,LED_BLINK3,1);
			APStep = 3;	
		}
	}else if(APStep == 3)
	{
		if(wifiWorkState == WIFI_CONNECTED)
		{
			hal_Oled_ClearArea(0,20,128,44);		//清屏
			hal_Oled_ShowString(0,30,"Connect to router ok.",8,1);
			hal_Oled_Refresh();
			LedMsgInput(LED1,LED_BLINK4,1);
			APStep = 4;
		}
	}else if(APStep == 4)
	{
		if(wifiWorkState == WIFI_CONN_CLOUD)
		{
			hal_Oled_ClearArea(0,20,128,44);		//清屏
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

//设备信息菜单处理函数
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
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
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

//恢复出厂设置菜单处理函数
static void stgMenu_FactorySettingsCBS(void)
{
	unsigned char keys = 0xFF;

	static unsigned short timer = 0;
	static unsigned char Complete = 0;
	
	static unsigned char stgMainMenuSelectedPos=0;


	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{	//执行页面切换时屏幕刷新显示 
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
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
		if(!Complete)
		{
			if((keys == KEY3_CLICK_RELEASE) || (keys == KEY4_CLICK_RELEASE))
			{
				if(stgMainMenuSelectedPos == 0)		
				{
					stgMainMenuSelectedPos = 1;
					hal_Oled_ClearArea(40,48,88,16);		//清屏
					hal_Oled_ShowString(40,48,"Yes",12,0); 
					hal_Oled_ShowString(88,48,"No",12,1); 
					hal_Oled_Refresh();

				}else if(stgMainMenuSelectedPos == 1)
				{
					stgMainMenuSelectedPos = 0;
					hal_Oled_ClearArea(40,48,88,16);		//清屏
					hal_Oled_ShowString(40,48,"Yes",12,1); 
					hal_Oled_ShowString(88,48,"No",12,0); 
					hal_Oled_Refresh();
	 
				}
			}else if(keys == KEY6_CLICK_RELEASE)
			{
				if(stgMainMenuSelectedPos)
				{
					//确认 
					Complete = 1;
					timer = 0;
					FactoryReset();		//调用复位EEPROM数据函数
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
		if(timer > 150)		//1.5秒自动退出
		{
			timer = 0;
			Complete = 0;
			pModeMenu = pModeMenu->pParent;
			pModeMenu->refreshScreenCmd = SCREEN_CMD_RECOVER;
		}
	}
}

//-----------------驱动层回调处理函数------------------------
 
//按键回调函数
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


//RFD接收回调函数
static void RfdRcvHandle(unsigned char *pBuff)
{
	unsigned char temp;
	temp = '#';
	QueueDataIn(RFDRcvMsg, &temp, 1);
	QueueDataIn(RFDRcvMsg, &pBuff[0], 3);
	
	 
}

//云服务器数据接收回调函数
static void ServerEventHandle(WIFI_MSG_TYPE type,unsigned char *pData)
{
	switch(type)
	{
		case WF_HOST_STATE:	//主机状态
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
		time[0]为是否获取时间成功标志，为 0 表示失败，为 1表示成功
		time[1] 为 年 份 , 0x00 表 示2000 年
		time[2]为月份，从 1 开始到12 结束
		time[3]为日期，从 1 开始到31 结束
		time[4]为时钟，从 0 开始到23 结束
		time[5]为分钟，从 0 开始到59 结束
		time[6]为秒钟，从 0 开始到59 结束
		time[7]为星期，从 1 开始到 7 结束，1代表星期一
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
			 * @brief  获取 WIFI 状态结果
			 * @param[in] {result} 指示 WIFI 工作状态
			 * @ref         0x00: wifi状态 1 smartconfig 配置状态
			 * @ref         0x01: wifi状态 2 AP 配置状态
			 * @ref         0x02: wifi状态 3 WIFI 已配置但未连上路由器
			 * @ref         0x03: wifi状态 4 WIFI 已配置且连上路由器
			 * @ref         0x04: wifi状态 5 已连上路由器且连接到云端
			 * @ref         0x05: wifi状态 6 WIFI 设备处于低功耗模式
			 * @ref         0x06: wifi状态 7 Smartconfig&AP共存状态
			 */	
		break;

	}
}

void mcu_all_dp_update()
{

  mcu_dp_enum_update(DPID_MASTER_MODE,pStuSystemMode->ID,STR_GATEWAY_ITSELF_ID,my_strlen(STR_GATEWAY_ITSELF_ID)); //枚举型数据上报;
  mcu_dp_string_update(DPID_ALARM_ACTIVE," ",sizeof(" "),STR_GATEWAY_ITSELF_ID,my_strlen(STR_GATEWAY_ITSELF_ID)); //STRING型数据上报;

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



