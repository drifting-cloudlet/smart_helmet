/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-18     User         MAX30102 心率血氧传感器驱动实现
 */

#include "drv_max30102.h"

/* ================================ 私有函数声明 ================================ */

/**
 * @brief 通过 I2C 总线写入 MAX30102 寄存器
 * @param dev MAX30102 设备句柄
 * @param reg 寄存器地址
 * @param data 要写入的数据
 * @return rt_err_t RT_EOK 成功，-RT_ERROR 失败
 */
static rt_err_t _max30102_write_reg(max30102_device_t *dev, rt_uint8_t reg, rt_uint8_t data);

/**
 * @brief 通过 I2C 总线读取 MAX30102 寄存器
 * @param dev MAX30102 设备句柄
 * @param reg 寄存器地址
 * @param data 数据指针（输出参数）
 * @return rt_err_t RT_EOK 成功，-RT_ERROR 失败
 */
static rt_err_t _max30102_read_reg(max30102_device_t *dev, rt_uint8_t reg, rt_uint8_t *data);

/* ================================ 私有函数实现 ================================ */

/**
 * @brief 通过 I2C 总线写入 MAX30102 寄存器
 * @note 此函数不包含互斥锁保护，需要在调用前确保线程安全
 */
static rt_err_t _max30102_write_reg(max30102_device_t *dev, rt_uint8_t reg, rt_uint8_t data)
{
    rt_uint8_t buf[2];              /* I2C 发送缓冲区：[寄存器地址, 数据] */
    struct rt_i2c_msg msgs;         /* I2C 消息结构体 */

    /* 参数有效性检查 */
    if (dev == RT_NULL || dev->i2c_bus == RT_NULL)
    {
        return -RT_ERROR;           /* 设备句柄或I2C总线无效 */
    }

    /* 构建发送数据：第一个字节是寄存器地址，第二个字节是数据 */
    buf[0] = reg;                   /* 寄存器地址 */
    buf[1] = data;                  /* 要写入的数据 */

    /* 配置 I2C 消息结构体 */
    msgs.addr  = dev->addr;         /* 设置 MAX30102 的 I2C 地址（7位） */
    msgs.flags = RT_I2C_WR;         /* 设置为写操作标志 */
    msgs.buf   = buf;               /* 指向发送缓冲区 */
    msgs.len   = 2;                 /* 发送长度：寄存器地址(1) + 数据(1) = 2 字节 */

    /* 发起 I2C 传输，返回值为成功传输的消息数量 */
    if (rt_i2c_transfer(dev->i2c_bus, &msgs, 1) == 1)
    {
        return RT_EOK;              /* 传输成功 */
    }
    else
    {
        return -RT_ERROR;           /* 传输失败 */
    }
}

/**
 * @brief 通过 I2C 总线读取 MAX30102 寄存器
 * @note 此函数不包含互斥锁保护，需要在调用前确保线程安全
 */
static rt_err_t _max30102_read_reg(max30102_device_t *dev, rt_uint8_t reg, rt_uint8_t *data)
{
    struct rt_i2c_msg msgs[2];      /* I2C 消息数组：先写寄存器地址，再读数据 */
    rt_size_t ret;

    /* 参数有效性检查 */
    if (dev == RT_NULL || dev->i2c_bus == RT_NULL || data == RT_NULL)
    {
        return -RT_ERROR;           /* 参数无效 */
    }

    /* 第一个消息：写入寄存器地址 */
    msgs[0].addr  = dev->addr;      /* MAX30102 的 I2C 地址 */
    msgs[0].flags = RT_I2C_WR;      /* 写操作标志 */
    msgs[0].buf   = &reg;           /* 指向寄存器地址 */
    msgs[0].len   = 1;              /* 发送 1 字节寄存器地址 */

    /* 第二个消息：读取寄存器数据 */
    msgs[1].addr  = dev->addr;      /* MAX30102 的 I2C 地址 */
    msgs[1].flags = RT_I2C_RD;      /* 读操作标志 */
    msgs[1].buf   = data;           /* 指向接收数据缓冲区 */
    msgs[1].len   = 1;              /* 读取 1 字节数据 */

    /* 发起 I2C 传输，返回值为成功传输的消息数量（应该为2） */
    ret = rt_i2c_transfer(dev->i2c_bus, msgs, 2);

    if (ret == 2)
    {
        return RT_EOK;              /* 传输成功 */
    }
    else
    {
        return -RT_ERROR;           /* 传输失败 */
    }
}

