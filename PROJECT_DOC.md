# FRDM-MCXA156 智能头盔传感器数据采集系统

## 1. 项目概述

本项目基于 **NXP FRDM-MCXA156** 开发板，运行 **RT-Thread** 实时操作系统，实现多传感器数据采集并通过 **ESP01S WiFi模块** 上报至 **华为云IoT平台**。

### 1.1 主要功能

| 功能模块 | 传感器/模块 | 数据类型 |
|---------|-----------|---------|
| 气体检测 | MQ2 | 甲烷浓度 (ppm) |
| 温湿度检测 | DHT11 | 温度 (°C)、湿度 (%) |
| 心率血氧检测 | MAX30102 | 红光/红外光原始值、心率 |
| GPS定位 | ATGM336H | 经度、纬度 |
| 云端通信 | ESP01S | MQTT协议上报华为云 |

### 1.2 系统架构

```
+------------------+     +------------------+     +------------------+
|   传感器层        |     |   应用层          |     |   通信层          |
+------------------+     +------------------+     +------------------+
| drv_mq2.c        | --> | MQ2_app.c        |     |                  |
| drv_dht11.c      | --> | dht11_app.c      | --> | esp_app.c        | --> 华为云IoT
| drv_max30102.c   | --> | max30102_app.c   |     | (MQTT上报)        |
| (UART接收)        | --> | ATGM336H_app.c   |     |                  |
+------------------+     +------------------+     +------------------+
```

---

## 2. 硬件平台

### 2.1 主控芯片

- **型号**: NXP MCXA156
- **内核**: ARM Cortex-M33
- **主频**: 96 MHz
- **Flash**: 1 MB
- **RAM**: 128 KB

### 2.2 引脚分配

| 外设 | 引脚 | 功能说明 |
|-----|-----|---------|
| UART0 | 默认 | 调试串口 (rt_kprintf输出) |
| UART1 | - | ESP01S WiFi模块通信 |
| UART2 | - | ATGM336H GPS模块通信 |
| I2C0 | - | MAX30102 心率血氧传感器 |
| GPIO P3_6 | DHT11_DATA | DHT11 数据引脚 |
| GPIO P3_7 | MQ2_DO | MQ2 数字输出引脚 |
| GPIO P1_0 | ADC0_CH0 | MQ2 模拟输出 (ADC采集) |
| GPIO P1_13 | MAX30102_INT | MAX30102 中断引脚 |

---

## 3. 目录结构

```
frdm-mcxa156/
├── applications/           # 应用层代码
│   ├── main.c             # 主函数入口
│   ├── mydefine.h         # 通用头文件定义
│   │
│   ├── drv_dht11.c/h      # DHT11 驱动层
│   ├── dht11_app.c/h      # DHT11 应用层
│   │
│   ├── drv_mq2.c/h        # MQ2 驱动层
│   ├── MQ2_app.c/h        # MQ2 应用层
│   │
│   ├── drv_max30102.c/h   # MAX30102 驱动层
│   ├── max30102_app.c/h   # MAX30102 应用层
│   │
│   ├── ATGM336H_app.c/h   # GPS模块应用层
│   │
│   ├── esp_app.c/h        # ESP01S WiFi/MQTT通信
│   │
│   ├── adc_app.c/h        # ADC采集封装
│   └── uart_app.c/h       # 串口工具函数
│
├── board/                  # 板级支持包
│   ├── board.c/h          # 板级初始化
│   ├── Kconfig            # 硬件配置菜单
│   ├── MCUX_Config/       # NXP MCUXpresso配置
│   │   └── board/
│   │       ├── clock_config.c/h   # 时钟配置
│   │       └── pin_mux.c/h        # 引脚复用配置
│   └── linker_scripts/    # 链接脚本
│
├── packages/               # RT-Thread软件包
│   ├── nxp-mcx-cmsis-latest/     # NXP CMSIS支持
│   └── nxp-mcx-series-latest/    # NXP MCX系列驱动
│
├── .config                # RT-Thread配置文件
├── rtconfig.h             # RT-Thread配置头文件
├── Kconfig                # 顶层配置菜单
├── SConstruct             # SCons构建主文件
├── project.uvprojx        # Keil MDK工程文件
└── rtthread.elf/bin       # 编译输出文件
```

---

## 4. 传感器模块详解

