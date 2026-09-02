#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_adc_to_voltage.h"

static const uint16_t g_raw[4] = {0U, 1024U, 2048U, 4095U};
static float g_voltage[4];
volatile signal_algorithm_status_t g_status;

int main(void)
{
    const signal_adc_to_voltage_config_t config = {
        4095U, 3.3f, 1.0f, 0.0f
    };
    SYSCFG_DL_init();
    g_status = SignalADCToVoltage_Process(g_raw, g_voltage, 4U, &config);
    while (1) { __WFI(); }
}