/* ================================ 公共 API 函数实现 ================================ */

/**
 * @brief 初始化 MAX30102 设备
 * @param i2c_bus_name I2C 总线设备名称（例如 "i2c0"）
 * @return max30102_device_t* 设备句柄，失败返回 RT_NULL
 */

max30102_device_t *max30102_init(const char *i2c_bus_name)
{
    max30102_device_t *dev = RT_NULL;   /* 设备句柄 */
    rt_uint8_t part_id = 0;             /* 芯片ID，用于验证设备 */

    rt_kprintf("[MAX30102] 开始初始化，I2C总线名称: %s\n", i2c_bus_name);

    /* 分配设备结构体内存 */
    dev = (max30102_device_t *)rt_malloc(sizeof(max30102_device_t));
    if (dev == RT_NULL)
    {
        rt_kprintf("[MAX30102] 内存分配失败\n");
        return RT_NULL;                 /* 内存分配失败 */
    }

    /* 清零设备结构体 */
    rt_memset(dev, 0, sizeof(max30102_device_t));

    /* 查找 I2C 总线设备 */
    dev->i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(i2c_bus_name);
    if (dev->i2c_bus == RT_NULL)
    {
        rt_kprintf("[MAX30102] 未找到 I2C 总线: %s\n", i2c_bus_name);
        rt_free(dev);                   /* 释放已分配的内存 */
        return RT_NULL;                 /* I2C 总线未找到 */
    }

    /* 打开 I2C 设备（重要：在使用 I2C 前必须先打开） */
    if (rt_device_open((rt_device_t)dev->i2c_bus, RT_DEVICE_FLAG_RDWR) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 打开I2C设备失败\n");
        rt_free(dev);
        return RT_NULL;
    }

    /* 创建互斥锁，用于多线程保护 */
    /* 互斥锁名称不能超过RT_NAME_MAX（通常为8），因此使用缩写 */
    dev->lock = rt_mutex_create("max3010", RT_IPC_FLAG_FIFO);
    if (dev->lock == RT_NULL)
    {
        rt_kprintf("[MAX30102] 互斥锁创建失败\n");
        rt_free(dev);                   /* 释放已分配的内存 */
        return RT_NULL;                 /* 互斥锁创建失败 */
    }

    /* 设置设备参数 */
    dev->addr = MAX30102_I2C_ADDR;      /* 设置 I2C 地址（7位格式：0x57） */
    dev->initialized = RT_FALSE;        /* 初始化标志暂时设为未完成 */

    /* 读取芯片 ID 以验证设备连接 */
    if (_max30102_read_reg(dev, REG_PART_ID, &part_id) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 读取芯片ID失败，请检查硬件连接\n");
        rt_device_close((rt_device_t)dev->i2c_bus);  /* 关闭I2C设备 */
        rt_mutex_delete(dev->lock);     /* 删除互斥锁 */
        rt_free(dev);                   /* 释放内存 */
        return RT_NULL;                 /* 通信失败 */
    }

    /* 验证芯片 ID（MAX30102 的 PART_ID 应该是 0x15） */
    if (part_id != 0x15)
    {
        rt_kprintf("[MAX30102] 芯片ID错误: 0x%02X (期望: 0x15)\n", part_id);
        rt_device_close((rt_device_t)dev->i2c_bus);  /* 关闭I2C设备 */
        rt_mutex_delete(dev->lock);     /* 删除互斥锁 */
        rt_free(dev);                   /* 释放内存 */
        return RT_NULL;                 /* 芯片ID不匹配 */
    }

    rt_kprintf("[MAX30102] 检测到芯片，ID: 0x%02X\n", part_id);

    /* ================ 开始配置 MAX30102 寄存器 ================ */

    /* 配置中断使能寄存器1：使能 A_FULL 和 PPG_RDY 中断 */
    /* 0xC0 = 0b11000000：bit7=A_FULL_EN(FIFO几乎满中断), bit6=PPG_RDY_EN(新数据就绪中断) */
    if (_max30102_write_reg(dev, REG_INTR_ENABLE_1, 0xC0) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 配置中断使能寄存器1失败\n");
        goto _init_failed;              /* 跳转到失败处理 */
    }

    /* 配置中断使能寄存器2：禁用其他所有中断 */
    /* 0x00：禁用温度中断、接近中断等 */
    if (_max30102_write_reg(dev, REG_INTR_ENABLE_2, 0x00) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 配置中断使能寄存器2失败\n");
        goto _init_failed;
    }

    /* 复位 FIFO 写指针为 0 */
    if (_max30102_write_reg(dev, REG_FIFO_WR_PTR, 0x00) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 复位FIFO写指针失败\n");
        goto _init_failed;
    }

    /* 清空 FIFO 溢出计数器 */
    if (_max30102_write_reg(dev, REG_OVF_COUNTER, 0x00) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 清空溢出计数器失败\n");
        goto _init_failed;
    }

    /* 复位 FIFO 读指针为 0 */
    if (_max30102_write_reg(dev, REG_FIFO_RD_PTR, 0x00) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 复位FIFO读指针失败\n");
        goto _init_failed;
    }

    /* 配置 FIFO 寄存器 */
    /* 0x0F = 0b00001111：
     * bit[7:5]=000(样本平均数=1，无平均)
     * bit4=0(FIFO满时禁用滚动覆盖)
     * bit[3:0]=1111(FIFO几乎满阈值=17个样本，32-15=17) */
    if (_max30102_write_reg(dev, REG_FIFO_CONFIG, 0x0F) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 配置FIFO失败\n");
        goto _init_failed;
    }

    /* 配置工作模式寄存器 */
    /* 0x03 = 0b00000011：SpO2 模式（红光LED + 红外LED双通道工作）
     * 0x02：仅红光模式（心率监测）
     * 0x07：多LED模式（可同时使用红光、红外、绿光等） */
    if (_max30102_write_reg(dev, REG_MODE_CONFIG, 0x03) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 配置工作模式失败\n");
        goto _init_failed;
    }

    /* 配置 SpO2 寄存器 */
    /* 0x27 = 0b00100111：
     * bit[6:5]=01(ADC测量范围=4096nA，满量程)
     * bit[4:2]=001(采样率=100Hz，每10ms一个样本)
     * bit[1:0]=11(LED脉冲宽度=411us，ADC分辨率=18位) */
    if (_max30102_write_reg(dev, REG_SPO2_CONFIG, 0x27) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 配置SpO2参数失败\n");
        goto _init_failed;
    }

    /* 配置 LED1（红光）脉冲幅度 */
    /* 0x24 = 36：LED电流 = 36 × 0.2mA = 7.2mA
     * 取值范围：0x00-0xFF（0-51mA），推荐值 0x1F-0x3F */
    if (_max30102_write_reg(dev, REG_LED1_PA, 0x24) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 配置LED1电流失败\n");
        goto _init_failed;
    }

    /* 配置 LED2（红外）脉冲幅度 */
    /* 0x24 = 36：LED电流 = 36 × 0.2mA = 7.2mA */
    if (_max30102_write_reg(dev, REG_LED2_PA, 0x24) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 配置LED2电流失败\n");
        goto _init_failed;
    }

    /* 配置导频 LED 脉冲幅度（用于接近检测） */
    /* 0x7F = 127：导频LED电流 = 127 × 0.2mA = 25.4mA */
    if (_max30102_write_reg(dev, REG_PILOT_PA, 0x7F) != RT_EOK)
    {
        rt_kprintf("[MAX30102] 配置导频LED电流失败\n");
        goto _init_failed;
    }

    /* 所有配置完成，设置初始化标志 */
    dev->initialized = RT_TRUE;
    rt_kprintf("[MAX30102] 初始化成功，工作在 SpO2 模式，采样率 100Hz\n");

    return dev;                         /* 返回设备句柄 */

_init_failed:
    /* 初始化失败，清理资源 */
    rt_device_close((rt_device_t)dev->i2c_bus);  /* 关闭I2C设备 */
    rt_mutex_delete(dev->lock);         /* 删除互斥锁 */
    rt_free(dev);                       /* 释放内存 */
    return RT_NULL;                     /* 返回空指针表示失败 */
}

