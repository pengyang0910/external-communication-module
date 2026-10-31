/*!
    \file    bsp_uart.c
    \brief   调试串口（USART2 / PB10 PB11）、ESP32 串口（USART0 / PA9 PA10）、
             RS485 串口（USART1 / PA2 PA3）驱动与日志输出
*/
#include "board.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void usart_gpio_config(uint32_t usart_rcu, uint32_t gpio_rcu, uint32_t gpio,
                              uint32_t af, uint16_t tx_pin, uint16_t rx_pin)
{
    rcu_periph_clock_enable(gpio_rcu);
    rcu_periph_clock_enable(usart_rcu);
    rcu_periph_clock_enable(RCU_AF);

    gpio_af_set(gpio, af, tx_pin | rx_pin);
    gpio_mode_set(gpio, GPIO_MODE_AF, GPIO_PUPD_PULLUP, tx_pin | rx_pin);
    gpio_output_options_set(gpio, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, tx_pin | rx_pin);
}

static void usart_common_init(uint32_t usart, uint32_t baud)
{
    usart_deinit(usart);
    usart_baudrate_set(usart, baud);
    usart_word_length_set(usart, USART_WL_8BIT);
    usart_stop_bit_set(usart, USART_STB_1BIT);
    usart_parity_config(usart, USART_PM_NONE);
    usart_receive_config(usart, USART_RECEIVE_ENABLE);
    usart_transmit_config(usart, USART_TRANSMIT_ENABLE);
    usart_enable(usart);
}

void debug_uart_init(void)
{
    usart_gpio_config(DEBUG_USART_RCU, DEBUG_GPIO_RCU, DEBUG_GPIO,
                      DEBUG_AF, DEBUG_TX_PIN, DEBUG_RX_PIN);
    usart_common_init(DEBUG_USART, DEBUG_BAUDRATE);
}

void esp_uart_init(void)
{
    usart_gpio_config(ESP_USART_RCU, ESP_GPIO_RCU, ESP_GPIO,
                      ESP_AF, ESP_TX_PIN, ESP_RX_PIN);
    usart_common_init(ESP_USART, ESP_BAUDRATE);
}

void rs485_uart_init(void)
{
    usart_gpio_config(RS485_USART_RCU, RS485_GPIO_RCU, RS485_GPIO,
                      RS485_AF, RS485_TX_PIN, RS485_RX_PIN);
    usart_common_init(RS485_USART, RS485_BAUDRATE);
}

int uart_send(uint32_t usart, const uint8_t *buf, uint32_t len)
{
    uint32_t i;

    for (i = 0U; i < len; i++) {
        while (RESET == usart_flag_get(usart, USART_FLAG_TBE)) {
        }
        usart_data_transmit(usart, buf[i]);
    }
    /* 等待最后一个字节移出（RS485 方向切换前必须等待）*/
    while (RESET == usart_flag_get(usart, USART_FLAG_TC)) {
    }
    return (int)len;
}

int uart_recv_byte_timeout(uint32_t usart, uint8_t *byte, uint32_t timeout_ms)
{
    uint32_t start = bsp_get_tick();

    while (RESET == usart_flag_get(usart, USART_FLAG_RBNE)) {
        if ((bsp_get_tick() - start) >= timeout_ms) {
            return -1;
        }
    }
    *byte = (uint8_t)usart_data_receive(usart);
    return 0;
}

void debug_printf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n > 0) {
        if (n > (int)sizeof(buf) - 1) {
            n = (int)sizeof(buf) - 1;
        }
        uart_send(DEBUG_USART, (const uint8_t *)buf, (uint32_t)n);
    }
}
