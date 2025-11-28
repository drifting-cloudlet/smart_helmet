/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C 总线扫描工具 - 用于诊断 I2C 总线问题
 */

#include <rtthread.h>
#include <rtdevice.h>

/**
 * @brief I2C 总线扫描命令
 * @usage: i2c_scan i2c1
 */
static int i2c_scan(int argc, char *argv[])
{
    struct rt_i2c_bus_device *i2c_bus;
    rt_uint8_t addr;
    struct rt_i2c_msg msgs;
    rt_uint8_t buf[1];
    int found = 0;

    if (argc != 2)
    {
        rt_kprintf("Usage: i2c_scan <i2c_bus_name>\n");
        rt_kprintf("Example: i2c_scan i2c1\n");
        return -1;
    }

    /* 查找 I2C 总线 */
    i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(argv[1]);
    if (i2c_bus == RT_NULL)
    {
        rt_kprintf("[ERROR] I2C bus '%s' not found!\n", argv[1]);
        rt_kprintf("Available I2C buses:\n");
        rt_kprintf("  - i2c0\n");
        rt_kprintf("  - i2c1\n");
        return -1;
    }

    rt_kprintf("\n[I2C Scan] Scanning I2C bus '%s'...\n", argv[1]);
    rt_kprintf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");

    /* 扫描所有可能的 7 位地址 (0x08-0x77) */
    for (addr = 0; addr < 0x80; addr++)
    {
        if (addr % 16 == 0)
        {
            rt_kprintf("%02x: ", addr);
        }

        /* 跳过保留地址 */
        if (addr < 0x08 || addr > 0x77)
        {
            rt_kprintf("   ");
            if ((addr + 1) % 16 == 0)
            {
                rt_kprintf("\n");
            }
            continue;
        }

        /* 尝试与设备通信（发送 0 字节数据） */
        msgs.addr  = addr;
        msgs.flags = RT_I2C_WR;
        msgs.buf   = buf;
        msgs.len   = 0;

        /* 使用短超时时间,避免扫描时间过长 */
        if (rt_i2c_transfer(i2c_bus, &msgs, 1) == 1)
        {
            rt_kprintf("%02x ", addr);
            found++;
        }
        else
        {
            rt_kprintf("-- ");
        }

        if ((addr + 1) % 16 == 0)
        {
            rt_kprintf("\n");
        }
    }

    rt_kprintf("\n[I2C Scan] Found %d device(s) on bus '%s'\n", found, argv[1]);

    if (found == 0)
    {
        rt_kprintf("\n[HINT] 可能的原因:\n");
        rt_kprintf("  1. I2C 引脚未正确配置\n");
        rt_kprintf("  2. I2C 总线缺少上拉电阻(通常需要 4.7kΩ)\n");
        rt_kprintf("  3. 从设备未上电或损坏\n");
        rt_kprintf("  4. SDA/SCL 接线错误或断开\n");
    }

    return 0;
}
MSH_CMD_EXPORT(i2c_scan, Scan I2C bus for devices);
