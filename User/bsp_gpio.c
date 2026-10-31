/*!
    \file    bsp_gpio.c
    \brief   板级 GPIO：24V 输出、ESP 控制、RS485 方向、CH394 复位/片选、
             数字量输入（Level_LT1~3）
*/
#include "board.h"

static void gpio_out_init(uint32_t port, uint32_t rcu, uint16_t pins, uint32_t level)
{
    rcu_periph_clock_enable(rcu);
    gpio_mode_set(port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, pins);
    gpio_output_options_set(port, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL2, pins);
    if (level) {
        gpio_bit_set(port, pins);
    } else {
        gpio_bit_reset(port, pins);
    }
}

void board_gpio_init(void)
{
    /* 24V 输出：上电默认关断（N-MOS 栅极低 -> P-MOS 关）*/
    gpio_out_init(OUT1_PORT, OUT1_RCU, OUT1_PIN, 0U);
    gpio_out_init(OUT2_PORT, OUT2_RCU, OUT2_PIN, 0U);

    /* ESP32：EN 高（运行），BOOT 高（正常启动，非下载模式）*/
    gpio_out_init(GPIOA, RCU_GPIOA, ESP_EN_PIN | ESP_BOOT_PIN, 1U);

    /* RS485：默认接收 */
    gpio_out_init(GPIOA, RCU_GPIOA, RS485_DE_PIN, 0U);

    /* CH394：RST 释放（高），CS 空闲（高）*/
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH394_CS_PIN | CH394_RST_PIN);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL2, CH394_CS_PIN | CH394_RST_PIN);
    gpio_bit_set(GPIOA, CH394_RST_PIN);
    gpio_bit_set(GPIOA, CH394_CS_PIN);

    /* CH394 中断输入 */
    rcu_periph_clock_enable(RCU_GPIOB);
    gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, CH394_INT_PIN);

    /* 数字量输入：内部上拉（板上经 N-MOS 反相输出，见 board.h 注释）*/
    rcu_periph_clock_enable(RCU_GPIOE);
    gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, DI_LT1_PIN | DI_LT2_PIN);
    gpio_mode_set(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, DI_LT3_PIN);
}

void board_esp_en(int on)
{
    if (on) {
        gpio_bit_set(GPIOA, ESP_EN_PIN);
    } else {
        gpio_bit_reset(GPIOA, ESP_EN_PIN);
    }
}

void board_esp_boot(int download)
{
    if (download) {
        gpio_bit_reset(GPIOA, ESP_BOOT_PIN);
    } else {
        gpio_bit_set(GPIOA, ESP_BOOT_PIN);
    }
}

int board_di_read(uint8_t idx)
{
    int raw;

    switch (idx) {
    case 1U:
        raw = gpio_input_bit_get(DI_LT1_PORT, DI_LT1_PIN);
        break;
    case 2U:
        raw = gpio_input_bit_get(DI_LT2_PORT, DI_LT2_PIN);
        break;
    case 3U:
        raw = gpio_input_bit_get(DI_LT3_PORT, DI_LT3_PIN);
        break;
    default:
        return 0;
    }
    /* 板上经 N-MOS 反相：外部输入高 -> 引脚读到低 */
    return raw ? 0 : 1;
}

void board_do_set(uint8_t idx, int on)
{
    uint32_t port = (idx == 1U) ? OUT1_PORT : OUT2_PORT;
    uint16_t pin = (idx == 1U) ? OUT1_PIN : OUT2_PIN;

    if (on) {
        gpio_bit_set(port, pin);
    } else {
        gpio_bit_reset(port, pin);
    }
}

int board_do_get(uint8_t idx)
{
    if (idx == 1U) {
        return gpio_output_bit_get(OUT1_PORT, OUT1_PIN) ? 1 : 0;
    }
    return gpio_output_bit_get(OUT2_PORT, OUT2_PIN) ? 1 : 0;
}

void board_rs485_de(int tx)
{
    if (tx) {
        gpio_bit_set(RS485_GPIO, RS485_DE_PIN);
    } else {
        gpio_bit_reset(RS485_GPIO, RS485_DE_PIN);
    }
}

void board_ch394_rst(int assert)
{
    if (assert) {
        gpio_bit_reset(GPIOA, CH394_RST_PIN);
    } else {
        gpio_bit_set(GPIOA, CH394_RST_PIN);
    }
}
