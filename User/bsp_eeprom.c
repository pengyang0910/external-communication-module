/*!
    \file    bsp_eeprom.c
    \brief   M24512-DF EEPROM 驱动（软件 I2C，PB6=SCL PB7=SDA，开漏 + 板上 4.7K 上拉）
             设备地址 0x50/0x51，16 位内部地址，页写 128 字节，写周期 <=5ms
*/
#include "board.h"

#define I2C_ACK_OK      0
#define I2C_ACK_NACK    1

static void scl_high(void) { gpio_bit_set(EE_SCL_PORT, EE_SCL_PIN); }
static void scl_low(void)  { gpio_bit_reset(EE_SCL_PORT, EE_SCL_PIN); }
static void sda_high(void) { gpio_bit_set(EE_SDA_PORT, EE_SDA_PIN); }
static void sda_low(void)  { gpio_bit_reset(EE_SDA_PORT, EE_SDA_PIN); }
static int  sda_read(void) { return gpio_input_bit_get(EE_SDA_PORT, EE_SDA_PIN) ? 1 : 0; }

static void i2c_delay(void)
{
    volatile uint32_t n = 120U;
    while (n--) {
    }
}

void eeprom_init(void)
{
    rcu_periph_clock_enable(EE_GPIO_RCU);

    /* 开漏输出：写 1 = 释放总线（外部上拉拉高），写 0 = 拉低 */
    gpio_mode_set(EE_SCL_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, EE_SCL_PIN | EE_SDA_PIN);
    gpio_output_options_set(EE_SCL_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_LEVEL3, EE_SCL_PIN | EE_SDA_PIN);
    scl_high();
    sda_high();
    i2c_delay();
}

static void i2c_start(void)
{
    sda_high();
    i2c_delay();
    scl_high();
    i2c_delay();
    sda_low();
    i2c_delay();
    scl_low();
    i2c_delay();
}

static void i2c_stop(void)
{
    sda_low();
    i2c_delay();
    scl_high();
    i2c_delay();
    sda_high();
    i2c_delay();
}

static int i2c_write_byte(uint8_t byte)
{
    uint8_t i;
    int ack;

    for (i = 0U; i < 8U; i++) {
        if (byte & 0x80U) {
            sda_high();
        } else {
            sda_low();
        }
        i2c_delay();
        scl_high();
        i2c_delay();
        scl_low();
        i2c_delay();
        byte <<= 1;
    }
    /* 第 9 个时钟读 ACK */
    sda_high();
    i2c_delay();
    scl_high();
    i2c_delay();
    ack = sda_read();
    scl_low();
    i2c_delay();

    return ack ? I2C_ACK_NACK : I2C_ACK_OK;
}

static uint8_t i2c_read_byte(int send_ack)
{
    uint8_t byte = 0U;
    uint8_t i;

    sda_high();
    for (i = 0U; i < 8U; i++) {
        scl_high();
        i2c_delay();
        byte <<= 1;
        if (sda_read()) {
            byte |= 0x01U;
        }
        scl_low();
        i2c_delay();
    }
    /* 第 9 个时钟回 ACK/NACK */
    if (send_ack) {
        sda_low();
    } else {
        sda_high();
    }
    i2c_delay();
    scl_high();
    i2c_delay();
    scl_low();
    i2c_delay();

    return byte;
}

static int eeprom_start_write(uint8_t dev7, uint16_t addr)
{
    i2c_start();
    if (i2c_write_byte((uint8_t)(dev7 << 1)) != I2C_ACK_OK) {
        i2c_stop();
        return -1;
    }
    if (i2c_write_byte((uint8_t)(addr >> 8)) != I2C_ACK_OK) {
        i2c_stop();
        return -1;
    }
    if (i2c_write_byte((uint8_t)(addr & 0xFFU)) != I2C_ACK_OK) {
        i2c_stop();
        return -1;
    }
    return 0;
}

int eeprom_probe(uint8_t dev7)
{
    int ret;

    i2c_start();
    ret = i2c_write_byte((uint8_t)(dev7 << 1));
    i2c_stop();

    return (ret == I2C_ACK_OK) ? 0 : -1;
}

int eeprom_write(uint8_t dev7, uint16_t addr, const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    if (eeprom_start_write(dev7, addr) != 0) {
        return -1;
    }
    for (i = 0U; i < len; i++) {
        if (i2c_write_byte(buf[i]) != I2C_ACK_OK) {
            i2c_stop();
            return -1;
        }
    }
    i2c_stop();
    /* 等待内部写周期（<=5ms）*/
    bsp_delay_ms(6);

    return 0;
}

int eeprom_read(uint8_t dev7, uint16_t addr, uint8_t *buf, uint16_t len)
{
    uint16_t i;
    int send_ack;

    /* 1. 写地址指针（dummy write）*/
    if (eeprom_start_write(dev7, addr) != 0) {
        return -1;
    }

    /* 2. 重复起始 + 读 */
    i2c_start();
    if (i2c_write_byte((uint8_t)((dev7 << 1) | 0x01U)) != I2C_ACK_OK) {
        i2c_stop();
        return -1;
    }

    for (i = 0U; i < len; i++) {
        send_ack = (i + 1U < len) ? 1 : 0;   /* 非最后字节回 ACK，最后回 NACK */
        buf[i] = i2c_read_byte(send_ack);
    }
    i2c_stop();

    return 0;
}
