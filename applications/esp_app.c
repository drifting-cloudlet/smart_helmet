#include "esp_app.h"
#include "mydefine.h"
#include <string.h>

/* 串口设备句柄 */
static rt_device_t esp_uart = RT_NULL;

/**
 * @brief 发送字符串到ESP模块
 */
void esp_send(const char *data)
{
    if (esp_uart && data) {
        rt_device_write(esp_uart, 0, data, strlen(data));
    }
}

/**
 * @brief ESP线程入口函数
 * @param parameter 线程参数（未使用）
 */
static void esp_thread_entry(void *parameter)
{
    char cmd[256];

    rt_kprintf("[ESP] Thread started!\n");

    /* 1. 复位模块 */
    esp_send("AT+RST\r\n");
    rt_thread_mdelay(2000);

    /* 2. 测试AT */
    esp_send("AT\r\n");
    rt_thread_mdelay(500);

    /* 3. 设置STA模式 */
    esp_send("AT+CWMODE=1\r\n");
    rt_thread_mdelay(500);

    /* 4. 连接WiFi */
    rt_snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_NAME, WIFI_PWD);
    esp_send(cmd);
    rt_thread_mdelay(5000);
    rt_kprintf("[ESP] WiFi connecting...\n");

    /* 5. 配置MQTT用户 */
    rt_snprintf(cmd, sizeof(cmd),
        "AT+MQTTUSERCFG=0,1,\"NULL\",\"%s\",\"%s\",0,0,\"\"\r\n",
        HUAWEI_MQTT_USERNAME, HUAWEI_MQTT_PWD);
    esp_send(cmd);
    rt_thread_mdelay(1000);

    /* 6. 配置MQTT ClientID */
    rt_snprintf(cmd, sizeof(cmd), "AT+MQTTCLIENTID=0,\"%s\"\r\n", HUAWEI_MQTT_ClientID);
    esp_send(cmd);
    rt_thread_mdelay(1000);

    /* 7. 连接MQTT服务器 */
    rt_snprintf(cmd, sizeof(cmd), "AT+MQTTCONN=0,\"%s\",%d,1\r\n",
        HUAWEI_MQTT_ADDRESS, HUAWEI_MQTT_PORT);
    esp_send(cmd);
    rt_thread_mdelay(3000);

    rt_kprintf("[ESP] MQTT connected!\n");

    /* 主循环 - 可以在这里周期性上报数据 */
    while (1)
    {
        rt_thread_mdelay(5000);
    }
}

/**
 * @brief ESP应用层初始化函数
 * @return 0 成功，-1 失败
 */
int esp_app_init(void)
{
    rt_thread_t thread;

    /* 查找并打开串口 */
    esp_uart = rt_device_find(ESP_UART_NAME);
    if (esp_uart == RT_NULL) {
        rt_kprintf("[ESP] %s not found!\n", ESP_UART_NAME);
        return -1;
    }
    rt_device_open(esp_uart, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    rt_kprintf("[ESP] uart opened\n");

    /* 等待模块稳定 */
    rt_kprintf("[ESP] 等待模块稳定...\n");
    rt_thread_mdelay(1000);

    /* 创建ESP线程 */
    thread = rt_thread_create("esp",
                              esp_thread_entry,
                              RT_NULL,
                              2048,
                              19,
                              10);

    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
        rt_kprintf("[ESP] Application initialized successfully!\n\n");
    }
    else
    {
        rt_kprintf("[ESP] Thread create failed!\n");
        return -1;
    }

    return 0;
}

/**
 * @brief 上报基础数据
 */
//int esp_report_basic(int spo2, float density, int hr, int fall, int collision)
//{
//    char cmd[512];

//    rt_snprintf(cmd, sizeof(cmd),
//        "AT+MQTTPUB=0,\"$oc/devices/%s/sys/properties/report\","
//        "\"{\\\"services\\\":[{\\\"service_id\\\":\\\"BasicData\\\","
//        "\\\"properties\\\":{\\\"spO2\\\":%d,\\\"density\\\":%.2f,"
//        "\\\"heart_rate\\\":%d,\\\"fall_flag\\\":%d,\\\"collision_flag\\\":%d}}]}\",0,0\r\n",
//        HUAWEI_MQTT_USERNAME, spo2, density, hr, fall, collision);

//    esp_send(cmd);
//    rt_thread_mdelay(500);
//    return 0;
//}

/**
 * @brief 上报环境数据
 */
int esp_report(float density, int hr, int temp, int humi)
{
    char cmd[512];

    rt_snprintf(cmd, sizeof(cmd),
        "AT+MQTTPUB=0,\"$oc/devices/%s/sys/properties/report\","
        "\"{\\\"services\\\":[{\\\"service_id\\\":\\\"BasedData\\\","
        "\\\"properties\\\":{\\\"density\\\":%.2f,\\\"heart_rate\\\":%d,"
        "\\\"temperature\\\":%d,\\\"humidity\\\":%d}}]}\",0,0\r\n",
        HUAWEI_MQTT_USERNAME, density, hr, temp, humi);

    esp_send(cmd);
    rt_thread_mdelay(500);
    return 0;
}

/* 使用 INIT_APP_EXPORT 宏，在系统启动时自动初始化 */
INIT_APP_EXPORT(esp_app_init);
