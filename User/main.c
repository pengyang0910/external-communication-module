/*!
    \file    main.c
    \brief   外部通信模块 V10 单项功能打通程序（bring-up）
    \date    2026-10-31

    运行流程（全部结果从调试串口输出，115200-8-N-1，J3 排针 DEBUGUART_TX/RX）：
      1. ADC：硬件版本识别 / 母线电压 / 4-20mA
      2. EEPROM：0x50/0x51 探测 + 写读比对
      3. 数字量输入：Level_LT1~3
      4. 24V 输出：OUT1/OUT2 开 1s 关 1s（带外部负载前请确认）
      5. CAN0/CAN1：内部回环自测后切正常模式
      6. RS485：发送测试字符串
      7. ESP32：复位后发送 AT 探测
      8. CH394Q：SPI 复位 + 读公共寄存器
    之后进入循环：每 2 秒打印一次 DI / 电压 / 版本快照。
*/
#include "board.h"
#include <string.h>

/* ------------------------------------------------------------------ */
static const char *hw_ver_name(hw_ver_t v)
{
    static const char *names[11] = {
        "UNKNOWN", "V0", "V1", "V2", "V3", "V4",
        "V5", "V6", "V7", "V8", "V9"
    };
    if ((unsigned)v > 10U) {
        return "UNKNOWN";
    }
    return names[(unsigned)v];
}

static void print_bytes_hex(const uint8_t *buf, uint32_t len)
{
    uint32_t i;

    for (i = 0U; i < len; i++) {
        debug_printf("%02X ", buf[i]);
    }
    debug_printf("\r\n");
}

/* ESP32 探测：复位 -> 发 AT -> 收集响应 */
static int esp_probe(uint8_t *resp, uint32_t maxlen, uint32_t timeout_ms)
{
    uint32_t got = 0U;
    uint8_t b;

    board_esp_boot(0);              /* BOOT 高：正常启动 */
    board_esp_en(0);                /* 复位 */
    bsp_delay_ms(50U);
    board_esp_en(1);                /* 释放复位 */
    bsp_delay_ms(500U);

    uart_send(ESP_USART, (const uint8_t *)"AT\r\n", 4U);
    while (got < maxlen) {
        /* 首字节等待 timeout_ms，后续字节 100ms 超时结束 */
        uint32_t to = (got == 0U) ? timeout_ms : 100U;
        if (uart_recv_byte_timeout(ESP_USART, &b, to) == 0) {
            resp[got++] = b;
        } else {
            break;
        }
    }
    return (int)got;
}

/* EEPROM 自测：写 8 字节测试图样到 0x0100，读回比对 */
static int eeprom_selftest(uint8_t dev7)
{
    static const uint8_t pattern[8] = {0x5AU, 0xA5U, 0x00U, 0xFFU, 0x12U, 0x34U, 0x56U, 0x78U};
    uint8_t rd[8];

    if (eeprom_write(dev7, 0x0100U, pattern, 8U) != 0) {
        return -1;
    }
    if (eeprom_read(dev7, 0x0100U, rd, 8U) != 0) {
        return -1;
    }
    return (memcmp(rd, pattern, 8U) == 0) ? 0 : -1;
}

