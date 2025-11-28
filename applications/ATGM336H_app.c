#include "ATGM336H_app.h"
#include "uart_app.h"
#include <string.h>
//RT_NAME_MAX
uint16_t point1 = 0; // 用于记录接收到的数据长度
float longitude;     // 用于存储经度
float latitude;      // 用于存储纬度

// 定义保存GPS数据的全局结构体实例
_SaveData Save_Data;
// 定义存储经纬度数据的全局结构体实例
LatitudeAndLongitude_s g_LatAndLongData =
{
    .E_W = 0,
    .N_S = 0,
    .latitude = 0.0,
    .longitude = 0.0
};

// 用于接收缓冲区及逐字节接收
char USART_RX_BUF[USART_REC_LEN];
uint8_t uart_A_RX_Buff;

// RT-Thread串口设备句柄
static rt_device_t gps_serial = RT_NULL;   // uart2 用于GPS模块
static rt_device_t debug_serial = RT_NULL; // uart0 用于调试打印

/**
 * @brief   串口接收完成回调函数（RT-Thread版本）
 */
// 用于统计接收字节数（调试用）
//static rt_uint32_t rx_byte_count = 0;

static rt_err_t uart_rx_callback(rt_device_t dev, rt_size_t size)
{
    // 判断是否为GPS串口（uart2）
    if (dev == gps_serial)
    {
        // 从串口读取一个字节
        if (rt_device_read(dev, -1, &uart_A_RX_Buff, 1) == 1)
        {
//            rx_byte_count++;  // 统计接收字节数
            // 判断是否收到帧头标志字符'$'
            if (uart_A_RX_Buff == '$')
            {
                point1 = 0; // 清空数据长度计数器
            }
            USART_RX_BUF[point1++] = uart_A_RX_Buff; // 存储接收到的数据

            // 检查是否收到的是GPRMC/GNRMC帧数据
            if (USART_RX_BUF[0] == '$' && USART_RX_BUF[4] == 'M' && USART_RX_BUF[5] == 'C')
            {
                // 如果收到换行符，表示一帧数据接收完成
                if (uart_A_RX_Buff == '\n')
                {
                    // 将GPS数据拷贝到结构体中并标记已接收到数据
                    memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);
                    memcpy(Save_Data.GPS_Buffer, USART_RX_BUF, point1);
                    Save_Data.isGetData = true; // 标记已经获取到GPS数据

                    // 清空缓冲区，准备接收下一帧数据
                    point1 = 0;
                    memset(USART_RX_BUF, 0, USART_REC_LEN);
                }
            }
            // 防止缓冲区溢出
            if (point1 >= USART_REC_LEN)
            {
                point1 = USART_REC_LEN;
            }
        }
    }

    return RT_EOK;
}

/**
 * @brief   GPS线程入口函数
 * @param   未使用线程参数
 */
static void atgm336h_entry(void *parameter)
{

    while(1)
    {
        parseGpsBuffer(); // 解析GPS数据
        printGpsBuffer(); // 打印GPS数据
        rt_thread_mdelay(100);
    }
}

/**
 * @brief   GPS应用层初始化
 * @return  0表示成功，-1表示失败
 */
int atgm336h_app_init(void)
{
    rt_thread_t thread;

    clrStruct(); // 清空结构体数据

    // 查找并打开GPS串口设备（uart2）
    gps_serial = rt_device_find("uart2");
    if (gps_serial == RT_NULL)
    {
        rt_kprintf("[ATGM336H] uart2 not found for GPS!\n");
        return -1;
    }

    // 以中断接收模式打开串口
    if (rt_device_open(gps_serial, RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        rt_kprintf("[ATGM336H] uart2 open failed!\n");
        return -1;
    }

    // 配置串口2波特率为9600（ATGM336H默认波特率）

    // 设置接收回调函数，准备接收数据
    rt_device_set_rx_indicate(gps_serial, uart_rx_callback);

    // 查找调试串口（uart0）用于打印
    debug_serial = rt_device_find("uart0");
    if (debug_serial != RT_NULL)
    {
        rt_device_open(debug_serial, RT_DEVICE_FLAG_RDWR);
    }

    rt_kprintf("[ATGM336H] Waiting for GPS module...\n");
    rt_kprintf("[ATGM336H] Debug output on uart0, GPS input on uart2\n");
    rt_thread_mdelay(1000);

    // 创建GPS处理线程
    thread = rt_thread_create("atgm336h",
                              atgm336h_entry,
                              RT_NULL,
                              1024,
                              25,
                              10);
    if(thread != RT_NULL)
    {
        rt_thread_startup(thread);
        rt_kprintf("[ATGM336H] Application initialized successfully!\n\n");
    }
    else
    {
        rt_kprintf("[ATGM336H] Thread create failed!\n");
        return -1;
    }

    return 0;
}

/**
 * @brief   清空结构体数据函数
 */
void clrStruct(void)
{
    Save_Data.isGetData = false; // 标记未获取到GPS数据
    Save_Data.isParseData = false; // 标记数据未解析
    Save_Data.isUsefull = false;   // 标记定位信息无效
    // 清空各个缓冲区
    memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);
    memset(Save_Data.UTCTime, 0, UTCTime_Length);
    memset(Save_Data.latitude, 0, latitude_Length);
    memset(Save_Data.N_S, 0, N_S_Length);
    memset(Save_Data.longitude, 0, longitude_Length);
    memset(Save_Data.E_W, 0, E_W_Length);
}

