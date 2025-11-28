/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-18     User         MAX30102 心率血氧传感器应用示例
 */

#include "mydefine.h"           // 包含通用定义头文件
#include "drv_max30102.h"       // 包含MAX30102驱动头文件

/* MAX30102 I2C 总线名称定义（根据实际硬件修改） */
#define MAX30102_I2C_BUS_NAME    "i2c0"

/* MAX30102 INT 中断引脚定义（P1_13 = 1*32+13 = 45） */
#define MAX30102_INT_PIN         ((1*32)+13)

/* 是否使用中断模式（0=禁用中断使用轮询；1=启用中断） */
#define USE_INTERRUPT_MODE       1

/* MAX30102 设备对象（全局静态变量，使用指针类型） */
static max30102_device_t *max30102_dev = RT_NULL;

/* 最新读取的LED数据（全局变量，供esp_app访问） */
rt_uint32_t g_max30102_red_led = 0;
rt_uint32_t g_max30102_ir_led = 0;
rt_uint32_t g_max30102_heart_rate = 0;

#if USE_INTERRUPT_MODE
/* 信号量，用于中断与线程之间的同步 */
static rt_sem_t max30102_sem = RT_NULL;

/**
 * @brief MAX30102 中断回调函数
 * @param args 中断回调参数（本例中未使用）
 */
static void max30102_int_callback(void *args)
{
    /* 在中断中释放信号量，通知读取线程有新数据可读 */
    rt_sem_release(max30102_sem);
}
#endif

/**
 * @brief MAX30102 读取线程入口函数
 * @param parameter 线程参数（本例中未使用）
 */
static void max30102_thread_entry(void *parameter)
{
    rt_uint32_t red_led;     // 存储红光LED数据（18位有效数据）
    rt_uint32_t ir_led;      // 存储红外LED数据（18位有效数据）
    rt_err_t result;         // 存储函数返回结果

    /* 打印线程启动信息 */
    rt_kprintf("[MAX30102] Thread started!\n");

#if USE_INTERRUPT_MODE
    rt_kprintf("[MAX30102] Running in INTERRUPT mode.\n");

    /* 线程主循环，等待中断触发 */
    while (1)
    {
        /* 等待中断信号量，永久等待直到中断触发 */
        /* 当MAX30102的INT引脚产生中断时，会释放信号量，线程被唤醒 */
        result = rt_sem_take(max30102_sem, RT_WAITING_FOREVER);
        if (result != RT_EOK)  // 如果获取信号量失败
        {
            /* 打印错误信息并继续等待 */
            rt_kprintf("[MAX30102] Semaphore take error!\n");
            continue;
        }

        /* 调用驱动函数从FIFO读取红光和红外光数据 */
        result = max30102_read_fifo(max30102_dev, &red_led, &ir_led);

        /* 根据读取结果进行处理 */
        if (result == RT_EOK)  // 如果读取成功
        {
            /* 更新全局变量 */
            g_max30102_red_led = red_led;
            g_max30102_ir_led = ir_led;
            /* 简易心率估算（实际应用需要更复杂的算法） */
            if (ir_led > 50000)
            {
                g_max30102_heart_rate = 75;  /* 检测到手指，假设正常心率 */
            }
            /* 打印红光和红外光的原始数据值 */
            rt_kprintf("[MAX30102] RED: %u, IR: %u\n", red_led, ir_led);
        }
        else  // 如果读取失败
        {
            /* 打印错误信息和错误代码 */
            rt_kprintf("[MAX30102] Read FIFO error! (error code: %d)\n", result);
        }
    }
#else
    rt_kprintf("[MAX30102] Running in POLLING mode (100ms interval).\n");

    /* 轮询模式主循环 */
    while (1)
    {
        /* 调用驱动函数从FIFO读取红光和红外光数据 */
        result = max30102_read_fifo(max30102_dev, &red_led, &ir_led);

        /* 根据读取结果进行处理 */
        if (result == RT_EOK)  // 如果读取成功
        {
            /* 更新全局变量 */
            g_max30102_red_led = red_led;
            g_max30102_ir_led = ir_led;
            /* 简易心率估算（实际应用需要更复杂的算法） */
            if (ir_led > 50000)
            {
                g_max30102_heart_rate = 75;  /* 检测到手指，假设正常心率 */
            }
            /* 打印红光和红外光的原始数据值 */
            rt_kprintf("[MAX30102] RED: %u, IR: %u\n", red_led, ir_led);
        }
        else  // 如果读取失败
        {
            /* 打印错误信息和错误代码 */
            rt_kprintf("[MAX30102] Read FIFO error! (error code: %d)\n", result);
        }

        /* 延时100毫秒后再次读取（根据实际采样率调整此值） */
        rt_thread_mdelay(100);
    }
#endif
}

/**
 * @brief MAX30102 应用层初始化函数
 * @return 0 表示成功，-1 表示失败
 */
