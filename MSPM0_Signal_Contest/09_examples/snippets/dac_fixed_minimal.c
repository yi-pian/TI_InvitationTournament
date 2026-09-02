#include <stdint.h>

#include "ti_msp_dl_config.h"

volatile uint16_t g_dac_code = 2048U;

int main(void)
{
    SYSCFG_DL_init();
    DL_DAC12_output12(DAC0, g_dac_code);
    while (1) __WFI();
}
