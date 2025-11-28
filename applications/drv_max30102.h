/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-18     User         MAX30102 心率血氧传感器驱动头文件
 */

#ifndef DRV_MAX30102_H
#define DRV_MAX30102_H

#include <rtthread.h>
#include <rtdevice.h>

/* MAX30102 I2C 设备地址（7位地址格式） */
#define MAX30102_I2C_ADDR           0x57    /* 7位地址：0xAE >> 1 = 0x57 */

/* MAX30102 寄存器地址定义 */
#define REG_INTR_STATUS_1           0x00    /* 中断状态寄存器1 */
#define REG_INTR_STATUS_2           0x01    /* 中断状态寄存器2 */
#define REG_INTR_ENABLE_1           0x02    /* 中断使能寄存器1 */
#define REG_INTR_ENABLE_2           0x03    /* 中断使能寄存器2 */
#define REG_FIFO_WR_PTR             0x04    /* FIFO 写指针寄存器 */
#define REG_OVF_COUNTER             0x05    /* FIFO 溢出计数器寄存器 */
#define REG_FIFO_RD_PTR             0x06    /* FIFO 读指针寄存器 */
#define REG_FIFO_DATA               0x07    /* FIFO 数据寄存器 */
#define REG_FIFO_CONFIG             0x08    /* FIFO 配置寄存器 */
#define REG_MODE_CONFIG             0x09    /* 模式配置寄存器 */
#define REG_SPO2_CONFIG             0x0A    /* SpO2 配置寄存器 */
#define REG_LED1_PA                 0x0C    /* LED1 脉冲幅度寄存器（红光LED） */
#define REG_LED2_PA                 0x0D    /* LED2 脉冲幅度寄存器（红外LED） */
#define REG_PILOT_PA                0x10    /* 导频LED脉冲幅度寄存器 */
#define REG_MULTI_LED_CTRL1         0x11    /* 多LED控制寄存器1 */
#define REG_MULTI_LED_CTRL2         0x12    /* 多LED控制寄存器2 */
#define REG_TEMP_INTR               0x1F    /* 温度传感器整数部分寄存器 */
#define REG_TEMP_FRAC               0x20    /* 温度传感器小数部分寄存器 */
#define REG_TEMP_CONFIG             0x21    /* 温度配置寄存器 */
#define REG_PROX_INT_THRESH         0x30    /* 接近中断阈值寄存器 */
#define REG_REV_ID                  0xFE    /* 版本ID寄存器 */
#define REG_PART_ID                 0xFF    /* 芯片ID寄存器（应该读到0x15） */

/* MAX30102 设备结构体 */
typedef struct
{
    struct rt_i2c_bus_device *i2c_bus; /* I2C 总线设备句柄 */
    rt_mutex_t lock;                    /* 互斥锁，用于多线程保护 */
    rt_uint8_t addr;                    /* I2C 设备地址（7位） */
    rt_bool_t initialized;              /* 初始化标志位 */
} max30102_device_t;

/* MAX30102 操作结果枚举 */
typedef enum
{
    MAX30102_OK = 0,                    /* 操作成功 */
    MAX30102_ERROR_TIMEOUT,             /* 超时错误 */
    MAX30102_ERROR_COMM,                /* 通信错误 */
    MAX30102_ERROR_PARAM                /* 参数错误 */
} max30102_result_t;

/**
 * @brief 初始化 MAX30102 设备
 * @param i2c_bus_name I2C 总线设备名称（例如 "i2c0"）
 * @return max30102_device_t* 设备句柄，失败返回 RT_NULL
 */
max30102_device_t *max30102_init(const char *i2c_bus_name);

/**
 * @brief 反初始化 MAX30102 设备，释放资源
 * @param dev MAX30102 设备句柄
 * @return rt_err_t RT_EOK 成功，其他值失败
 */
rt_err_t max30102_deinit(max30102_device_t *dev);

/**
 * @brief 从 FIFO 读取红光和红外光数据
 * @param dev MAX30102 设备句柄
 * @param red_led 红光LED数据指针（输出参数，18位有效数据）
 * @param ir_led 红外LED数据指针（输出参数，18位有效数据）
 * @return rt_err_t RT_EOK 成功，其他值失败
 */
rt_err_t max30102_read_fifo(max30102_device_t *dev, rt_uint32_t *red_led, rt_uint32_t *ir_led);

/**
 * @brief 软件复位 MAX30102
 * @param dev MAX30102 设备句柄
 * @return rt_err_t RT_EOK 成功，其他值失败
 */
rt_err_t max30102_reset(max30102_device_t *dev);

/**
 * @brief 读取 MAX30102 寄存器
 * @param dev MAX30102 设备句柄
 * @param reg 寄存器地址
 * @param data 数据指针（输出参数）
 * @return rt_err_t RT_EOK 成功，其他值失败
 */
rt_err_t max30102_read_reg(max30102_device_t *dev, rt_uint8_t reg, rt_uint8_t *data);

/**
 * @brief 写入 MAX30102 寄存器
 * @param dev MAX30102 设备句柄
 * @param reg 寄存器地址
 * @param data 要写入的数据
 * @return rt_err_t RT_EOK 成功，其他值失败
 */
rt_err_t max30102_write_reg(max30102_device_t *dev, rt_uint8_t reg, rt_uint8_t data);

#endif /* DRV_MAX30102_H */
