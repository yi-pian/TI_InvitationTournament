#include <stddef.h>
#include <stdint.h>

#include "signal_adc_dma.h"
#include "signal_adc_to_voltage.h"
#include "signal_config.h"
#include "signal_dac_dma.h"
#include "signal_dac_dma_platform.h"
#include "signal_dac_wave_table.h"
#include "signal_dds.h"
#include "signal_frequency_sweep.h"
#include "signal_lock_in.h"
#include "signal_sine.h"
#include "signal_sweep_analyzer.h"
#include "ti_msp_dl_config.h"

static uint16_t g_raw[SIGNAL_SAMPLE_COUNT];
static float g_voltage_v[SIGNAL_SAMPLE_COUNT];
static uint16_t g_dds_table[SIGNAL_DDS_TABLE_COUNT];
static uint16_t g_dds_block[SIGNAL_DDS_DMA_BUFFER_COUNT];
static float g_sweep_frequency_hz[SIGNAL_SWEEP_POINT_COUNT];

volatile signal_sweep_point_result_t
    g_sweep_results[SIGNAL_SWEEP_POINT_COUNT];
volatile uint32_t g_sweep_completed_points;
volatile int32_t g_sweep_status;

static signal_dds_t g_dds;
static signal_dac_dma_t g_dac_dma;

static void Sweep_Fail(int32_t status)
{
    g_sweep_status = status;
    __BKPT(0);
    while (1) { __WFI(); }
}

int main(void)
{
    signal_adc_dma_config_t adc_config = {
        SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ, 65536U
    };
    signal_dac_wave_table_t table = {
        g_dds_table, SIGNAL_DDS_TABLE_COUNT, SIGNAL_DAC_BITS
    };
    signal_frequency_sweep_config_t sweep_config = {
        SIGNAL_SWEEP_START_FREQ_HZ, SIGNAL_SWEEP_STOP_FREQ_HZ,
        SIGNAL_SWEEP_POINT_COUNT, false
    };
    signal_adc_to_voltage_config_t voltage_config = {
        (1UL << SIGNAL_ADC_BITS) - 1UL, SIGNAL_ADC_VREF_V,
        SIGNAL_INPUT_SCALE, SIGNAL_INPUT_OFFSET_V
    };
    size_t point;
    signal_result_t status;

    SYSCFG_DL_init();
    status = SignalFrequencySweep_Generate(&sweep_config,
        g_sweep_frequency_hz, SIGNAL_SWEEP_POINT_COUNT);
    if (status != SIGNAL_RESULT_OK) { Sweep_Fail(status); }
    status = SignalSine_Generate(&table,
        SIGNAL_DDS_OFFSET_V / SIGNAL_DAC_VREF_V,
        SIGNAL_DDS_AMPLITUDE_PEAK_V / SIGNAL_DAC_VREF_V,
        SIGNAL_DDS_PHASE_DEG / 360.0f);
    if (status != SIGNAL_RESULT_OK) { Sweep_Fail(status); }
    status = SignalDDS_Init(&g_dds, g_dds_table, SIGNAL_DDS_TABLE_COUNT,
        g_sweep_frequency_hz[0], (float) SIGNAL_DAC_UPDATE_RATE_HZ, 0U);
    if (status != SIGNAL_RESULT_OK) { Sweep_Fail(status); }
    status = SignalDACPlatform_Init(SIGNAL_DAC_UPDATE_RATE_HZ, CPUCLK_FREQ);
    if (status != SIGNAL_RESULT_OK) { Sweep_Fail(status); }
    status = SignalDACDMA_Init(&g_dac_dma, NULL,
        SignalDACPlatform_Start, SignalDACPlatform_Stop);
    if (status != SIGNAL_RESULT_OK) { Sweep_Fail(status); }
    status = SignalADC_Init(&adc_config);
    if (status != SIGNAL_RESULT_OK) { Sweep_Fail(status); }

    for (point = 0U; point < SIGNAL_SWEEP_POINT_COUNT; ++point) {
        signal_lock_in_config_t lock_config;
        signal_lock_in_result_t lock_result;
        signal_sweep_point_result_t point_result;
        signal_algorithm_status_t algorithm_status;
        uint32_t configured_adc_rate;
        float configured_frequency;

        if (point != 0U) {
            status = SignalDACDMA_Stop(&g_dac_dma);
            if (status != SIGNAL_RESULT_OK) { Sweep_Fail(status); }
        }
        status = SignalDDS_SetFrequency(&g_dds, g_sweep_frequency_hz[point],
            (float) SIGNAL_DAC_UPDATE_RATE_HZ);
        if (status != SIGNAL_RESULT_OK) { Sweep_Fail(status); }
        status = SignalDDS_Fill(&g_dds, g_dds_block,
            SIGNAL_DDS_DMA_BUFFER_COUNT);
        if (status != SIGNAL_RESULT_OK) { Sweep_Fail(status); }
        status = SignalDACDMA_Start(&g_dac_dma, g_dds_block,
            SIGNAL_DDS_DMA_BUFFER_COUNT, true);
        if (status != SIGNAL_RESULT_OK) { Sweep_Fail(status); }

        DL_Common_delayCycles((CPUCLK_FREQ / 1000000U) *
            SIGNAL_SWEEP_SETTLING_TIME_US);
        status = SignalADC_Start(g_raw, SIGNAL_SAMPLE_COUNT);
        if (status != SIGNAL_RESULT_OK) { Sweep_Fail(status); }
        while (!SignalADC_IsFinished()) { __WFI(); }
        configured_adc_rate = SignalADC_GetConfiguredTriggerRate();
        algorithm_status = SignalADCToVoltage_Process(g_raw, g_voltage_v,
            SIGNAL_SAMPLE_COUNT, &voltage_config);
        if (algorithm_status != SIGNAL_ALGORITHM_OK) {
            Sweep_Fail((int32_t) algorithm_status);
        }

        configured_frequency = SignalDDS_GetConfiguredFrequency(&g_dds,
            (float) SignalDACPlatform_GetConfiguredRate());
        lock_config.reference_frequency_hz = configured_frequency;
        lock_config.sample_rate_hz = (float) configured_adc_rate;
        lock_config.reference_phase_rad = -1.57079632679f;
        lock_config.remove_dc = 1U;
        algorithm_status = SignalLockIn_Process(g_voltage_v,
            SIGNAL_SAMPLE_COUNT, &lock_config, &lock_result);
        if (algorithm_status != SIGNAL_ALGORITHM_OK) {
            Sweep_Fail((int32_t) algorithm_status);
        }
        status = SignalSweepAnalyzer_PointAtFrequency(configured_frequency,
            SIGNAL_DDS_AMPLITUDE_PEAK_V, lock_result.amplitude_peak_v,
            lock_result.phase_deg, &point_result);
        if (status != SIGNAL_RESULT_OK) { Sweep_Fail(status); }
        g_sweep_results[point] = point_result;
        g_sweep_completed_points = (uint32_t) point + 1U;
    }

    (void) SignalDACDMA_Stop(&g_dac_dma);
    g_sweep_status = SIGNAL_RESULT_OK;
    while (1) { __WFI(); }
}
