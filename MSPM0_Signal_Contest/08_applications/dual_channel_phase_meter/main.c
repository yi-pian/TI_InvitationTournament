#include <stdint.h>

#include "signal_config.h"
#include "signal_dual_adc_platform.h"
#include "signal_integration.h"
#include "ti_msp_dl_config.h"

static uint16_t g_raw_a[SIGNAL_SAMPLE_COUNT];
static uint16_t g_raw_b[SIGNAL_SAMPLE_COUNT];
static float g_voltage_a[SIGNAL_SAMPLE_COUNT];
static float g_voltage_b[SIGNAL_SAMPLE_COUNT];
static signal_complex_f32_t g_fft_a[SIGNAL_SAMPLE_COUNT];
static signal_complex_f32_t g_fft_b[SIGNAL_SAMPLE_COUNT];
static float g_correlation[(2U * SIGNAL_MAX_CORRELATION_LAG) + 1U];

signal_phase_integration_result_t g_phase_result;
volatile signal_algorithm_status_t g_phase_status;

int main(void)
{
    SYSCFG_DL_init();
    if (SignalDualADCPlatform_Init(
            SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ) != SIGNAL_RESULT_OK) {
        goto fail;
    }
    if (SignalDualADCPlatform_Start(
            g_raw_a, g_raw_b, SIGNAL_SAMPLE_COUNT) != SIGNAL_RESULT_OK) {
        goto fail;
    }
    while (!SignalDualADCPlatform_IsFinished()) __WFE();

    g_phase_status = SignalIntegration_RawToVoltage(g_raw_a,
        SIGNAL_SAMPLE_COUNT, SIGNAL_ADC_BITS, SIGNAL_ADC_A_VREF_V,
        SIGNAL_INPUT_A_SCALE, SIGNAL_INPUT_A_OFFSET_V, g_voltage_a,
        SIGNAL_SAMPLE_COUNT);
    if (g_phase_status != SIGNAL_ALGORITHM_OK) goto fail;
    g_phase_status = SignalIntegration_RawToVoltage(g_raw_b,
        SIGNAL_SAMPLE_COUNT, SIGNAL_ADC_BITS, SIGNAL_ADC_B_VREF_V,
        SIGNAL_INPUT_B_SCALE, SIGNAL_INPUT_B_OFFSET_V, g_voltage_b,
        SIGNAL_SAMPLE_COUNT);
    if (g_phase_status != SIGNAL_ALGORITHM_OK) goto fail;
    g_phase_status = SignalIntegration_DualPhase(g_voltage_a, g_voltage_b,
        SIGNAL_SAMPLE_COUNT,
        (float) SignalDualADCPlatform_GetConfiguredRate(),
        SIGNAL_KNOWN_FREQUENCY_HZ, SIGNAL_MAX_CORRELATION_LAG,
        g_fft_a, g_fft_b, SIGNAL_SAMPLE_COUNT, g_correlation,
        (2U * SIGNAL_MAX_CORRELATION_LAG) + 1U, &g_phase_result);
    if (g_phase_status != SIGNAL_ALGORITHM_OK) goto fail;
    __BKPT(0);
    while (1) __WFI();
fail:
    SignalDualADCPlatform_Stop();
    __BKPT(0);
    while (1) __WFI();
}
