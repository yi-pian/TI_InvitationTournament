#include <stddef.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"

static const char g_message[] = "MSPM0 direct DriverLib\r\n";

int main(void)
{
    size_t index;

    SYSCFG_DL_init();
    for (index = 0U; index < (sizeof(g_message) - 1U); ++index) {
        DL_UART_Main_transmitDataBlocking(
            SIGNAL_UART_INST, (uint8_t)g_message[index]);
    }
    while (1) __WFI();
}
