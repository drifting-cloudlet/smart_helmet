#ifndef MYDEFINE_H
#define MYDEFINE_H

#include <math.h>
#include <rtthread.h>
#include <board.h>
#include <rtdevice.h>
#include "drv_pin.h"

/*
 * 注意：不要在这里包含应用层的驱动头文件（如 drv_mq2.h, drv_dht11.h）
 * 因为这些驱动头文件也会包含 mydefine.h，会造成循环包含问题
 * 应该在各个应用文件中单独包含所需的驱动头文件
 */

#endif

