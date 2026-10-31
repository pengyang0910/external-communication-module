/*!
    \file    board.h
    \brief   外部通信模块 V10 引脚映射与板级常量
             （依据：4_架构设计文档\硬件\external communication module-v10-2610261026.pdf）

    引脚映射来源：原理图 MCU 页（第 5 页）按"导线坐标—网络标号"几何关系提取，
    与 GD32F503xx Datasheet 表 2-11~2-15（复用功能表）交叉核对。

    注意（数据手册/原理图确认的事实）：
    - 调试串口是 USART2 (PB10/PB11)，接 J3 调试排针，不是 USART0；
    - ESP32-S2-MINI-2U 的 UART 接 USART0 (PA9/PA10)；
    - RS485 接 USART1 (PA2/PA3)；
    - CAN0 在 PB8/PB9（不是 PA11/PA12），CAN1 在 PB12/PB13；
    - 板上无 GD32 直驱 LED（LED1/LED2 为电源指示，LED3/LED4 由 ESP32 侧驱动），
      因此"心跳"用串口日志体现。
*/
#ifndef __BOARD_H__
#define __BOARD_H__

#include "gd32f50x.h"
#include <stdint.h>
#include <stddef.h>

/* ================= 系统时钟 ================= */
/* system_gd32f50x.c 已按 __SYSTEM_CLOCK_252M_PLL_HXTAL 配置：
   HXTAL 12MHz -> PLL -> SYSCLK 252MHz，AHB/APB1/APB2 均 DIV1 */
#define BOARD_SYSCLK_HZ        (252000000U)
#define BOARD_PCLK1_HZ         (252000000U)   /* APB1 = 252MHz（CAN/串口时钟）*/
#define BOARD_PCLK2_HZ         (252000000U)   /* APB2 = 252MHz */

/* ================= 调试串口：USART2 / PB10 TX, PB11 RX ================= */
#define DEBUG_USART            USART2
#define DEBUG_USART_RCU        RCU_USART2
#define DEBUG_GPIO_RCU         RCU_GPIOB
#define DEBUG_GPIO             GPIOB
#define DEBUG_AF               GPIO_AF_1
#define DEBUG_TX_PIN           GPIO_PIN_10
#define DEBUG_RX_PIN           GPIO_PIN_11
#define DEBUG_BAUDRATE         115200U

/* ================= ESP32-S2-MINI-2U：USART0 / PA9 TX, PA10 RX ================= */
#define ESP_USART              USART0
#define ESP_USART_RCU          RCU_USART0
#define ESP_GPIO_RCU           RCU_GPIOA
#define ESP_GPIO               GPIOA
#define ESP_AF                 GPIO_AF_0
#define ESP_TX_PIN             GPIO_PIN_9
#define ESP_RX_PIN             GPIO_PIN_10
#define ESP_EN_PIN             GPIO_PIN_11      /* PA11: 高=运行，低=复位 */
#define ESP_BOOT_PIN           GPIO_PIN_12      /* PA12: 低=下载模式（复位时采样）*/
#define ESP_BAUDRATE           115200U

/* ================= RS485：USART1 / PA2 TX, PA3 RX，方向 PA1 ================= */
#define RS485_USART            USART1
#define RS485_USART_RCU        RCU_USART1
#define RS485_GPIO_RCU         RCU_GPIOA
#define RS485_GPIO             GPIOA
#define RS485_AF               GPIO_AF_0
#define RS485_TX_PIN           GPIO_PIN_2
#define RS485_RX_PIN           GPIO_PIN_3
#define RS485_DE_PIN           GPIO_PIN_1       /* 高=发送，低=接收（DE/RE 并联）*/
#define RS485_BAUDRATE         115200U

/* ================= CAN0：PB8 RX, PB9 TX（TCAN1044A U8）================= */
#define CAN0_PERIPH            CAN0
#define CAN0_RCU               RCU_CAN0
#define CAN0_GPIO_RCU          RCU_GPIOB
#define CAN0_GPIO              GPIOB
#define CAN0_AF                GPIO_AF_1
#define CAN0_RX_PIN            GPIO_PIN_8
#define CAN0_TX_PIN            GPIO_PIN_9

/* ================= CAN1：PB12 RX, PB13 TX（TCAN1044A U13）================= */
#define CAN1_PERIPH            CAN1
#define CAN1_RCU               RCU_CAN1
#define CAN1_GPIO_RCU          RCU_GPIOB
#define CAN1_GPIO              GPIOB
#define CAN1_AF                GPIO_AF_3
#define CAN1_RX_PIN            GPIO_PIN_12
#define CAN1_TX_PIN            GPIO_PIN_13

/* ================= SPI0 -> CH394Q：PA5 SCK, PA6 MISO, PA7 MOSI ================= */
#define CH394_SPI              SPI0
#define CH394_SPI_RCU          RCU_SPI0
#define CH394_GPIO_RCU         RCU_GPIOA
#define CH394_GPIO             GPIOA
#define CH394_AF               GPIO_AF_0
#define CH394_SCK_PIN          GPIO_PIN_5
#define CH394_MISO_PIN         GPIO_PIN_6
#define CH394_MOSI_PIN         GPIO_PIN_7
#define CH394_CS_PIN           GPIO_PIN_4       /* PA4: 软件片选，低有效 */
#define CH394_RST_PIN          GPIO_PIN_15      /* PA15: 复位，低有效 */
#define CH394_INT_PORT         GPIOB
#define CH394_INT_PIN          GPIO_PIN_3       /* PB3: 中断输入，低有效 */
#define CH394_SPI_PRESCALE     SPI_PSC_8        /* 252MHz/2/8 ≈ 15.75MHz */