/**
 * @brief 反初始化 MAX30102 设备，释放资源
 * @param dev MAX30102 设备句柄
 * @return rt_err_t RT_EOK 成功，-RT_ERROR 失败
 */
rt_err_t max30102_deinit(max30102_device_t *dev)
{
    /* 参数有效性检查 */
    if (dev == RT_NULL)
    {
        return -RT_ERROR;               /* 设备句柄无效 */
    }

    /* 关闭 I2C 设备 */
    if (dev->i2c_bus != RT_NULL)
    {
        rt_device_close((rt_device_t)dev->i2c_bus);
    }

    /* 删除互斥锁 */
    if (dev->lock != RT_NULL)
    {
        rt_mutex_delete(dev->lock);     /* 释放互斥锁资源 */
    }

    /* 释放设备结构体内存 */
    rt_free(dev);

    rt_kprintf("[MAX30102] 设备已释放\n");

    return RT_EOK;                      /* 反初始化成功 */
}

/**
 * @brief 从 FIFO 读取红光和红外光数据
 * @param dev MAX30102 设备句柄
 * @param red_led 红光LED数据指针（输出参数，18位有效数据）
 * @param ir_led 红外LED数据指针（输出参数，18位有效数据）
 * @return rt_err_t RT_EOK 成功，-RT_ERROR 失败
 */