### 4.1 MQ2 气体传感器

**文件**: `drv_mq2.c/h`, `MQ2_app.c/h`

**功能**: 检测甲烷(CH4)等可燃气体浓度

**数据结构**:
```c
typedef struct {
    rt_base_t dopin;    // 数字输出引脚
    float adc_val;      // ADC原始值
    float ch4ppm;       // 甲烷浓度 (ppm)
} mq2_device_t;
```

**API接口**:
```c
// 初始化MQ2设备
rt_err_t mq2_init(mq2_device_t *dev, rt_base_t dopin);

// 读取气体浓度
mq2_result_t MQ2_GetPmm(mq2_device_t *dev);

// 获取当前甲烷浓度 (应用层接口)
float mq2_get_ch4ppm(void);
```

**全局变量**: `g_mq2_dev` - MQ2设备对象

---

### 4.2 DHT11 温湿度传感器

**文件**: `drv_dht11.c/h`, `dht11_app.c/h`

**功能**: 检测环境温度和湿度

**数据结构**:
```c
typedef struct {
    rt_base_t pin;          // 数据引脚
    rt_uint8_t humidity;    // 湿度整数部分
    rt_uint8_t temperature; // 温度整数部分
} dht11_device_t;
```

**API接口**:
```c
// 初始化DHT11设备
rt_err_t dht11_init(dht11_device_t *dev, rt_base_t pin);

// 读取温湿度数据
dht11_result_t dht11_read(dht11_device_t *dev, rt_uint8_t *temp, rt_uint8_t *humi);

// 获取当前温度 (应用层接口)
rt_uint8_t dht11_get_temperature(void);

// 获取当前湿度 (应用层接口)
rt_uint8_t dht11_get_humidity(void);
```

**全局变量**:
- `g_dht11_dev` - DHT11设备对象
- `g_dht11_temperature` - 最新温度值
- `g_dht11_humidity` - 最新湿度值

---

### 4.3 MAX30102 心率血氧传感器

**文件**: `drv_max30102.c/h`, `max30102_app.c/h`

**功能**: 通过红光和红外光检测心率和血氧

**通信接口**: I2C (地址: 0x57)

**数据结构**:
```c
typedef struct {
    struct rt_i2c_bus_device *i2c_bus;  // I2C总线句柄
    rt_mutex_t lock;                     // 互斥锁
    rt_uint8_t addr;                     // I2C地址
    rt_bool_t initialized;               // 初始化标志
} max30102_device_t;
```

**API接口**:
```c
// 初始化MAX30102设备
max30102_device_t *max30102_init(const char *i2c_bus_name);

// 从FIFO读取LED数据
rt_err_t max30102_read_fifo(max30102_device_t *dev,
                            rt_uint32_t *red_led,
                            rt_uint32_t *ir_led);

// 获取心率 (应用层接口)
rt_uint32_t max30102_get_heart_rate(void);
```

**全局变量**:
- `g_max30102_red_led` - 红光LED原始值
- `g_max30102_ir_led` - 红外LED原始值
- `g_max30102_heart_rate` - 心率估算值

**工作模式**: 支持中断模式和轮询模式 (通过 `USE_INTERRUPT_MODE` 宏切换)

---

### 4.4 ATGM336H GPS模块

**文件**: `ATGM336H_app.c/h`

**功能**: 获取GPS定位信息 (经度、纬度)

**通信接口**: UART2 (波特率: 9600)

**数据结构**:
```c
typedef struct {
    char GPS_Buffer[80];    // GPS原始数据缓冲
    char isGetData;         // 数据接收标志
    char isParseData;       // 数据解析标志
    char UTCTime[11];       // UTC时间
    char latitude[11];      // 纬度字符串
    char N_S[2];            // 南北半球
    char longitude[12];     // 经度字符串
    char E_W[2];            // 东西半球
    char isUsefull;         // 定位有效标志
} _SaveData;

typedef struct {
    float latitude;         // 纬度 (十进制度)
    float longitude;        // 经度 (十进制度)
    char N_S;               // 南北半球标识
    char E_W;               // 东西半球标识
} LatitudeAndLongitude_s;
```

**全局变量**:
- `Save_Data` - GPS原始数据结构
- `g_LatAndLongData` - 解析后的经纬度数据
- `latitude`, `longitude` - 全局经纬度变量

---