static int max30102_app_init(void)
{
    rt_thread_t thread;  // 定义线程句柄变量
    rt_err_t ret;        // 存储返回值

    rt_kprintf("[MAX30102] Starting initialization...\n");

#if USE_INTERRUPT_MODE
    /* 创建二值信号量，用于中断与线程同步 */
    max30102_sem = rt_sem_create("max30102", 0, RT_IPC_FLAG_FIFO);
    if (max30102_sem == RT_NULL)  // 如果信号量创建失败
    {
        /* 打印错误信息 */
        rt_kprintf("[MAX30102] Semaphore create failed!\n");
        return -1;  // 返回错误代码
    }
    rt_kprintf("[MAX30102] Semaphore created successfully.\n");

    /* 配置INT引脚为输入模式，用于接收中断信号 */
    rt_kprintf("[MAX30102] Configuring pin P1_13 as input...\n");
    rt_pin_mode(MAX30102_INT_PIN, PIN_MODE_INPUT);
    rt_kprintf("[MAX30102] Pin mode set successfully.\n");

    /* 绑定中断回调函数，下降沿触发（MAX30102中断为低电平有效） */
    rt_kprintf("[MAX30102] Attaching IRQ handler (pin=%d)...\n", MAX30102_INT_PIN);
    ret = rt_pin_attach_irq(MAX30102_INT_PIN,                 // 中断引脚号
                            PIN_IRQ_MODE_FALLING,              // 下降沿触发模式
                            max30102_int_callback,             // 中断回调函数
                            RT_NULL);                          // 回调函数参数（无）
    rt_kprintf("[MAX30102] rt_pin_attach_irq returned: %d\n", ret);
    if (ret != RT_EOK)  // 如果绑定失败
    {
        /* 打印错误信息 */
        rt_kprintf("[MAX30102] Pin attach IRQ failed! Error code: %d\n", ret);
        rt_sem_delete(max30102_sem);  // 删除已创建的信号量
        return -1;  // 返回错误代码
    }
    rt_kprintf("[MAX30102] IRQ attached successfully.\n");

    /* 使能引脚中断，开始接收中断信号 */
    rt_kprintf("[MAX30102] Enabling IRQ...\n");
    ret = rt_pin_irq_enable(MAX30102_INT_PIN, PIN_IRQ_ENABLE);
    if (ret != RT_EOK)  // 如果使能失败
    {
        /* 打印错误信息 */
        rt_kprintf("[MAX30102] Pin IRQ enable failed! Error code: %d\n", ret);
        rt_pin_detach_irq(MAX30102_INT_PIN);  // 解除中断绑定
        rt_sem_delete(max30102_sem);          // 删除信号量
        return -1;  // 返回错误代码
    }
    rt_kprintf("[MAX30102] IRQ enabled successfully.\n");
#endif

    /* 调用驱动初始化函数，传入I2C总线名称 */
    rt_kprintf("[MAX30102] Initializing I2C device...\n");
    max30102_dev = max30102_init(MAX30102_I2C_BUS_NAME);
    if (max30102_dev == RT_NULL)  // 如果初始化失败（返回空指针）
    {
        /* 打印初始化失败信息 */
        rt_kprintf("[MAX30102] Device init failed!\n");
        rt_kprintf("[MAX30102] Please check I2C bus name and hardware connection.\n");
#if USE_INTERRUPT_MODE
        rt_pin_irq_enable(MAX30102_INT_PIN, PIN_IRQ_DISABLE);  // 禁用中断
        rt_pin_detach_irq(MAX30102_INT_PIN);                   // 解除中断绑定
        rt_sem_delete(max30102_sem);                           // 删除信号量
#endif
        return -1;  // 返回错误代码
    }
    rt_kprintf("[MAX30102] Device initialized successfully.\n");

    /* 等待500毫秒，让传感器进入稳定工作状态（上电后需要稳定时间） */
    rt_kprintf("[MAX30102] Waiting for sensor to stabilize...\n");
    rt_thread_mdelay(500);

    /* 创建MAX30102数据读取线程 */
    rt_kprintf("[MAX30102] Creating thread...\n");
    thread = rt_thread_create("max30102",              // 线程名称字符串
                              max30102_thread_entry,   // 线程入口函数指针
                              RT_NULL,                 // 线程参数（传递给入口函数，此处为空）
                              2048,                    // 线程栈大小（2048字节，I2C通信需要较大栈）
                              20,                      // 线程优先级（数值越小优先级越高）
                              10);                     // 线程时间片（10个系统时钟节拍）

    /* 检查线程是否创建成功 */
    if (thread != RT_NULL)  // 如果线程创建成功（返回非空句柄）
    {
        /* 启动线程，使其进入就绪状态 */
        rt_thread_startup(thread);
        rt_kprintf("[MAX30102] Application initialized successfully!\n");
#if USE_INTERRUPT_MODE
        rt_kprintf("[MAX30102] INT pin: P1_13, waiting for interrupt...\n\n");
#else
        rt_kprintf("[MAX30102] Polling mode active.\n\n");
#endif
    }
    else  // 如果线程创建失败
    {
        /* 打印线程创建失败信息 */
        rt_kprintf("[MAX30102] Thread create failed!\n");
        /* 释放已分配的所有资源 */
        max30102_deinit(max30102_dev);                         // 反初始化设备
#if USE_INTERRUPT_MODE
        rt_pin_irq_enable(MAX30102_INT_PIN, PIN_IRQ_DISABLE);  // 禁用中断
        rt_pin_detach_irq(MAX30102_INT_PIN);                   // 解除中断绑定
        rt_sem_delete(max30102_sem);                           // 删除信号量
#endif
        return -1;  // 返回错误代码
    }

    return 0;  // 返回成功代码
}

/* 使用INIT_APP_EXPORT宏在系统启动时自动调用初始化函数 */
//INIT_APP_EXPORT(max30102_app_init);




