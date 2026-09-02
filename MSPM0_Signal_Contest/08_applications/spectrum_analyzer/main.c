#include <stdint.h>

#include "signal_adc_dma.h"
#include "signal_config.h"
#include "signal_integration.h"
#include "ti_msp_dl_config.h"

static uint16_t g_raw[SIGNAL_SAMPLE_COUNT];
static float g_voltage[SIGNAL_SAMPLE_COUNT];
static signal_complex_f32_t g_fft[SIGNAL_SAMPLE_COUNT];
static float g_magnitude[(SIGNAL_SAMPLE_COUNT / 2U) + 1U];

signal_spectrum_integration_result_t g_spectrum_result;
volatile signal_algorithm_status_t g_spectrum_status;

int main(void)
{
    signal_adc_dma_config_t adc_config = {
        SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
    };

    SYSCFG_DL_init();
    if (SignalADC_Init(&adc_config) != SIGNAL_RESULT_OK) goto fail;
    if (SignalADC_Start(g_raw, SIGNAL_SAMPLE_COUNT) != SIGNAL_RESULT_OK) goto fail;
    while (!SignalADC_IsFinished()) __WFE();
    g_spectrum_status = SignalIntegration_RawToVoltage(
        SignalADC_GetBuffer(), SignalADC_GetSampleCount(), SIGNAL_ADC_BITS,
        SIGNAL_ADC_VREF_V, SIGNAL_INPUT_SCALE, SIGNAL_INPUT_OFFSET_V,
        g_voltage, SIGNAL_SAMPLE_COUNT);
    if (g_spectrum_status != SIGNAL_ALGORITHM_OK) goto fail;
    g_spectrum_status = SignalIntegration_Spectrum(g_voltage,
        SIGNAL_SAMPLE_COUNT, (float) SignalADC_GetConfiguredTriggerRate(),
        (float) SIGNAL_EXPECTED_FREQ_MIN_HZ,
        (float) SIGNAL_EXPECTED_FREQ_MAX_HZ, g_fft, SIGNAL_SAMPLE_COUNT,
        g_magnitude, (SIGNAL_SAMPLE_COUNT / 2U) + 1U,
        SIGNAL_SPECTRUM_PEAK_COUNT, &g_spectrum_result);
    if (g_spectrum_status != SIGNAL_ALGORITHM_OK) goto fail;
    __BKPT(0);
    while (1) __WFI();

fail:
    __BKPT(0);
    while (1) __WFI();
}
