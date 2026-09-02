/* MSPM0G3507 fixed-pin minimum closed loop: call Start after SYSCFG_DL_init(). */
#include "signal_matrix_keypad_4x4.h"

#if defined(__MSPM0G3507__)
#include "ti_msp_dl_config.h"

void matrix_keypad_4x4_MinimalExample_Start(void)
{
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) {
        while (1) { }
    }
}

void SysTick_Handler(void)
{
    static uint8_t milliseconds;
    char symbol;

    ++milliseconds;
    if (milliseconds < 5U) return;
    milliseconds = 0U;

    if (SignalMatrixKeypad4x4_ReadNewSymbol(&symbol) == SIGNAL_RESULT_OK) {
        /* symbol is the newly debounced key: '1'..'9', 'A'..'D', '*', '0', '#'. */
    }
}
#else
/* The fixed-pin convenience API intentionally exists only for MSPM0G3507. */
void matrix_keypad_4x4_MinimalExample_Start(void)
{
}
#endif
