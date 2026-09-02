#include <stdint.h>

#include "signal_config.h"
#include "signal_dac_dma.h"
#include "signal_dac_dma_platform.h"
#include "signal_dac_wave_table.h"
#include "signal_dds.h"
#include "signal_sine.h"
#include "ti_msp_dl_config.h"

static uint16_t g_dds_lookup_table[SIGNAL_DDS_TABLE_COUNT];
static uint16_t g_dds_dma_buffer[SIGNAL_DDS_DMA_BUFFER_COUNT];
static signal_dds_t g_dds;
static signal_dac_dma_t g_dac_dma;

volatile signal_result_t g_dds_status;
volatile float g_dds_configured_frequency_hz;
volatile uint32_t g_dds_configured_update_rate_hz;

int main(void)
{
    signal_dac_wave_table_t table = {
        g_dds_lookup_table, SIGNAL_DDS_TABLE_COUNT, SIGNAL_DAC_BITS
    };
    float offset_fraction = SIGNAL_DDS_OFFSET_V / SIGNAL_DAC_VREF_V;
    float amplitude_fraction =
        SIGNAL_DDS_AMPLITUDE_PEAK_V / SIGNAL_DAC_VREF_V;
    float phase_cycles = SIGNAL_DDS_PHASE_DEG / 360.0f;

    SYSCFG_DL_init();
    g_dds_status = SignalSine_Generate(&table, offset_fraction,
        amplitude_fraction, phase_cycles);
    if (g_dds_status != SIGNAL_RESULT_OK) goto fail;
    g_dds_status = SignalDDS_Init(&g_dds, g_dds_lookup_table,
        SIGNAL_DDS_TABLE_COUNT, SIGNAL_DDS_FREQUENCY_HZ,
        (float) SIGNAL_DAC_UPDATE_RATE_HZ, 0U);
    if (g_dds_status != SIGNAL_RESULT_OK) goto fail;
    g_dds_status = SignalDDS_Fill(
        &g_dds, g_dds_dma_buffer, SIGNAL_DDS_DMA_BUFFER_COUNT);
    if (g_dds_status != SIGNAL_RESULT_OK) goto fail;
    g_dds_status = SignalDACPlatform_Init(
        SIGNAL_DAC_UPDATE_RATE_HZ, CPUCLK_FREQ);
    if (g_dds_status != SIGNAL_RESULT_OK) goto fail;
    g_dds_status = SignalDACDMA_Init(&g_dac_dma, NULL,
        SignalDACPlatform_Start, SignalDACPlatform_Stop);
    if (g_dds_status != SIGNAL_RESULT_OK) goto fail;
    g_dds_status = SignalDACDMA_Start(&g_dac_dma, g_dds_dma_buffer,
        SIGNAL_DDS_DMA_BUFFER_COUNT, true);
    if (g_dds_status != SIGNAL_RESULT_OK) goto fail;

    g_dds_configured_update_rate_hz =
        SignalDACPlatform_GetConfiguredRate();
    g_dds_configured_frequency_hz = SignalDDS_GetConfiguredFrequency(
        &g_dds, (float) g_dds_configured_update_rate_hz);
    while (1) __WFI();
fail:
    __BKPT(0);
    while (1) __WFI();
}
