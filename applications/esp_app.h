#ifndef ESP_APP_H
#define ESP_APP_H

#include <rtthread.h>
#include <rtdevice.h>

/* Wi-Fi 配置 */
#define WIFI_NAME               "LP11"
#define WIFI_PWD                "123456aa"

/* 华为云 MQTT 配置 */
#define HUAWEI_MQTT_ADDRESS     "e8b7ac5772.st1.iotda-device.cn-east-3.myhuaweicloud.com"
#define HUAWEI_MQTT_USERNAME    "69258d7746c60374e3f77d7f_helmet"
#define HUAWEI_MQTT_PWD         "79d7444ad2a170c6b4ec6db962602e3f9ef7a6f3c1166daf8e5017773fb348ee"
#define HUAWEI_MQTT_ClientID    "69258d7746c60374e3f77d7f_helmet_0_0_2025112511"
#define HUAWEI_MQTT_PORT        1883

/* 串口设备名 */
#define ESP_UART_NAME           "uart1"

/* API */
int esp_init(void);
void esp_send(const char *data);
int esp_report_basic(int spo2, float density, int hr, int fall, int collision);
int esp_report(float density, int hr, int temp, int humi);

#endif /* ESP_APP_H */
