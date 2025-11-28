/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-11     User         DHT11 温湿度传感器应用示例
 */

#include "mydefine.h"
#include "drv_dht11.h"

/* DHT11 数据引脚定义（根据实际硬件修改） */
/* 例如使用 GPIO 10 号引脚，请根据您的板卡原理图修改 */
#define DHT11_DATA_PIN     ((3*32)+6)			//P3_6

/* DHT11 设备对象（全局变量，供esp_app访问） */
dht11_device_t g_dht11_dev;

/* 最新读取的温湿度数据（全局变量，供esp_app访问） */
rt_uint8_t g_dht11_temperature = 0;
rt_uint8_t g_dht11_humidity = 0;

/**
 * @brief DHT11 读取线程入口函数
 * @param parameter 线程参数（未使用）
 */
static void dht11_thread_entry(void *parameter)
{
    rt_uint8_t temperature;  /* 存储温度值 */
    rt_uint8_t humidity;     /* 存储湿度值 */
    dht11_result_t result;   /* 存储读取结果 */

    /* 打印线程启动信息 */
    rt_kprintf("DHT11 thread started!\n");

    /* 线程主循环 */
    while (1)
    {
        /* 调用驱动读取温湿度数据 */
        result = dht11_read(&g_dht11_dev, &temperature, &humidity);

        /* 根据读取结果进行处理 */
        if (result == DHT11_OK)
        {
            /* 更新全局变量 */
            g_dht11_temperature = temperature;
            g_dht11_humidity = humidity;
            /* 读取成功，打印温湿度数据 */
            rt_kprintf("[DHT11] Temperature: %d C, Humidity: %d %%\n",
                       temperature, humidity);
        }
        else if (result == DHT11_ERROR_TIMEOUT)
        {
            /* 超时错误 */
            rt_kprintf("[DHT11] Read timeout error!\n");
        }
        else if (result == DHT11_ERROR_CHECKSUM)
        {
            /* 校验和错误 */
            rt_kprintf("[DHT11] Checksum error!\n");
        }

        /* 延时 2 秒后再次读取（DHT11 要求两次读取间隔至少 2 秒） */
        rt_thread_mdelay(1000);
    }
}

/**
 * @brief DHT11 应用初始化函数
 * @return 0 成功
 */
static int dht11_app_init(void)
{
    rt_thread_t thread;  /* 线程句柄 */
    rt_err_t ret;        /* 返回值 */


    /* 初始化 DHT11 设备 */
    ret = dht11_init(&g_dht11_dev, DHT11_DATA_PIN);
    if (ret != RT_EOK)
    {
        /* 初始化失败，打印错误信息 */
        rt_kprintf("[DHT11] Init failed!\n");
        return -1;
    }

    /* 等待 1 秒，让 DHT11 传感器进入稳定状态（上电后不稳定期） */
    rt_kprintf("[DHT11] 等待传感器稳定...\n");
    rt_thread_mdelay(1000);

    /* 创建 DHT11 读取线程 */
    thread = rt_thread_create("dht11",              /* 线程名称 */
                              dht11_thread_entry,   /* 线程入口函数 */
                              RT_NULL,              /* 线程参数（无） */
                              1024,                 /* 线程栈大小（1024 字节） */
                              20,                   /* 线程优先级（20，数值越小优先级越高） */
                              10);                  /* 线程时间片（10 个系统节拍） */

    /* 检查线程是否创建成功 */
    if (thread != RT_NULL)
    {
        /* 启动线程 */
        rt_thread_startup(thread);
        rt_kprintf("[DHT11] Application initialized successfully!\n\n");
    }
    else
    {
        /* 线程创建失败 */
        rt_kprintf("[DHT11] Thread create failed!\n");
        return -1;
    }

    return 0;  /* 返回成功 */
}

/* 使用 INIT_APP_EXPORT 宏，在系统启动时自动初始化 DHT11 应用 */
INIT_APP_EXPORT(dht11_app_init);

/**
 * @brief 获取当前温度
 * @return 温度值(摄氏度)
 */
rt_uint8_t dht11_get_temperature(void)
{
    return g_dht11_temperature;
}

/**
 * @brief 获取当前湿度
 * @return 湿度值(百分比)
 */
rt_uint8_t dht11_get_humidity(void)
{
    return g_dht11_humidity;
}

