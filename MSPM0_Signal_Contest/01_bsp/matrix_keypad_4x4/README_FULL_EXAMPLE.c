/* MSPM0G3507 fixed-pin full example: call Start after SYSCFG_DL_init(). */
#include "signal_matrix_keypad_4x4.h"

#if defined(__MSPM0G3507__)
#include "ti_msp_dl_config.h"

void matrix_keypad_4x4_FullExample_Start(void)
{
    if (SysTick_Config(CPUCLK_FREQ / 1000U) != 0U) {
        while (1) { }
    }
}

void SysTick_Handler(void)
{
    static uint8_t milliseconds;
    char symbol;
    signal_result_t result;
    signal_module_status_t module_status;

    ++milliseconds;
    if (milliseconds < 5U) return;
    milliseconds = 0U;

    module_status = SignalMatrixKeypad4x4_GetModuleStatus();
    if (module_status != MODULE_STATUS_BUILD_VERIFIED) return;

    result = SignalMatrixKeypad4x4_ReadNewSymbol(&symbol);
    if (result == SIGNAL_RESULT_NO_DATA) {
        /* No newly debounced key, a possible ghost key, or a key still debouncing. */
        return;
    }
    if (result != SIGNAL_RESULT_OK) {
        /* Handle a GPIO or module initialization error here. */
        return;
    }

    /* Use symbol to select a menu, frequency, range, or another application action. */
}
#else
/* The fixed-pin convenience API intentionally exists only for MSPM0G3507. */
void matrix_keypad_4x4_FullExample_Start(void)
{
}
#endif
