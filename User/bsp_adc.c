/*!
    \file    bsp_adc.c
    \brief   ADC 三路：4-20mA（PA0/ADC0 ch0）、硬件版本（PB0/ADC0 ch8）、
             母线电压（PE15/ADC2 ch14）
*/
#include "board.h"

static uint16_t adc_single_conv(uint32_t adc, uint8_t ch)
{
    adc_routine_channel_config(adc, 0U, ch, ADC_SAMPLETIME_55POINT5);
    adc_software_trigger_enable(adc, ADC_ROUTINE_CHANNEL);
    while (RESET == adc_flag_get(adc, ADC_FLAG_EORC)) {
    }
    adc_flag_clear(adc, ADC_FLAG_EORC);
    return adc_routine_data_read(adc);
}

static void adc_periph_config(uint32_t adc)
{
    adc_deinit(adc);
    adc_special_function_config(adc, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(adc, ADC_CONTINUOUS_MODE, DISABLE);
    adc_data_alignment_config(adc, ADC_DATAALIGN_RIGHT);
    adc_channel_length_config(adc, ADC_ROUTINE_CHANNEL, 1U);
    adc_external_trigger_config(adc, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    adc_enable(adc);
}

void board_adc_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_periph_clock_enable(RCU_ADC2);
    rcu_periph_clock_enable(RCU_AF);

    /* ADC 时钟 = APB2/6 = 42MHz */
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV6);

    /* 模拟输入引脚 */
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0);
    gpio_mode_set(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0);
    gpio_mode_set(GPIOE, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_15);

    adc_periph_config(ADC0);
    adc_periph_config(ADC2);

    bsp_delay_ms(2);
}

uint16_t adc_read_avg(uint32_t adc, uint8_t ch, uint8_t times)
{
    uint32_t sum = 0U;
    uint8_t i;

    for (i = 0U; i < times; i++) {
        sum += adc_single_conv(adc, ch);
    }
    return (uint16_t)(sum / times);
}

hw_ver_t board_read_hwver(void)
{
    /* 原理图 V0~V9 判定表（3.3V 基准，ADC 码值）*/
    static const uint16_t hwver_code[10] = {
        3823U, 3517U, 3186U, 2832U, 2478U,
        2154U, 1802U, 1436U, 1102U, 698U
    };
    uint16_t code = adc_read_avg(ADC_HWVER_ADC, ADC_HWVER_CH, 16U);
    int best = -1;
    int best_diff = 0x7FFF;
    int diff;
    uint8_t i;

    for (i = 0U; i < 10U; i++) {
        diff = (int)code - (int)hwver_code[i];
        if (diff < 0) {
            diff = -diff;
        }
        if (diff < best_diff) {
            best_diff = diff;
            best = (int)i;
        }
    }
    if ((best < 0) || (best_diff > 100)) {
        return HW_VER_UNKNOWN;
    }
    return (hw_ver_t)(best + 1);   /* HW_VER_V0 == 1 */
}

uint32_t board_read_vbus_mv(void)
{
    uint16_t code = adc_read_avg(ADC_VBUS_ADC, ADC_VBUS_CH, 16U);
    uint32_t vdet_mv = ((uint32_t)code * ADC_VREF_MV) / ADC_FULL_SCALE;

    /* 分压还原：VDET = VIN * 4.7/104.7 */
    return (vdet_mv * VBUS_DIV_RTOT) / VBUS_DIV_RBOT;
}

uint32_t board_read_v420_ua(void)
{
    uint16_t code = adc_read_avg(ADC_V420_ADC, ADC_V420_CH, 16U);
    uint32_t mv = ((uint32_t)code * ADC_VREF_MV) / ADC_FULL_SCALE;

    /* I(uA) = V(mV) / R(Ohm) * 1000 */
    return (mv * 1000U) / V420_SHUNT_OHM;
}
