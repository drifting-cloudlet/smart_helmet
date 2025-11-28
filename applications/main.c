/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 * Copyright (c) 2019-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-10-24     Magicoe      first version
 * 2020-01-10     Kevin/Karl   Add PS demo
 * 2020-09-21     supperthomas fix the main.c
 *
 */

#include <rtdevice.h>
#include "drv_pin.h"
#include "fsl_lpi2c.h"  // NXP SDK LPI2C header
#include "ATGM336H_app.h"  // GPS模块


int main(void)
{
    // 手动初始化GPS模块
//    atgm336h_app_init();

	return 0;
}