/* ------------------------------------------------------------------ */
int main(void)
{
    uint8_t resp[64];
    int n;
    int ret;
    int v1, v2;
    hw_ver_t ver;
    uint32_t vbus_mv, v420_ua;
    uint8_t di1, di2, di3;
    uint32_t tick;

    bsp_delay_init();
    debug_uart_init();
    board_gpio_init();

    debug_printf("\r\n\r\n");
    debug_printf("=========================================\r\n");
    debug_printf(" External Communication Module V10\r\n");
    debug_printf(" GD32F503CEO7 bring-up  %s %s\r\n", __DATE__, __TIME__);
    debug_printf("=========================================\r\n");
    debug_printf("NOTE: LED1/LED2 = power, LED3/LED4 = ESP side.\r\n");
    debug_printf("      Heartbeat is shown on this UART.\r\n\r\n");

    /* 1. ADC ------------------------------------------------------ */
    board_adc_init();
    ver = board_read_hwver();
    vbus_mv = board_read_vbus_mv();
    v420_ua = board_read_v420_ua();
    debug_printf("[1] ADC   : HW_VER=%s, Vbus=%lu mV, 4-20mA=%lu uA\r\n",
                 hw_ver_name(ver), (unsigned long)vbus_mv, (unsigned long)v420_ua);

    /* 2. EEPROM --------------------------------------------------- */
    eeprom_init();
    v1 = eeprom_probe(EE_ADDR_0);
    v2 = eeprom_probe(EE_ADDR_1);
    debug_printf("[2] EEPROM: probe 0x50 -> %s, 0x51 -> %s\r\n",
                 (v1 == 0) ? "ACK" : "NAK",
                 (v2 == 0) ? "ACK" : "NAK");
    if (v1 == 0) {
        ret = eeprom_selftest(EE_ADDR_0);
        debug_printf("    EEPROM: write/read test (0x0100, 8B) -> %s\r\n",
                     (ret == 0) ? "PASS" : "FAIL");
    }

    /* 3. 数字量输入 ----------------------------------------------- */
    di1 = (uint8_t)board_di_read(1U);
    di2 = (uint8_t)board_di_read(2U);
    di3 = (uint8_t)board_di_read(3U);
    debug_printf("[3] DI    : LT1=%u LT2=%u LT3=%u  (1=外部输入有效)\r\n", di1, di2, di3);

    /* 4. 24V 输出 ------------------------------------------------- */
    debug_printf("[4] DO    : 24V_OUT1/OUT2 pulse 1s on / 1s off\r\n");
    board_do_set(1U, 1);
    board_do_set(2U, 1);
    bsp_delay_ms(1000U);
    board_do_set(1U, 0);
    board_do_set(2U, 0);
    bsp_delay_ms(1000U);
    debug_printf("    DO    : done (both OFF now)\r\n");

    /* 5. CAN0 / CAN1 ---------------------------------------------- */
    ret = can_selftest(CAN0_PERIPH, CAN0_RCU, CAN0_GPIO_RCU, CAN0_GPIO,
                       CAN0_AF, CAN0_RX_PIN, CAN0_TX_PIN);
    debug_printf("[5] CAN0  : loopback self-test -> %s\r\n", (ret == 0) ? "PASS" : "FAIL");
    ret = can_selftest(CAN1_PERIPH, CAN1_RCU, CAN1_GPIO_RCU, CAN1_GPIO,
                       CAN1_AF, CAN1_RX_PIN, CAN1_TX_PIN);
    debug_printf("    CAN1  : loopback self-test -> %s\r\n", (ret == 0) ? "PASS" : "FAIL");
    debug_printf("    CAN   : both re-init to NORMAL mode, 500kbps\r\n");

    /* 6. RS485 ---------------------------------------------------- */
    rs485_uart_init();
    board_rs485_de(1);
    uart_send(RS485_USART, (const uint8_t *)"RS485 BRINGUP OK\r\n", 18U);
    board_rs485_de(0);
    debug_printf("[6] RS485 : test frame sent @115200 (echo needs external adapter)\r\n");

    /* 7. ESP32 ---------------------------------------------------- */
    esp_uart_init();
    n = esp_probe(resp, sizeof(resp), 200U);
    debug_printf("[7] ESP32 : AT probe -> %d bytes: ", n);
    if (n > 0) {
        print_bytes_hex(resp, (uint32_t)n);
    } else {
        debug_printf("no response (module may lack AT firmware)\r\n");
    }

    /* 8. CH394Q --------------------------------------------------- */
    v1 = ch394_init();
    if (v1 >= 0) {
        debug_printf("[8] CH394 : SPI reg[0x002C] read twice OK, value=0x%04X\r\n", v1);
    } else {
        debug_printf("[8] CH394 : SPI probe FAIL (check SPI0 wiring/reset)\r\n");
    }

    debug_printf("\r\nAll single-function tests triggered. Loop status below:\r\n\r\n");

    /* 循环状态快照 ------------------------------------------------ */
    tick = bsp_get_tick();
    while (1) {
        if ((bsp_get_tick() - tick) >= 2000U) {
            tick = bsp_get_tick();
            ver = board_read_hwver();
            vbus_mv = board_read_vbus_mv();
            v420_ua = board_read_v420_ua();
            di1 = (uint8_t)board_di_read(1U);
            di2 = (uint8_t)board_di_read(2U);
            di3 = (uint8_t)board_di_read(3U);
            debug_printf("[%lu s] VER=%s Vbus=%lu mV I420=%lu uA DI=%u%u%u\r\n",
                         (unsigned long)(bsp_get_tick() / 1000U),
                         hw_ver_name(ver),
                         (unsigned long)vbus_mv,
                         (unsigned long)v420_ua,
                         di1, di2, di3);
        }
    }
}
