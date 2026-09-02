#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();
    DL_GPIO_setPins(SIGNAL_GPIO_PORT, SIGNAL_GPIO_OUTPUT_PIN);
    while (1) __WFI();
}
