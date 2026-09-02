#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_dds.h"

#define SIGNAL_DDS_UPDATE_RATE_HZ  (100000.0f)
#define SIGNAL_DDS_FREQUENCY_HZ    (1000.0f)

static const uint16_t g_table[8] = {
    2048U, 3496U, 4095U, 3496U, 2048U, 600U, 0U, 600U
};
static uint16_t g_output[16];
static signal_dds_t g_dds;
volatile signal_result_t g_status;

int main(void)
{
    SYSCFG_DL_init();
    g_status = SignalDDS_Init(&g_dds, g_table, 8U,
        SIGNAL_DDS_FREQUENCY_HZ, SIGNAL_DDS_UPDATE_RATE_HZ, 0U);
    if (g_status == SIGNAL_RESULT_OK) {
        g_status = SignalDDS_Fill(&g_dds, g_output, 16U);
    }

    while (1) {
        /* ===== 这里写你自己的逻辑：把 g_output 交给 DAC DMA ===== */
        __WFI();
    }
}
