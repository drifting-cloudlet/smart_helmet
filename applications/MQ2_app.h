#ifndef MQ2_APP_H
#define MQ2_APP_H

#include "mydefine.h"
#include "drv_mq2.h"

/* 暴露MQ2设备对象供外部访问 */
extern mq2_device_t g_mq2_dev;

/* 获取当前甲烷浓度 */
float mq2_get_ch4ppm(void);

#endif

