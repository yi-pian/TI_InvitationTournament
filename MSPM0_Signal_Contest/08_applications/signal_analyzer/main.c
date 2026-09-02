#include <stdint.h>

#include "signal_analyzer_pipeline.h"
#include "signal_config.h"
#include "signal_dual_adc_platform.h"
#include "ti_msp_dl_config.h"

static uint16_t g_raw_a[SIGNAL_SAMPLE_COUNT];
static uint16_t g_raw_b[SIGNAL_SAMPLE_COUNT];

volatile signal_analyzer_pipeline_result_t g_signal_analyzer_result;
volatile int32_t g_signal_analyzer_status;

int main(void)
{
    signal_analyzer_pipeline_result_t result;
    signal_result_t hardware_status;
    signal_algorithm_status_t algorithm_status;

    SYSCFG_DL_init();
    hardware_status = SignalDualADCPlatform_Init(
        SIGNAL_SAMPLE_RATE_HZ, CPUCLK_FREQ);
    if (hardware_status != SIGNAL_RESULT_OK) { goto fail; }
    hardware_status = SignalDualADCPlatform_Start(
        g_raw_a, g_raw_b, SIGNAL_SAMPLE_COUNT);
    if (hardware_status != SIGNAL_RESULT_OK) { goto fail; }
    while (!SignalDualADCPlatform_IsFinished()) { __WFI(); }

    algorithm_status = SignalAnalyzerPipeline_Process(g_raw_a, g_raw_b,
        SignalDualADCPlatform_GetConfiguredRate(), &result);
    if (algorithm_status != SIGNAL_ALGORITHM_OK) {
        g_signal_analyzer_status = (int32_t) algorithm_status;
        goto fail;
    }
    g_signal_analyzer_result = result;
    g_signal_analyzer_status = 0;
    while (1) { __WFI(); }

fail:
    if (g_signal_analyzer_status == 0) {
        g_signal_analyzer_status = (int32_t) hardware_status;
    }
    __BKPT(0);
    while (1) { __WFI(); }
}