### 4.5 ESP01S WiFi模块

**文件**: `esp_app.c/h`

**功能**: 通过MQTT协议将传感器数据上报至华为云IoT平台

**通信接口**: UART1

**云平台配置** (定义在 `esp_app.h`):
```c
#define WIFI_NAME               "LP11"
#define WIFI_PWD                "123456aa"
#define HUAWEI_MQTT_ADDRESS     "e8b7ac5772.st1.iotda-device.cn-east-3.myhuaweicloud.com"
#define HUAWEI_MQTT_PORT        1883
```

**API接口**:
```c
// 发送AT指令
void esp_send(const char *data);

// 上报传感器数据到云端
int esp_report(float density, int hr, int temp, int humi);
```

**数据上报格式** (MQTT JSON):
```json
{
  "services": [{
    "service_id": "BasedData",
    "properties": {
      "density": 100.5,
      "heart_rate": 75,
      "temperature": 25,
      "humidity": 60
    }
  }]
}
```

---

## 5. 线程架构

系统采用多线程架构，各传感器独立采集：

| 线程名 | 优先级 | 栈大小 | 功能 |
|-------|-------|-------|------|
| main | - | - | 系统主线程 |
| mq2 | 30 | 1024 | MQ2气体浓度采集 |
| dht11 | 20 | 1024 | DHT11温湿度采集 |
| max30102 | 20 | 2048 | MAX30102心率采集 |
| atgm336h | 25 | 1024 | GPS数据解析 |
| esp | 19 | 2048 | WiFi/MQTT通信 |

---

## 6. 构建与烧录

### 6.1 使用Keil MDK

1. 打开 `project.uvprojx` 工程文件
2. 编译: `Project -> Build Target` 或 `F7`
3. 烧录: `Flash -> Download` 或 `F8`

### 6.2 使用SCons (命令行)

```bash
# 配置
scons --menuconfig

# 编译
scons

# 清理
scons -c
```

---

## 7. 配置说明

### 7.1 RT-Thread配置

通过 `menuconfig` 或直接编辑 `.config` 文件配置：

```
# 串口配置
CONFIG_BSP_USING_UART0=y    # 调试串口
CONFIG_BSP_USING_UART1=y    # ESP01S
CONFIG_BSP_USING_UART2=y    # GPS模块

# I2C配置
CONFIG_BSP_USING_I2C0=y     # MAX30102

# ADC配置
CONFIG_BSP_USING_ADC0_CH0=y # MQ2模拟输出
```

### 7.2 传感器引脚配置

在各应用文件中修改引脚定义：

```c
// MQ2_app.c
#define MQ2_DATA_PIN     ((3*32)+7)    // P3_7

// dht11_app.c
#define DHT11_DATA_PIN   ((3*32)+6)    // P3_6

// max30102_app.c
#define MAX30102_INT_PIN ((1*32)+13)   // P1_13
```

---

## 8. 数据流说明

```
[传感器采集] --> [全局变量更新] --> [esp_thread_entry读取] --> [esp_report上报] --> [华为云]

时序:
1. 各传感器线程周期性采集数据，更新全局变量
2. ESP线程在主循环中读取全局变量
3. 调用 esp_report() 构造MQTT消息并发送
4. 华为云IoT平台接收并存储数据
```

---

## 9. 注意事项

1. **DHT11**: 两次读取间隔至少2秒
2. **MQ2**: 上电后需预热稳定期
3. **MAX30102**: I2C通信需要较大栈空间
4. **GPS**: 首次定位需要较长时间，室内可能无法定位
5. **ESP01S**: WiFi连接需要5秒等待，MQTT连接需要3秒等待

---

## 10. 文件依赖关系

```
mydefine.h (基础定义)
    ├── drv_dht11.h
    ├── drv_mq2.h
    ├── drv_max30102.h
    │
    ├── dht11_app.h  --> dht11_app.c
    ├── MQ2_app.h    --> MQ2_app.c
    ├── max30102_app.h --> max30102_app.c
    ├── ATGM336H_app.h --> ATGM336H_app.c
    │
    └── esp_app.h    --> esp_app.c (引用所有传感器数据)
```

---

## 11. 版本信息

- **RT-Thread版本**: 5.x
- **开发板**: NXP FRDM-MCXA156
- **IDE**: Keil MDK 5.x
- **创建日期**: 2025-11
