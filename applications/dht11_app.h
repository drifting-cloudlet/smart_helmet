#ifndef DHT11_APP_H
#define DHT11_APP_H

#include "mydefine.h"
#include "drv_dht11.h"

/* 暴露DHT11设备对象供外部访问 */
extern dht11_device_t g_dht11_dev;

/* 暴露最新读取的温湿度数据 */
extern rt_uint8_t g_dht11_temperature;
extern rt_uint8_t g_dht11_humidity;

/* 获取当前温度 */
rt_uint8_t dht11_get_temperature(void);

/* 获取当前湿度 */
rt_uint8_t dht11_get_humidity(void);

#endif
