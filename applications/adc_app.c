#include "adc_app.h"

#define ADC_DEV_NAME "adc0"					//ADC设备名称
#define ADC_DEV_CHANNEL 0						//ADC通道
#define REDER_VOLTAGE	330						//参考电压3V3 数据精度乘以100保留两位小数
#define CONVERT_BITS (1 << 16)				//转化位数

static int adc_vol_sample(int argc,char *argv[])
{
	rt_adc_device_t adc_dev;
	rt_uint32_t value,vol;
	rt_err_t ret = RT_EOK;
	
	//查找设备
	adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
	if(adc_dev == RT_NULL)
	{
		rt_kprintf("adc sample run failed!cannot find %s devvice!\n",ADC_DEV_NAME);
		return RT_ERROR;
	}
	//使能设备
	ret = rt_adc_enable(adc_dev,ADC_DEV_CHANNEL);
	
	//读取采样值
	value = rt_adc_read(adc_dev,ADC_DEV_CHANNEL);
	rt_kprintf("the value is %d\n",value);
	
	//转化为对应电压值
	vol = value * REDER_VOLTAGE / CONVERT_BITS;
	rt_kprintf("the voltage is %d.%02d\n",vol / 100, vol % 100);
	
	//关闭通道
	ret = rt_adc_disable(adc_dev,ADC_DEV_CHANNEL);
	
	return ret;
}	

//导出amsh命令列表中
//MSH_CMD_EXPORT(adc_vol_sample,adc voltage convert sample);


float adc_read_value(void)
{
	rt_adc_device_t adc_dev;
  rt_uint32_t value = 0;
	
  adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
  if(adc_dev == RT_NULL)
  {
		rt_kprintf("Cannot find adc0 device!\n");
    return 0;
  }

	rt_adc_enable(adc_dev, 0);
  value = rt_adc_read(adc_dev, 0);
  rt_adc_disable(adc_dev, 0);
//	return (float)value * 3.3f / 65536.0f;
  return (float)value;
}


