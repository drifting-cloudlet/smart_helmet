/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-11     User         DHT11 温湿度传感器驱动头文件
 */

#ifndef DRV_DHT11_H
#define DRV_DHT11_H

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_pin.h"

/* DHT11 设备结构体 */
typedef struct
{
    rt_base_t pin;          /* DHT11 数据引脚 */
    rt_uint8_t humidity;    /* 湿度整数部分 */
    rt_uint8_t temperature; /* 温度整数部分 */
} dht11_device_t;

/* DHT11 读取结果枚举 */
typedef enum
{
    DHT11_OK = 0,           /* 读取成功 */
    DHT11_ERROR_TIMEOUT,    /* 超时错误 */
    DHT11_ERROR_CHECKSUM    /* 校验和错误 */
} dht11_result_t;

/**
 * @brief 初始化 DHT11 设备
 * @param dev DHT11 设备结构体指针
 * @param pin 数据引脚编号
 * @return RT_EOK 成功，其他值失败
 */
rt_err_t dht11_init(dht11_device_t *dev, rt_base_t pin);

/**
 * @brief 读取 DHT11 温湿度数据
 * @param dev DHT11 设备结构体指针
 * @param temp 温度指针（输出参数）
 * @param humi 湿度指针（输出参数）
 * @return dht11_result_t 读取结果
 */
dht11_result_t dht11_read(dht11_device_t *dev, rt_uint8_t *temp, rt_uint8_t *humi);

#endif /* DRV_DHT11_H */
