#ifndef MAX30102_APP_H
#define MAX30102_APP_H

#include "mydefine.h"
#include "drv_max30102.h"

/* 暴露最新读取的LED数据 */
extern rt_uint32_t g_max30102_red_led;
extern rt_uint32_t g_max30102_ir_led;
extern rt_uint32_t g_max30102_heart_rate;

/* 获取当前心率（简易估算值） */
rt_uint32_t max30102_get_heart_rate(void);

/* 获取红光LED原始值 */
rt_uint32_t max30102_get_red_led(void);

/* 获取红外LED原始值 */
rt_uint32_t max30102_get_ir_led(void);

#endif
