# 基于STM32的车载大灯控制系统

## 1. 项目简介

<details>
<summary>📌 点击展开</summary>

**基于STM32F103C8T6 的模拟车载灯光控制系统**

- **硬件平台**：STM32F103C8T6
- **传感器**：HC-SR04（超声波测距）、BH1750（I2C 光照采集）
- **执行器**：WS2812（转向灯/双闪，单总线控制）、LED（PWM 模拟远/近光切换）

系统根据距离和光照强度，通过状态机控制灯光模式。

</details>

---

## 2. 硬件清单

<details>
<summary>📌 点击展开</summary>

- KEY按键
- STM32F103C8T6
- WS2812灯条
- HC-SR04
- BH1750
- RGB LED
- OLED显示屏

</details>

---

## 3. 硬件连接

<details>
<summary>📌 点击展开</summary>

- 三色LED连接PA6,PA7,PB0推挽输出，采用低电平GPIO_PIN_RESET(GPIO_PIN_SET/高电平)
- KEY1 - PB12 外部上拉，初始高电平，按下低电平
- KEY2 - PB13 需内部上拉，初始高电平，按下低电平
- KEY3 - PB15 需内部上拉，初始高电平，按下低电平
- USART2_TX - PA2 异步发送
- USART2_RX - PA3 接收
- I2C - SDA SCL 连接OLED CH1116，地址0x7A
- BH1750 - SDA SCL，地址0x23
- HC-SR04 - PA11(TTrig) PA10(Echo)
- WS2812 - DI DO 输入输出引脚，DO引脚连接DI 连接PB4，外置5V高电平采用开漏输出

</details>

---

## 4. 关键技术点

### 4.1 BH1750 光照传感器（I2C）

<details>
<summary>📌 点击展开</summary>

- **通信协议**：I2C
- **设备地址**：0x23（ADDR 引脚接 GND）
- **工作模式**：连续高分辨率模式（0x10），约 120ms 采样一次
- **数据读取**：非阻塞方式，I2C 中断回调中处理数据
- **光照强度计算**：原始数据 / 1.2

</details>

### 4.2 超声波测距（输入捕获）

<details>
<summary>📌 点击展开</summary>

**测距原理**：距离 = (发送时刻 - 接收时刻) × 声速 / 2

**实现流程**：
- 超声波模块接收到GPIO给Trig引脚的脉冲后，发送超声波，此时拉高Echo
- 当接收到返回的超声波时，将Echo电平拉低，接收完毕
- 从捕获上升沿立即开始定时器计数，一直到捕获到下降沿，得出超声波轨迹时间
- 记录后，程序可以随时读取（捕获上升沿为TI1，下降沿为TI2）

**定时器配置**：
- 时钟速度：72MHz（为了更快响应）
- 预分频：72-1 → 72MHz / 72 = 1MHz，即 1us 计数器加1
- 自动重装载：最大值 65535

**代码关键点**：
- 通过捕获中断来回调超声波距离
- 每次触发拉低即结束触发时，将计数器清0，防止自动重装载导致负数
- 用 `htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3` 确认通道
- 用 `HAL_TIM_ReadCapturedValue` 读取数值

**距离计算公式**：
```c
distance = (float)(downEdge - upEdge) * 0.034f / 2.0f;
  
<details>

### 4.3 WS2812 灯带控制（PWM + DMA）

<details>
<summary>📌 点击展开</summary>

**通信协议**：
- 数据速率：800Kbps
- 颜色顺序：**GRB**（注意不是常见的 RGB 顺序）
- 亮度范围：0 - 255（0x00 - 0xFF）

**时序要求**：

| 码型 | 高电平时间 | 低电平时间 |
|------|-----------|-----------|
| 0 码 | ≤ 0.4us | ≥ 0.85us |
| 1 码 | ≥ 0.85us | ≤ 0.4us |
| Reset 码 | - | 低电平 ≥ 125us |

**为什么用 PWM + DMA**：
- 灯带对时序要求严格，用 GPIO 翻转会频繁打断 CPU
- 改用 PWM 输出 + DMA 搬运数据，CPU 只需要把颜色数据准备好，剩下的由硬件自动完成

**具体配置**：
- 定时器 TIM3 输出 PWM，频率设为 800KHz（周期 1.25us）
- 定义两个比较值：
  - `code0 = 30`：高电平约 0.4us（0 码）
  - `code1 = 60`：高电平约 0.8us（1 码）
- 每个灯需要 24 位数据（GRB 各 8 位），通过 DMA 依次发送到比较寄存器

**代码示意**：
```c
#define code0 30
#define code1 60

</details> ```

### 4.4 状态机架构

<details>
<summary>📌 点击展开</summary>

**设计思想**：
- 采用非阻塞状态机，避免 while 或 delay 造成的卡顿
- 每个模块（按键、传感器、灯光）独立运行，主循环轮询状态

**系统状态定义**：

| 状态 | 说明 |
|------|------|
| STATE_INIT | 初始化 / 主菜单 |
| STATE_AUTO_HEADLIGHT | 自动大灯模式（基于光照） |
| STATE_AHB | 自动远近光模式（基于距离） |
| STATE_MANUAL_AHB | 手动远近光模式 |
| STATE_LOWLIGHT | 近光 |
| STATE_HIGHLIGHT | 远光 |
| STATE_CORNER_LIGHT | 转向灯辅助模式（菜单） |
| STATE_TURN_LEFT | 左转向流水灯 |
| STATE_TURN_RIGHT | 右转向流水灯 |
| STATE_HAZARD | 双闪警示模式 |

**状态迁移条件**：
- KEY1：切换自动大灯 / 退出子模式
- KEY2：切换自动远近光 / 近光/远光切换
- KEY3（EC11）：进入/退出转向灯辅助模式
- KEY1 + KEY2 同时按下：进入双闪模式

**非阻塞实现**：
- 不使用 HAL_Delay，所有延时通过状态机和定时器变量实现
- 每个状态只在进入时执行一次初始化，在循环中持续更新

</details>

## 5. 代码结构
<details> <summary>📌 点击展开</summary>
分类多个函数
Core/
--Src/
  --main.c               # 主程序入口，状态机轮询
  -- bh1750.c            # BH1750 驱动（I2C 非阻塞）
  -- ws2812.c            # WS2812 驱动（PWM + DMA）
  -- oled.c              # OLED 显示驱动
  -- Sensor.c            # 超声波测距 + 中断回调
  -- state_machine.c     # 状态机核心逻辑
--Inc/
  --bh1750.h
  --ws2812.h
  --oled.h
  --Sensor.h
  --state_machine.h
  --font.h              # 字库（ASCII + 中文）
  <details>