/**
 * @brief   错误日志函数
 */
void errorLog(int num)
{
    while (1)
    {
        // 打印错误号并进入死循环
        if (debug_serial != RT_NULL)
        {
            uart_printf(debug_serial, "ERROR%d\r\n", num);
        }
        else
        {
            rt_kprintf("ERROR%d\r\n", num);
        }
        rt_thread_mdelay(1000);
    }
}

/**
 * @brief   解析GPS数据缓冲区函数
 */
void parseGpsBuffer(void)
{
    char *subString;      // 指向当前逗号位置
    char *subStringNext;  // 指向下个逗号位置
    char i = 0;           // 循环计数器

    uint16_t Number = 0, Integer = 0, Decimal = 0; // 用于经纬度数值转换

    if (Save_Data.isGetData) // 如果已经获取到GPS数据
    {
        Save_Data.isGetData = false;
        // 打印调试信息
        if (debug_serial != RT_NULL)
        {
//            uart_printf(debug_serial, "**************\r\n");
            uart_printf(debug_serial, "%s\r\n", Save_Data.GPS_Buffer);
        }

        for (i = 0; i <= 6; i++) // 循环解析各字段
        {
            if (i == 0)
            {
                // 找到第一个逗号，分隔字段
                if ((subString = strstr(Save_Data.GPS_Buffer, ",")) == NULL)
                    errorLog(1); // 如果找不到逗号，记录错误
            }
            else
            {
                subString++;
                // 寻找下一个逗号
                if ((subStringNext = strstr(subString, ",")) != NULL)
                {
                    char usefullBuffer[2]; // 用于存储数据有效性信息
                    switch (i)
                    {
                        case 1: // 解析时间字段
                            memcpy(Save_Data.UTCTime, subString, subStringNext - subString);
                            break;
                        case 2: // 解析数据有效性字段
                            memcpy(usefullBuffer, subString, subStringNext - subString);
                            break;
                        case 3: // 解析纬度字段
                            memcpy(Save_Data.latitude, subString, subStringNext - subString);
                            break;
                        case 4: // 解析纬度方向字段
                            memcpy(Save_Data.N_S, subString, subStringNext - subString);
                            break;
                        case 5: // 解析经度字段
                            memcpy(Save_Data.longitude, subString, subStringNext - subString);
                            break;
                        case 6: // 解析经度方向字段
                            memcpy(Save_Data.E_W, subString, subStringNext - subString);
                            break;
                        default:
                            break;
                    }
                    subString = subStringNext; // 更新当前逗号位置
                    Save_Data.isParseData = true; // 标记数据已解析
                    // 判断数据有效性
                    if (usefullBuffer[0] == 'A')
                        Save_Data.isUsefull = true; // 数据有效
                    else if (usefullBuffer[0] == 'V')
                        Save_Data.isUsefull = false; // 数据无效
                }
                else
                {
//                    errorLog(2); // 如果找不到逗号，记录错误
                }
            }
        }

        if (Save_Data.isParseData) // 如果数据已解析
        {
            if (Save_Data.isUsefull) // 如果数据有效
            {
                // 获取纬度方向和经度方向
                g_LatAndLongData.N_S = Save_Data.N_S[0];
                g_LatAndLongData.E_W = Save_Data.E_W[0];

                // 转换纬度数据
                for (uint8_t i = 0; i < 9; i++)
                {
                    if (i < 2)
                    {
                        Number *= 10;
                        Number += Save_Data.latitude[i] - '0'; // 获取度部分
                    }
                    else if (i < 4)
                    {
                        Integer *= 10;
                        Integer += Save_Data.latitude[i] - '0'; // 获取分的整数部分
                    }
                    else if (i == 4);
                    else if (i < 9)
                    {
                        Decimal *= 10;
                        Decimal += Save_Data.latitude[i] - '0'; // 获取分的小数部分
                    }
                }
                // 转换为十进制度数
                g_LatAndLongData.latitude = 1.0 * Number + (1.0 * Integer + 1.0 * Decimal / 10000) / 60;

                Number = 0;
                Integer = 0;
                Decimal = 0;

                // 转换经度数据
                for (uint8_t i = 0; i < 10; i++)
                {
                    if (i < 3)
                    {
                        Number *= 10;
                        Number += Save_Data.longitude[i] - '0'; // 获取度部分
                    }
                    else if (i < 5)
                    {
                        Integer *= 10;
                        Integer += Save_Data.longitude[i] - '0'; // 获取分的整数部分
                    }
                    else if (i == 5);
                    else if (i < 10)
                    {
                        Decimal *= 10;
                        Decimal += Save_Data.longitude[i] - '0'; // 获取分的小数部分
                    }
                }
                // 转换为十进制度数
                g_LatAndLongData.longitude = 1.0 * Number + (1.0 * Integer + 1.0 * Decimal / 10000) / 60;

                // 更新全局经纬度变量
                longitude = g_LatAndLongData.longitude;
                latitude = g_LatAndLongData.latitude;

				//根据上报纬度，向纬度添加符号
				if(g_LatAndLongData.E_W=='W')
					 latitude = -latitude;
				if(g_LatAndLongData.N_S=='S')
					 latitude = -latitude;
            }
        }
    }
}

