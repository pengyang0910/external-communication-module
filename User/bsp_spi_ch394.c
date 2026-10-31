/*!
    \file    bsp_spi_ch394.c
    \brief   CH394Q 以太网协议栈芯片 SPI 驱动（SPI0：PA5/PA6/PA7 + 软件片选 PA4，
             复位 PA15，中断 PB3）
             SPI 帧格式（CH394 Datasheet V1.2 第 5 章）：16 位偏移地址(高字节先) +
             8 位控制字 + 数据。控制字 [7:5]=块选择(0=公共寄存器,1=socket0 寄存器,...),
             [4:3]=功能(0=公共寄存器,1=socket 寄存器,2=TX 缓冲,3=RX 缓冲)，[2]=1 写 0 读。
*/
#include "board.h"

static uint8_t spi_txrx(uint8_t d)
{
    while (RESET == spi_i2s_flag_get(CH394_SPI, SPI_FLAG_TBE)) {
    }
    spi_i2s_data_transmit(CH394_SPI, d);
    while (RESET == spi_i2s_flag_get(CH394_SPI, SPI_FLAG_RBNE)) {
    }
    return (uint8_t)spi_i2s_data_receive(CH394_SPI);
}

static void ch394_cs(int low)
{
    if (low) {
        gpio_bit_reset(GPIOA, CH394_CS_PIN);
    } else {
        gpio_bit_set(GPIOA, CH394_CS_PIN);
    }
}

void ch394_spi_init(void)
{
    spi_parameter_struct spi_init_struct;

    rcu_periph_clock_enable(RCU_SPI0);
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_AF);

    gpio_af_set(GPIOA, GPIO_AF_0, CH394_SCK_PIN | CH394_MISO_PIN | CH394_MOSI_PIN);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  CH394_SCK_PIN | CH394_MISO_PIN | CH394_MOSI_PIN);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL2,
                            CH394_SCK_PIN | CH394_MISO_PIN | CH394_MOSI_PIN);

    spi_i2s_deinit(CH394_SPI);
    spi_struct_para_init(&spi_init_struct);
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;   /* 模式 0 */
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.prescale = CH394_SPI_PRESCALE;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(CH394_SPI, &spi_init_struct);
    spi_enable(CH394_SPI);
}

/* 一次完整数据帧：地址(2B) + 控制字(1B) + 数据 */
static void ch394_frame(uint16_t addr, uint8_t ctrl, uint8_t *data, uint16_t len, int read)
{
    uint16_t i;

    ch394_cs(1);
    spi_txrx((uint8_t)(addr >> 8));
    spi_txrx((uint8_t)(addr & 0xFFU));
    spi_txrx(ctrl);
    for (i = 0U; i < len; i++) {
        if (read) {
            data[i] = spi_txrx(0x00U);
        } else {
            (void)spi_txrx(data[i]);
        }
    }
    /* 等待最后一个字节移出再释放片选 */
    while (SET == spi_i2s_flag_get(CH394_SPI, SPI_FLAG_TRANS)) {
    }
    ch394_cs(0);
}

uint16_t ch394_read_common_reg(uint16_t addr)
{
    uint8_t b[2] = {0xFFU, 0xFFU};

    ch394_frame(addr, 0x00U, b, 2U, 1);   /* 控制字 0x00：公共寄存器读 */
    return ((uint16_t)b[0] << 8) | (uint16_t)b[1];
}

int ch394_probe(void)
{
    uint16_t v1, v2;

    /* 复位时序：RST 低 10ms -> 释放 -> 等待 20ms */
    board_ch394_rst(1);
    bsp_delay_ms(10U);
    board_ch394_rst(0);
    bsp_delay_ms(20U);

    /* 连续两次读同一公共寄存器（0x002C，数据手册示例用寄存器）；
       两次读数一致即认为 SPI 通道与片选工作正常。 */
    v1 = ch394_read_common_reg(0x002CU);
    v2 = ch394_read_common_reg(0x002CU);

    if (v1 == v2) {
        return (int)v1;
    }
    return -1;
}

int ch394_init(void)
{
    ch394_spi_init();
    return ch394_probe();
}
