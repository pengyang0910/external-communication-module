/*!
    \file    bsp_can.c
    \brief   CAN0/CAN1 驱动：500kbps（22.28 时基：252MHz/24/21），回环自测与正常发送
             注意：TCAN1044A 的 STB 脚板上已硬接（原理图第 6 页未引回 MCU），
             上电即为正常模式，无需软件控制。
*/
#include "board.h"

int can_bus_init(uint32_t can_periph, uint32_t can_rcu, uint32_t can_gpio_rcu,
                 uint32_t can_gpio, uint32_t can_af, uint16_t rx_pin, uint16_t tx_pin,
                 uint8_t mode)
{
    can_parameter_struct can_parameter;

    rcu_periph_clock_enable(can_rcu);
    rcu_periph_clock_enable(can_gpio_rcu);
    rcu_periph_clock_enable(RCU_AF);

    gpio_af_set(can_gpio, can_af, rx_pin | tx_pin);
    gpio_mode_set(can_gpio, GPIO_MODE_AF, GPIO_PUPD_NONE, rx_pin | tx_pin);
    gpio_output_options_set(can_gpio, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, rx_pin | tx_pin);

    can_deinit(can_periph);
    can_struct_para_init(CAN_INIT_STRUCT, &can_parameter);
    can_parameter.time_triggered = DISABLE;
    can_parameter.auto_bus_off_recovery = ENABLE;
    can_parameter.auto_wake_up = DISABLE;
    can_parameter.auto_retrans = ENABLE;
    can_parameter.rec_fifo_overwrite = ENABLE;
    can_parameter.trans_fifo_order = ENABLE;
    can_parameter.working_mode = mode;
    /* 500kbps：252MHz / 24 = 10.5MHz，1+16+4 = 21 TQ -> 500kbps，采样点 81% */
    can_parameter.resync_jump_width = CAN_BT_SJW_1TQ;
    can_parameter.time_segment_1 = CAN_BT_BS1_16TQ;
    can_parameter.time_segment_2 = CAN_BT_BS2_4TQ;
    can_parameter.prescaler = 24U;
    can_init(can_periph, &can_parameter);

    /* 接收过滤器：标准帧全接收 -> FIFO0 */
    can_filter_mask_mode_init(0x0000U, 0x0000U, CAN_STANDARD_FIFO0, 0U);

    return 0;
}

int can_send_std(uint32_t can_periph, uint16_t id, const uint8_t *data, uint8_t len)
{
    can_transmit_message_struct tx_message;
    uint8_t i;

    tx_message.tx_sfid = id;
    tx_message.tx_efid = 0U;
    tx_message.tx_ff = CAN_FF_STANDARD;
    tx_message.tx_ft = CAN_FT_DATA;
    if (len > 8U) {
        len = 8U;
    }
    tx_message.tx_dlen = len;
    for (i = 0U; i < len; i++) {
        tx_message.tx_data[i] = data[i];
    }
    if (can_message_transmit(can_periph, &tx_message) == CAN_NOMAILBOX) {
        return -1;
    }
    return 0;
}

/* 内部回环自测：发送一帧，从 FIFO0 收回并比对 */
int can_selftest(uint32_t can_periph, uint32_t can_rcu, uint32_t can_gpio_rcu,
                 uint32_t can_gpio, uint32_t can_af, uint16_t rx_pin, uint16_t tx_pin)
{
    static const uint8_t pattern[4] = {0xA5U, 0x5AU, 0x12U, 0x34U};
    can_receive_message_struct rx_message;
    uint8_t i;
    int pass;

    can_bus_init(can_periph, can_rcu, can_gpio_rcu, can_gpio, can_af,
                 rx_pin, tx_pin, CAN_LOOPBACK_MODE);
    if (can_send_std(can_periph, 0x123U, pattern, 4U) != 0) {
        return -1;
    }
    /* 等待接收完成（最多 100ms）*/
    {
        uint32_t start = bsp_get_tick();
        while (can_receive_message_length_get(can_periph, CAN_FIFO0) == 0U) {
            if ((bsp_get_tick() - start) >= 100U) {
                return -1;
            }
        }
    }
    can_message_receive(can_periph, CAN_FIFO0, &rx_message);
    can_fifo_release(can_periph, CAN_FIFO0);

    if ((rx_message.rx_dlen != 4U) || (rx_message.rx_sfid != 0x123U)) {
        return -1;
    }
    pass = 1;
    for (i = 0U; i < 4U; i++) {
        if (rx_message.rx_data[i] != pattern[i]) {
            pass = 0;
        }
    }

    /* 自测完成后切回正常模式，供外部总线测试 */
    can_bus_init(can_periph, can_rcu, can_gpio_rcu, can_gpio, can_af,
                 rx_pin, tx_pin, CAN_NORMAL_MODE);

    return pass ? 0 : -1;
}
