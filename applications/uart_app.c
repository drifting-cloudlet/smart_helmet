#include <rtthread.h>
#include <stdarg.h>
#include "uart_app.h"
#define SAMPLE_UART_NAME "uart0"

/* 类似 rt_kprintf，但可以指定串口设备 */
void uart_printf(rt_device_t dev, const char *fmt, ...)
{
    char buf[256];
    va_list args;

    va_start(args, fmt);
    rt_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    rt_device_write(dev, 0, buf, rt_strlen(buf));
}
//RT_NAME_MAX
/* 信号量，用于接收数据通知 */
static struct rt_semaphore rx_sem;
static rt_device_t serial;   // uart0 接收
static rt_device_t serial3;  // uart1 发送

/* 接收回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    rt_kprintf("[RX] callback triggered, size=%d\n", size);
    rt_sem_release(&rx_sem);  // 发送信号量
    return RT_EOK;
}

/* 处理串口接收到的数据 */
static void serial_thread_entry(void *parameter)
{
    char ch;
    while (1)
    {
        rt_sem_take(&rx_sem, RT_WAITING_FOREVER);  // 等待信号量

        // 从uart3读取接收到的数据
        while(rt_device_read(serial3, -1, &ch, 1) == 1)
        {
            // 通过uart0发送出去显示
            uart_printf(serial, "[UART3->UART0] data: 0x%02X '%c'\n", ch,
                       (ch >= 32 && ch <= 126) ? ch : '.');
        }
    }
}

static int uart_sample(void)
{
    /* 查找并打开串口0（用于调试输出） */
    serial = rt_device_find(SAMPLE_UART_NAME);
    if (!serial)
    {
        rt_kprintf("uart0 not found\n");
        return -RT_ERROR;
    }
    rt_device_open(serial, RT_DEVICE_FLAG_RDWR);  // 只用于发送

    /* 查找并打开串口2（用于接收数据） */
    serial3 = rt_device_find("uart1");
    if (!serial3)
    {
        rt_kprintf("uart3 not found!\n");
        return -RT_ERROR;
    }
    // 以中断接收模式打开uart3
    rt_device_open(serial3, RT_DEVICE_FLAG_INT_RX);

    /* 初始化信号量和接收回调 */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    rt_device_set_rx_indicate(serial3, uart_input);  // uart3接收触发回调

    /* 创建线程处理接收到的数据 */
    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024, 25, 10);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
        rt_kprintf("[UART_APP] Started: UART2(RX) -> UART0(TX)\n");
        rt_kprintf("[UART_APP] Send data to UART2, will echo to UART0\n\n");
    }
    else
    {
        rt_kprintf("[UART_APP] Thread create failed!\n");
        return -RT_ERROR;
    }
		uart_printf(serial3,"aaa\r\n");
    return 0;
}

//INIT_APP_EXPORT(uart_sample);


