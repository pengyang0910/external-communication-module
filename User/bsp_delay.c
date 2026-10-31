/*!
    \file    bsp_delay.c
    \brief   SysTick 1ms 延时（CMSIS SysTick_Config，SystemCoreClock=252MHz）
*/
#include "board.h"

static volatile uint32_t s_ms_tick = 0;

void SysTick_Handler(void)
{
    s_ms_tick++;
}

void bsp_delay_init(void)
{
    (void)SysTick_Config(SystemCoreClock / 1000U);
}

void bsp_delay_ms(uint32_t ms)
{
    uint32_t start = s_ms_tick;

    while ((s_ms_tick - start) < ms) {
        /* 等待 SysTick 中断 */
    }
}

uint32_t bsp_get_tick(void)
{
    return s_ms_tick;
}
