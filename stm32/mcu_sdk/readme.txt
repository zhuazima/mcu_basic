
此MCU_SDK是根据涂鸦开发平台上定义的产品功能，自动生成的MCU代码。在此基础上进行修改、补充，可快速完成MCU程序。



开发步骤：


1：需要根据产品实际情况（配网方式，设备能力选择等等）进行配置，请在protocol.h内修改此配置；

2：移植此MCU_SDK，请查看protocol.c文件内的移植步骤,并正确完成移植。移植后，请完成数据下发处理、数据上报、子设备管理【添加删除子设备需要用户自行实现】部分的代码，即可完成全部wifi功能。

文件概览：
此MCU_SDK包括7个文件：
	
（1）protocol.h和protocol.c是需要你修改的。protocol.h 和protocol.c文件内有详细修改说明，请仔细阅读。
	
（2）wifi.h文件为总的.h文件，如需要调用wifi内部功能，请#include "wifi.h"。
	
（3）system.c和system.h是wifi功能实现代码，用户无需修改。
	
（4）mcu_api.c和mcu_api.h内实现全部此用户需调用函数，用户无需修改。

（5）cJSON.h和cJSON.c是cJSON库的源文件，编译的时候需要和cJSON.c一起编译，并且
	 要链接数学函数库（加上-lm）【例如在linux环境下下，使用gcc xxx.c cJSON.c -lm即可成功编译】

开发注意点说明：
（1）用户可以根据subdev_manage.if_subdev_net_in的状态来判断子设备是否可以入网，在该状态为true的情况下进行子设备入网（如果改状态为false，入网会返回子设备入网失败）

（2）子设备添加和删除是否成功最终需要根据模块下发的ADD_DEV_RESULT_CMD指令来判断是否真正添加成功