rt_err_t max30102_read_fifo(max30102_device_t *dev, rt_uint32_t *red_led, rt_uint32_t *ir_led)
{
    rt_uint8_t buf[6];                  /* 数据缓冲区：红光(3字节) + 红外(3字节) */
    struct rt_i2c_msg msgs[2];          /* I2C 消息数组 */
    rt_uint8_t reg = REG_FIFO_DATA;     /* FIFO 数据寄存器地址 */
    rt_uint8_t temp;                    /* 临时变量，用于读取中断状态 */
    rt_err_t result = RT_EOK;           /* 操作结果 */

    /* 参数有效性检查 */
    if (dev == RT_NULL || red_led == RT_NULL || ir_led == RT_NULL)
    {
        return -RT_ERROR;               /* 参数无效 */
    }

    /* 检查设备是否已初始化 */
    if (dev->initialized == RT_FALSE)
    {
        rt_kprintf("[MAX30102] 设备未初始化\n");
        return -RT_ERROR;
    }

    /* 获取互斥锁，确保线程安全 */
    rt_mutex_take(dev->lock, RT_WAITING_FOREVER);

    /* 读取并清除中断状态寄存器1（读取后自动清除） */
    _max30102_read_reg(dev, REG_INTR_STATUS_1, &temp);

    /* 读取并清除中断状态寄存器2（读取后自动清除） */
    _max30102_read_reg(dev, REG_INTR_STATUS_2, &temp);

    /* ================ 读取 FIFO 数据（6字节连续读取） ================ */

    /* 第一个消息：写入 FIFO 数据寄存器地址 */
    msgs[0].addr  = dev->addr;          /* MAX30102 的 I2C 地址 */
    msgs[0].flags = RT_I2C_WR;          /* 写操作标志 */
    msgs[0].buf   = &reg;               /* 指向寄存器地址 */
    msgs[0].len   = 1;                  /* 发送 1 字节寄存器地址 */

    /* 第二个消息：连续读取 6 字节 FIFO 数据 */
    msgs[1].addr  = dev->addr;          /* MAX30102 的 I2C 地址 */
    msgs[1].flags = RT_I2C_RD;          /* 读操作标志 */
    msgs[1].buf   = buf;                /* 指向接收缓冲区 */
    msgs[1].len   = 6;                  /* 读取 6 字节：红光3字节 + 红外3字节 */

    /* 发起 I2C 传输 */
    if (rt_i2c_transfer(dev->i2c_bus, msgs, 2) != 2)
    {
        rt_kprintf("[MAX30102] FIFO数据读取失败\n");
        result = -RT_ERROR;             /* 传输失败 */
        goto _read_exit;                /* 跳转到退出处理 */
    }

    /* ================ 解析 FIFO 数据 ================ */
    /* MAX30102 每个通道数据为 18 位，存储在 3 个字节中：
     * 字节格式（大端序）：[高8位] [中8位] [低2位+保留6位]
     * 数据组织：buf[0~2]=红光数据，buf[3~5]=红外数据 */

    /* 解析红光 LED 数据（前3个字节） */
    *red_led = 0;                                       /* 清零 */
    *red_led |= ((rt_uint32_t)buf[0] << 16);           /* 高8位：左移16位 */
    *red_led |= ((rt_uint32_t)buf[1] << 8);            /* 中8位：左移8位 */
    *red_led |= ((rt_uint32_t)buf[2]);                 /* 低8位：直接赋值 */
    *red_led &= 0x03FFFF;                              /* 屏蔽高6位，保留低18位有效数据 */

    /* 解析红外 LED 数据（后3个字节） */
    *ir_led = 0;                                        /* 清零 */
    *ir_led |= ((rt_uint32_t)buf[3] << 16);            /* 高8位：左移16位 */
    *ir_led |= ((rt_uint32_t)buf[4] << 8);             /* 中8位：左移8位 */
    *ir_led |= ((rt_uint32_t)buf[5]);                  /* 低8位：直接赋值 */
    *ir_led &= 0x03FFFF;                               /* 屏蔽高6位，保留低18位有效数据 */

_read_exit:
    /* 释放互斥锁 */
    rt_mutex_release(dev->lock);

    return result;                      /* 返回操作结果 */
}