/**
 * @brief   打印GPS数据函数
 */
void printGpsBuffer(void)
{
    if (Save_Data.isParseData) // 如果数据已解析
    {
        Save_Data.isParseData = false;

        // 打印UTC时间
//        if (debug_serial != RT_NULL)
//        {
//            uart_printf(debug_serial, "Save_Data.UTCTime = %s\r\n", Save_Data.UTCTime);
//        }
//        else
//        {
//            rt_kprintf("Save_Data.UTCTime = %s\r\n", Save_Data.UTCTime);
//        }

        if (Save_Data.isUsefull) // 如果数据有效
        {
            Save_Data.isUsefull = false;
            // 打印原始经纬度数据
            if (debug_serial != RT_NULL)
            {
                uart_printf(debug_serial, "Save_Data.latitude = %s\r\n", Save_Data.latitude);
                uart_printf(debug_serial, "Save_Data.N_S = %s", Save_Data.N_S);
                uart_printf(debug_serial, "Save_Data.longitude = %s", Save_Data.longitude);
                uart_printf(debug_serial, "Save_Data.E_W = %s\r\n", Save_Data.E_W);

                // 打印转换后的经纬度数据
                uart_printf(debug_serial, "latitude: %c,%.4f\r\n", g_LatAndLongData.N_S, g_LatAndLongData.latitude);
                uart_printf(debug_serial, "longitude: %c,%.4f\r\n", g_LatAndLongData.E_W, g_LatAndLongData.longitude);
            }
            else
            {
                rt_kprintf("Save_Data.latitude = %s\r\n", Save_Data.latitude);
                rt_kprintf("Save_Data.N_S = %s", Save_Data.N_S);
                rt_kprintf("Save_Data.longitude = %s", Save_Data.longitude);
                rt_kprintf("Save_Data.E_W = %s\r\n", Save_Data.E_W);
                rt_kprintf("latitude: %c,%.4f\r\n", g_LatAndLongData.N_S, g_LatAndLongData.latitude);
                rt_kprintf("longitude: %c,%.4f\r\n", g_LatAndLongData.E_W, g_LatAndLongData.longitude);
            }
        }
        else
        {
            // 显示GPS数据无效
            if (debug_serial != RT_NULL)
            {
                uart_printf(debug_serial, "GPS DATA is not usefull!\r\n");
            }
            else
            {
                rt_kprintf("GPS DATA is not usefull!\r\n");
            }
        }
    }
}

//INIT_APP_EXPORT(atgm336h_app_init);
