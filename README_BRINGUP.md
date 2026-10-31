# 外部通信模块 V10 — and bring-up 工程说明

> 生成日期：2026-10-31
> 依据：原理图 `4_架构设计文档\硬件\external communication module-v10-2610261026.pdf`
> 编译验证：Arm Compiler 6.24（armclang/armlink），19 个源文件 0 错误 0 警告，链接通过（含启动文件）。

## 1. 工程结构

工程位于 `workspace/external-communication-module/`：

```
external-communication-module/
├── GD32F503.uvprojx       # Keil 工程（Target: Target_1, Device: GD32F503CE）
├── User/                  # 应用层
│   ├── board.h            # ★ 引脚映射总表 + 板级常量（先看这个）
│   ├── main.c             # bring-up 测试主流程
│   ├── bsp_delay.c        # SysTick 1ms 延时
│   ├── bsp_uart.c         # 调试串口/ESP/RS485 串口 + 日志
│   ├── bsp_gpio.c         # 24V输出、ESP控制、RS485方向、CH394复位、DI
│   ├── bsp_adc.c          # 电压/4-20mA/硬件版本
│   ├── bsp_eeprom.c       # 软件 I2C EEPROM 驱动
│   ├── bsp_can.c          # CAN0/CAN1 + 回环自测
│   └── bsp_spi_ch394.c    # CH394Q SPI 驱动
├── Drivers/               # 从 pack 复制的标准外设库源文件（RTE 当时只勾了 5 个外设）
│   ├── gd32f50x_spi.c / can.c / adc.c / fmc.c / dbg.c
└── RTE/                   # RTE 自动管理的组件（startup/system/gpio/misc/pmu/rcu/usart）
```

## 2. 引脚映射（已与数据手册表 2-11~2-15 的 AF 表交叉核对）

| 功能 | 原理图网络 | 引脚 | 复用功能 |
|---|---|---|---|
| 调试串口 TX / RX | DEBUGUART_TX / RX | PB10 / PB11 | USART2，AF1 |
| ESP32 串口 TX / RX | ESP_TX / RX | PA9 / PA10 | USART0，AF0 |
| ESP32 控制 | ESP_EN / ESP_BOOT | PA11 / PA12 | GPIO 输出 |
| RS485 TX / RX | RS485_TX / RS485_RX | PA2 / PA3 | USART1，AF0 |
| RS485 方向 | RS485_EN | PA1 | GPIO（高=发送） |
| CAN0 TX / RX | CAN0_TX / CAN0_RX | PB9 / PB8 | AF1 |
| CAN1 TX / RX | CAN1_TX / CAN1_RX | PB13 / PB12 | AF3 |
| SPI0 SCK / MISO / MOSI | SPI0_CLK / MISO / MOSI | PA5 / PA6 / PA7 | AF0 |
| CH394 片选 / 复位 / 中断 | SPI0_CSN / CH394Q_RST / SPI_ETHER_INT | PA4 / PA15 / PB3 | GPIO / 输入 |
| EEPROM I2C | EE_CLK / EE_SDA | PB6 / PB7 | 软件 I2C（开漏） |
| 4-20mA 采样 | ADC_IN_3V | PA0 | ADC0 通道 0 |
| 硬件版本 | HW_VER_DET | PB0 | ADC0 通道 8 |
| 母线电压 | ADC_IN1 | PE15 | ADC2 通道 14 |
| 24V 输出 1 / 2 | MCU_24V_OUT1 / OUT2 | PB15 / PB14 | GPIO（高=开） |
| 数字量输入 1/2/3 | Level_LT1 / 2 / 3 | PB4 / PB5 / PE0 | GPIO 输入（经 N-MOS 反相，代码已还原） |
| SWD | — | PA13 (SWDIO) / PA14 (SWCLK) | — |
| 外部晶振 | OSC12M_IN / OUT | PD0 / PD1 | 12MHz（X1） |
| BOOT0 | — | 引脚 44（R13 10K 下拉） | 板上固定 |

> 注：调试串口是 **USART2（PB10/PB11）**，不是 USART0——USART0 留给 ESP32。CAN0 在 **PB8/PB9**。板上没有 GD32 直驱 LED（LED1/LED2 为电源指示，LED3/LED4 由 ESP32 侧驱动），"心跳"通过串口日志体现。