/**
 * @brief 软件复位 MAX30102
 * @param dev MAX30102 设备句柄
 * @return rt_err_t RT_EOK 成功，-RT_ERROR 失败
 */
rt_err_t max30102_reset(max30102_device_t *dev)
{
    rt_err_t result;

    /* 参数有效性检查 */
    if (dev == RT_NULL)
    {
        return -RT_ERROR;               /* 设备句柄无效 */
    }

    /* 获取互斥锁 */
    rt_mutex_take(dev->lock, RT_WAITING_FOREVER);

    /* 向模式配置寄存器写入复位位 */
    /* 0x40 = 0b01000000：bit6 为复位位，写入1触发软件复位 */
    result = _max30102_write_reg(dev, REG_MODE_CONFIG, 0x40);

    /* 释放互斥锁 */
    rt_mutex_release(dev->lock);

    if (result == RT_EOK)
    {
        rt_kprintf("[MAX30102] 软件复位成功\n");
        dev->initialized = RT_FALSE;    /* 复位后需要重新初始化 */
    }
    else
    {
        rt_kprintf("[MAX30102] 软件复位失败\n");
    }

    return result;                      /* 返回操作结果 */
}

/**
 * @brief 读取 MAX30102 寄存器（带互斥锁保护的公共接口）
 * @param dev MAX30102 设备句柄
 * @param reg 寄存器地址
 * @param data 数据指针（输出参数）
 * @return rt_err_t RT_EOK 成功，-RT_ERROR 失败
 */
rt_err_t max30102_read_reg(max30102_device_t *dev, rt_uint8_t reg, rt_uint8_t *data)
{
    rt_err_t result;

    /* 参数有效性检查 */
    if (dev == RT_NULL || data == RT_NULL)
    {
        return -RT_ERROR;               /* 参数无效 */
    }

    /* 获取互斥锁，确保线程安全 */
    rt_mutex_take(dev->lock, RT_WAITING_FOREVER);

    /* 调用内部读寄存器函数 */
    result = _max30102_read_reg(dev, reg, data);

    /* 释放互斥锁 */
    rt_mutex_release(dev->lock);

    return result;                      /* 返回操作结果 */
}

/**
 * @brief 写入 MAX30102 寄存器（带互斥锁保护的公共接口）
 * @param dev MAX30102 设备句柄
 * @param reg 寄存器地址
 * @param data 要写入的数据
 * @return rt_err_t RT_EOK 成功，-RT_ERROR 失败
 */
rt_err_t max30102_write_reg(max30102_device_t *dev, rt_uint8_t reg, rt_uint8_t data)
{
    rt_err_t result;

    /* 参数有效性检查 */
    if (dev == RT_NULL)
    {
        return -RT_ERROR;               /* 设备句柄无效 */
    }

    /* 获取互斥锁，确保线程安全 */
    rt_mutex_take(dev->lock, RT_WAITING_FOREVER);

    /* 调用内部写寄存器函数 */
    result = _max30102_write_reg(dev, reg, data);

    /* 释放互斥锁 */
    rt_mutex_release(dev->lock);

    return result;                      /* 返回操作结果 */
}
