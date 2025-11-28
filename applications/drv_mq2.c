#include "drv_mq2.h"
#include "mydefine.h"
#include "adc_app.h"

#define MQ2_MAXRead  10

/*
	初始化函数
	启动ADC引脚
*/
rt_err_t mq2_init(mq2_device_t *dev,rt_base_t dopin)
{
//	dev->aopin = aopin;
	dev->dopin = dopin;
	dev->adc_val = 0;
	dev->ch4ppm = 0;

	rt_pin_mode(dopin,PIN_MODE_OUTPUT);
//	rt_pin_write(dopin,PIN_MODE_INPUT);

	return RT_EOK;
}

/*
	ADC数据读取
*/

//static  MQ2_GetData(mq2_device_t *dev)
//{
//	float temp = 0;
//	for(rt_uint8_t i = 0;i< MQ2_MAXRead;i++)
//	{
//		temp += adc_read_value();
//		rt_mdelay(5);
//	}
//	temp = temp / MQ2_MAXRead;
//	dev->adc_val = temp;
//}

/*
	浓度数据读取
*/

mq2_result_t MQ2_GetPmm(mq2_device_t *dev)
{
	float temp = 0;
//	float first_read = adc_read_value();
//  rt_kprintf("[DEBUG] First ADC read: %.4f\n", first_read);
	for(rt_uint8_t i = 0;i< MQ2_MAXRead;i++)
	{
		temp += adc_read_value();
		rt_thread_mdelay(5);
	}
	temp = temp / MQ2_MAXRead;
	dev->adc_val = temp;
	
	float Vol = (temp*3.3f/65536.0f);
	float RS = ((3.3f-Vol)/Vol);
	float R0=6.64;
	
	float ppm = pow(11.5428*R0/RS, 0.6549f);
	dev->ch4ppm = ppm;
	return MQ2_OK;
	  
}