## 3. 编译配置要点（重要）

- **宏定义：`GD32F50X,GD32F503`** —— 两个都要！`GD32F50X` 让 `gd32f50x.h` 通过；`GD32F503` 让 `system_gd32f50x.c` 选择 252MHz 时钟配置（否则 SystemCoreClock 不生成、时钟停在默认值）。
- 工程分组：`User`（应用）、`Drivers`（补充外设库源文件）、`::CMSIS`、`::Device`（RTE 管理）。
- 编译器：AC6（armclang 6.24），优化 `-O1`。
- 若以后在 Manage Run-Time Environment 里勾选新外设，用 Resolve 补齐依赖即可，无需手动拷贝驱动。

## 4. 测试流程与预期输出

上电后（调试串口 115200-8-N-1，接 J3 的 DEBUGUART_TX/RX/GND），应看到：

```
=========================================
 External Communication Module V10
 GD32F503CEO7 bring-up  Oct 31 2026 ...
=========================================
NOTE: LED1/LED2 = power, LED3/LED4 = ESP side.
      Heartbeat is shown on this UART.

[1] ADC   : HW_VER=V?, Vbus=???? mV, 4-20mA=???? uA
[2] EEPROM: probe 0x50 -> ACK, 0x51 -> ACK
    EEPROM: write/read test (0x0100, 8B) -> PASS
[3] DI    : LT1=0 LT2=0 LT3=0  (1=外部输入有效)
[4] DO    : 24V_OUT1/OUT2 pulse 1s on / 1s off
    DO    : done (both OFF now)
[5] CAN0  : loopback self-test -> PASS
    CAN1  : loopback self-test -> PASS
    CAN   : both re-init to NORMAL mode, 500kbps
[6] RS485 : test frame sent @115200 (echo needs external adapter)
[7] ESP32 : AT probe -> N bytes: 4F 4B ...
[8] CH394 : SPI reg[0x002C] read twice OK, value=0x????   （或 FAIL）

All single-function tests triggered. Loop status below:

[123 s] VER=V3 Vbus=24012 mV I420=4010 uA DI=000
```

## 5. 调试注意事项

1. **24V 输出会在上电时脉冲 1 秒**（演示用）。若输出端接了外部设备，请先断开或修改 `main.c` 第 4 节。
2. **ESP32 未接 U.FL 天线前不要让它发射**（`esp_probe` 只发 4 字节 AT，影响很小，但正式测 WiFi 请先接天线）。
3. CAN 先跑**内部回环**（单板可验证）；代码在自测后自动切回正常模式（500kbps），接分析仪即可对外通信。
4. CH394 当前仅验证 **SPI 通路**（复位 + 公共寄存器两次读取一致）。协议栈初始化（GLOB 配置、socket 建立）需要后续按 WCH 驱动库补充。
5. 数字量输入经板上 N-MOS 反相（外部输入有效 → MCU 读到低），代码已做还原；若实测逻辑相反，检查 `board.h` 中 `board_di_read` 的说明并按需取反。
6. 软件 I2C 速率非严格 100kHz（约 100~200kHz），M24512 支持 1MHz，无风险。

## 6. 当前状态

- [x] 全部源文件编译通过（AC6）
- [x] 启动文件 + 链接通过（armlink，0 错误）
- [x] 引脚映射经原理图坐标提取 + 数据手册 AF 表核对
- [ ] 待上板实测（电压/电流/波形/通信链路按第 4 节逐项确认）
- [ ] CH394 协议栈初始化与 TCP 通信（下一阶段）
- [ ] CAN FD 模式（当前为经典 CAN 500kbps；FD 可参考官方 `communication_FDmode` 例程）

## 7. 构建方法

1. **关闭已有 Keil 窗口**（避免旧工程状态覆盖），重新打开 `external-communication-module\GD32F503.uvprojx`；
2. 确认 Options → C/C++ → Define 为 `GD32F50X,GD32F503`；
3. F7 编译 → 0 错误；下载到板子（SWD：PA13/PA14，GD-Link/J-Link）；
4. 串口 115200 观察启动日志。
