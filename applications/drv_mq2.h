#ifndef DRV_MQ2_H
#define DRV_MQ2_H

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_pin.h"


//MQ2设备结构体
typedef struct
{
//	rt_base_t aopin;          /* MQ2 ADC连接的引脚 */			默认连接P1_0
	rt_base_t dopin;					/* MQ2 DO所连接的引脚*/
	float adc_val;    		/* ADC读取的数据 */
	float ch4ppm;  						/* 甲烷浓度 */
}mq2_device_t;

//MQ2 读取数据结果枚举
typedef enum
{
	MQ2_OK = 0,							//0表示正常读取数据
	MQ2_ERROR_TIMEOUT,			//超时错误
	MQ2_ERROR_CHECKSUM,			//校验和错误
}mq2_result_t;


//mq2初始化函数
rt_err_t mq2_init(mq2_device_t *dev,rt_base_t dopin);


//数据读取函数
mq2_result_t MQ2_GetPmm(mq2_device_t *dev);
#endif
