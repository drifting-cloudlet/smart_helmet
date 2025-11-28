#include "mydefine.h"
#include "drv_mq2.h"
#include "adc_app.h"

//MQ2的DO所接的位置
#define MQ2_DATA_PIN     ((3*32)+7)			//P3_7

/* MQ2设备对象（全局变量，供esp_app访问） */
mq2_device_t g_mq2_dev;

/**
*		@brief 读取线程入口参数
*		@param	未使用线程参数
*/
static void mq2_entry(void *parameter)
{
	mq2_result_t result;
	while(1)
	{
		result = MQ2_GetPmm(&g_mq2_dev);
		if(result == MQ2_OK)
		{
			rt_kprintf("adc_val:%.2f ch4:%.2fppm\n",g_mq2_dev.adc_val,g_mq2_dev.ch4ppm);
		}
		else if(result == MQ2_ERROR_TIMEOUT)
		{
			rt_kprintf("ERROE 超时\n");
		}
		else if(result == MQ2_ERROR_CHECKSUM)
		{
			rt_kprintf("校验和错误\n");
		}

		rt_thread_mdelay(1000);
	}

}

/**
*		@brief MQ2应用层初始化
*		@param 0表示成功
*/

int mq2_app_init(void)
{
	rt_thread_t thread;
	rt_err_t ret;

	ret = mq2_init(&g_mq2_dev,MQ2_DATA_PIN);
	if(ret != RT_EOK)
	{
		rt_kprintf("初始化失败\n");
		return -1;
	}
	
	rt_kprintf("[MQ2] 等待传感器稳定...\n");
  rt_thread_mdelay(1000);
	
	thread = rt_thread_create("mq2",
														mq2_entry,
														RT_NULL,
														1024,
														30,
														10);
	if(thread != RT_NULL)
	{
		rt_thread_startup(thread);
		rt_kprintf("[MQ2] Application initialized successfully!\n\n");
	}
	else
	{
    rt_kprintf("[MQ2] Thread create failed!\n");
		return -1;
	}
	return 0;
}

INIT_APP_EXPORT(mq2_app_init);

/**
 * @brief 获取当前甲烷浓度
 * @return 甲烷浓度值(ppm)
 */
float mq2_get_ch4ppm(void)
{
	return g_mq2_dev.ch4ppm;
}

