/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-11     User         DHT11 温湿度传感器驱动实现
 */

#include "drv_dht11.h"
#include "drv_pin.h"

/* DHT11 时序常量定义 */
#define DHT11_START_SIGNAL_MS   20      /* 起始信号：拉低 20ms */
#define DHT11_WAIT_RESPONSE     30      /* 等待响应时间：30us */
#define DHT11_RESPONSE_TIMEOUT  200     /* 响应超时时间：200us */
#define DHT11_BIT_TIMEOUT       150     /* 位读取超时：150us */
#define CPU_DELAY_US_FACTOR     50      /* CPU 延时校准系数50 */

/* 微秒级延时函数 */
//static void dht11_delay_us(rt_uint32_t us)
//{
//    volatile rt_uint32_t count = us * CPU_DELAY_US_FACTOR;
//    while (count--)
//    {
//        __asm volatile("NOP");
//    }
//}

/* 等待引脚变为指定电平状态（带超时） */
static rt_err_t dht11_wait_for_level(rt_base_t pin, rt_uint8_t level, rt_uint32_t timeout_us)
{
    rt_uint32_t timeout_count = timeout_us * CPU_DELAY_US_FACTOR / 10;
    rt_uint32_t count = 0;

    while (rt_pin_read(pin) != level)
    {
        if (++count > timeout_count)
        {
            return -RT_ETIMEOUT;
        }
    }

    return RT_EOK;
}

/* 读取一个位的数据 */
static rt_int8_t dht11_read_bit(rt_base_t pin)
{
    rt_uint32_t high_time = 0;
    rt_uint32_t timeout_count = DHT11_BIT_TIMEOUT * CPU_DELAY_US_FACTOR / 10;

    /* 等待低电平结束 */
    if (dht11_wait_for_level(pin, PIN_HIGH, DHT11_BIT_TIMEOUT) != RT_EOK)
    {
        return -1;
    }
//		rt_hw_us_delay(40);
		
    /* 测量高电平持续时间 */
    while (rt_pin_read(pin) == PIN_HIGH)
    {
        if (++high_time > timeout_count)
        {
            return -1;
        }
    }

    /* 根据高电平持续时间判断位值（阈值40us） */
//    rt_uint32_t threshold = 40 * CPU_DELAY_US_FACTOR / 10;
		 rt_uint32_t threshold = 40;
//		rt_kprintf("high_time=%d, threshold=%d\n", high_time, threshold);
		
    return (high_time > threshold) ? 1 : 0;
//		if((high_time >= 23) && (high_time <= 27))
//		{
//			return 0;
//		}
//		else if((high_time >= 68) && (high_time <= 74))
//		{
//			return -1;
//		}
//		else
//			return -1;
}

/* 读取一个字节的数据 */
static rt_int16_t dht11_read_byte(rt_base_t pin)
{
    rt_uint8_t byte = 0;
    rt_int8_t bit;

    for (rt_uint8_t i = 0; i < 8; i++)
    {
        byte <<= 1;
        bit = dht11_read_bit(pin);
        if (bit < 0)
        {
            return -1;
        }
        byte |= bit;
    }

    return byte;
}

/* 初始化 DHT11 设备 */
rt_err_t dht11_init(dht11_device_t *dev, rt_base_t pin)
{
    dev->pin = pin;
    dev->humidity = 0;
    dev->temperature = 0;

    rt_pin_mode(pin, PIN_MODE_OUTPUT);
    rt_pin_write(pin, PIN_HIGH);

    return RT_EOK;
}

/* 读取 DHT11 温湿度数据 */
dht11_result_t dht11_read(dht11_device_t *dev, rt_uint8_t *temp, rt_uint8_t *humi)
{
    rt_int16_t data[5];
    rt_uint8_t checksum;
    rt_base_t pin = dev->pin;
    rt_base_t level;

    /* 发送起始信号 */
    rt_pin_mode(pin, PIN_MODE_OUTPUT);
    rt_pin_write(pin, PIN_LOW);
    rt_thread_mdelay(DHT11_START_SIGNAL_MS);

    /* 关闭中断，确保时序精确 */
    level = rt_hw_interrupt_disable();

    rt_pin_write(pin, PIN_HIGH);
//    dht11_delay_us(DHT11_WAIT_RESPONSE);
		rt_hw_us_delay(DHT11_WAIT_RESPONSE);
    /* 切换为输入模式，等待 DHT11 响应 */
    rt_pin_mode(pin, PIN_MODE_INPUT_PULLUP);

    /* 等待响应信号 */
    if (dht11_wait_for_level(pin, PIN_LOW, DHT11_RESPONSE_TIMEOUT) != RT_EOK ||
        dht11_wait_for_level(pin, PIN_HIGH, DHT11_RESPONSE_TIMEOUT) != RT_EOK ||
        dht11_wait_for_level(pin, PIN_LOW, DHT11_RESPONSE_TIMEOUT) != RT_EOK)
    {
        rt_hw_interrupt_enable(level);
        return DHT11_ERROR_TIMEOUT;
    }

    /* 读取 5 个字节数据 */
    for (rt_uint8_t i = 0; i < 5; i++)
    {
        data[i] = dht11_read_byte(pin);
        if (data[i] < 0)
        {
            rt_hw_interrupt_enable(level);
            return DHT11_ERROR_TIMEOUT;
        }
    }

    /* 恢复中断 */
    rt_hw_interrupt_enable(level);

    /* 校验数据 */
    checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
    if (checksum != data[4])
    {
        return DHT11_ERROR_CHECKSUM;
    }

    /* 提取温湿度数据 */
    *humi = data[0];
    *temp = data[2];

    dev->humidity = *humi;
    dev->temperature = *temp;

    return DHT11_OK;
}