/* ================= I2C EEPROM（M24512-DF x2, 0x50/0x51）：PB6 SCL, PB7 SDA ================= */
/* 采用软件 I2C（开漏 GPIO），板上已有 4.7K 上拉 */
#define EE_SCL_PORT            GPIOB
#define EE_SCL_PIN             GPIO_PIN_6
#define EE_SDA_PORT            GPIOB
#define EE_SDA_PIN             GPIO_PIN_7
#define EE_GPIO_RCU            RCU_GPIOB
#define EE_ADDR_0              0x50U            /* U6 */
#define EE_ADDR_1              0x51U            /* U7 */

/* ================= ADC 通道 ================= */
#define ADC_V420_CH            0U               /* PA0  - ADC_IN_3V（4-20mA，ADC0 通道0）*/
#define ADC_HWVER_CH           8U               /* PB0  - HW_VER_DET（ADC0 通道8）*/
#define ADC_VBUS_CH            14U              /* PE15 - ADC_IN1（ADC2 通道14，母线电压）*/
#define ADC_V420_ADC           ADC0
#define ADC_HWVER_ADC          ADC0
#define ADC_VBUS_ADC           ADC2

#define ADC_VREF_MV            3300U            /* REF3033/VDDA 3.3V 基准 */
#define ADC_FULL_SCALE         4095U

/* 4-20mA 采样电阻 150R 0.1% */
#define V420_SHUNT_OHM         150U

/* 母线电压分压：R8=100K, R12=4.7K -> VDET = VIN * 4.7/(100+4.7) */
#define VBUS_DIV_RTOT          104700UL         /* 100K + 4.7K */
#define VBUS_DIV_RBOT          4700UL

/* ================= 24V 输出（NCE60P04Y1 高边开关）================= */
#define OUT1_PORT              GPIOB
#define OUT1_RCU               RCU_GPIOB
#define OUT1_PIN               GPIO_PIN_15      /* MCU_24V_OUT1 */
#define OUT2_PORT              GPIOB
#define OUT2_RCU               RCU_GPIOB
#define OUT2_PIN               GPIO_PIN_14      /* MCU_24V_OUT2 */

/* ================= 数字量输入（Level_LT1~3，经 N-MOS 反相）================= */
#define DI_LT1_PORT            GPIOB
#define DI_LT1_PIN             GPIO_PIN_4
#define DI_LT2_PORT            GPIOB
#define DI_LT2_PIN             GPIO_PIN_5
#define DI_LT3_PORT            GPIOE
#define DI_LT3_PIN             GPIO_PIN_0
#define DI_RCU                 (RCU_GPIOB | RCU_GPIOE)

/* ================= 硬件版本判定表（V0~V9，ADC 码值，容差 ±100）================= */
typedef enum {
    HW_VER_UNKNOWN = 0,
    HW_VER_V0, HW_VER_V1, HW_VER_V2, HW_VER_V3, HW_VER_V4,
    HW_VER_V5, HW_VER_V6, HW_VER_V7, HW_VER_V8, HW_VER_V9
} hw_ver_t;

/* ================= 延时 / 日志 ================= */
void bsp_delay_init(void);
void bsp_delay_ms(uint32_t ms);
uint32_t bsp_get_tick(void);

/* ================= BSP 接口 ================= */
/* UART */
void debug_uart_init(void);
void debug_printf(const char *fmt, ...);
int  uart_send(uint32_t usart, const uint8_t *buf, uint32_t len);
int  uart_recv_byte_timeout(uint32_t usart, uint8_t *byte, uint32_t timeout_ms);
void esp_uart_init(void);
void rs485_uart_init(void);

/* GPIO */
void board_gpio_init(void);
void board_esp_en(int on);
void board_esp_boot(int download);
int  board_di_read(uint8_t idx);          /* 1..3 -> Level_LT1..3（已反相还原）*/
void board_do_set(uint8_t idx, int on);   /* 1..2 -> MCU_24V_OUT1/2 */
int  board_do_get(uint8_t idx);
void board_rs485_de(int tx);
void board_ch394_rst(int assert);

/* ADC */
void board_adc_init(void);
uint16_t adc_read_avg(uint32_t adc, uint8_t ch, uint8_t times);
hw_ver_t board_read_hwver(void);
uint32_t board_read_vbus_mv(void);
uint32_t board_read_v420_ua(void);

/* EEPROM（软件 I2C）*/
void eeprom_init(void);
int  eeprom_probe(uint8_t dev7);
int  eeprom_write(uint8_t dev7, uint16_t addr, const uint8_t *buf, uint16_t len);
int  eeprom_read(uint8_t dev7, uint16_t addr, uint8_t *buf, uint16_t len);

/* CAN */
int  can_bus_init(uint32_t can_periph, uint32_t can_rcu, uint32_t can_gpio_rcu,
                  uint32_t can_gpio, uint32_t can_af, uint16_t rx_pin, uint16_t tx_pin,
                  uint8_t mode);
int  can_selftest(uint32_t can_periph, uint32_t can_rcu, uint32_t can_gpio_rcu,
                  uint32_t can_gpio, uint32_t can_af, uint16_t rx_pin, uint16_t tx_pin);
int  can_send_std(uint32_t can_periph, uint16_t id, const uint8_t *data, uint8_t len);

/* CH394 以太网 */
void ch394_spi_init(void);
int  ch394_init(void);
uint16_t ch394_read_common_reg(uint16_t addr);
int  ch394_probe(void);

#endif /* __BOARD_H__ */
