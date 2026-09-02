/**
 * @file main.c
 * @brief LP-MSPM0G3507 板载 TMP6131 的 ADC_DMA LEVEL 1 验收。
 *
 * min/max/mean、哨兵、FCC 和 100 帧统计全部属于 TEST ONLY 层；
 * 正式 signal_adc_dma 模块不依赖 TMP6131、LFXT、FCC 或这些全局变量。
 */

#include <stdbool.h>
#include <stdint.h>

#include "signal_adc_dma.h"
#include "signal_config.h"
#include "ti_msp_dl_config.h"

typedef enum {
    SELFTEST_FAILURE_NONE = 0,
    SELFTEST_FAILURE_CLOCK_REFERENCE,
    SELFTEST_FAILURE_INIT,
    SELFTEST_FAILURE_TRIGGER_RATE,
    SELFTEST_FAILURE_START,
    SELFTEST_FAILURE_STATUS,
    SELFTEST_FAILURE_METADATA,
    SELFTEST_FAILURE_SENTINEL,
    SELFTEST_FAILURE_ALL_ZERO,
    SELFTEST_FAILURE_ALL_FULL_SCALE,
    SELFTEST_FAILURE_MEAN_RANGE
} selftest_failure_t;

/** 非 static，便于 CCS Graph/Memory Browser 直接输入符号名。 */
uint16_t g_adc_buffer[SIGNAL_SELFTEST_MAX_SAMPLE_COUNT];

volatile uint16_t g_current_sample_count;
volatile uint16_t g_failed_sample_count;
volatile uint16_t g_adc_raw_min;
volatile uint16_t g_adc_raw_max;
volatile uint32_t g_adc_raw_mean;
volatile uint32_t g_sentinel_residue_count;
volatile uint32_t g_completed_blocks;
volatile uint32_t g_total_completed_blocks;
volatile uint32_t g_completed_sizes;
volatile uint32_t g_failed_block;
volatile uint32_t g_wfe_return_count;
volatile uint32_t g_wfe_completed_blocks;
volatile uint32_t g_configured_trigger_rate_hz;
volatile uint32_t g_fcc_count;
volatile uint32_t g_measured_sysosc_hz;
volatile uint32_t g_estimated_trigger_rate_hz;
volatile int32_t g_sysosc_error_ppm;
volatile bool g_fcc_done;
volatile bool g_fcc_pass;
volatile bool g_mean_reasonable;
volatile bool g_acceptance_complete;
volatile bool g_acceptance_pass;
volatile signal_result_t g_last_result;
volatile signal_status_t g_module_status;
volatile selftest_failure_t g_selftest_failure;

static const uint16_t kSampleCounts[] = {256U, 512U, 1024U, 2048U, 4096U};

static void SelfTest_FillSentinel(uint16_t sample_count)
{
    uint16_t i;

    for (i = 0U; i < sample_count; i++) {
        g_adc_buffer[i] = UINT16_MAX;
    }
}

static void SelfTest_CalculateStats(uint16_t sample_count)
{
    uint16_t i;
    uint16_t minimum = UINT16_MAX;
    uint16_t maximum = 0U;
    uint32_t sum = 0U;
    uint32_t residue_count = 0U;

    for (i = 0U; i < sample_count; i++) {
        const uint16_t sample = g_adc_buffer[i];

        if (sample > 4095U) {
            residue_count++;
            continue;
        }
        if (sample < minimum) {
            minimum = sample;
        }
        if (sample > maximum) {
            maximum = sample;
        }
        sum += sample;
    }

    g_sentinel_residue_count = residue_count;
    if (residue_count == 0U) {
        g_adc_raw_min = minimum;
        g_adc_raw_max = maximum;
        g_adc_raw_mean =
            (sum + ((uint32_t) sample_count / 2U)) / sample_count;
        g_mean_reasonable =
            (g_adc_raw_mean >= SIGNAL_SELFTEST_REASONABLE_RAW_MIN) &&
            (g_adc_raw_mean <= SIGNAL_SELFTEST_REASONABLE_RAW_MAX);
    } else {
        g_adc_raw_min = 0U;
        g_adc_raw_max = 0U;
        g_adc_raw_mean = 0U;
        g_mean_reasonable = false;
    }
}

/** 使用 TI 官方 FCC 流程，以两个 32.768 kHz 周期统计 SYSOSC 周期数。 */
static bool SelfTest_MeasureSYSOSC(void)
{
    int64_t frequency_delta;

    DL_SYSCTL_startFCC();
    while (!DL_SYSCTL_isFCCDone()) {
        /* FCC 由 LFXT/LFCLK 硬件触发，不使用 ADC Timer 或软件延时。 */
    }

    g_fcc_count = DL_SYSCTL_readFCC();
    g_fcc_done = true;
    g_measured_sysosc_hz =
        (g_fcc_count * SIGNAL_SELFTEST_LFXT_HZ) /
        SIGNAL_SELFTEST_FCC_REFERENCE_PERIODS;
    frequency_delta = (int64_t) g_measured_sysosc_hz -
                      (int64_t) SIGNAL_SELFTEST_SYSOSC_NOMINAL_HZ;
    g_sysosc_error_ppm = (int32_t) ((frequency_delta * 1000000LL) /
                                     SIGNAL_SELFTEST_SYSOSC_NOMINAL_HZ);
    g_fcc_pass =
        (g_measured_sysosc_hz >= SIGNAL_SELFTEST_SYSOSC_MIN_HZ) &&
        (g_measured_sysosc_hz <= SIGNAL_SELFTEST_SYSOSC_MAX_HZ);
    return g_fcc_pass;
}

static uint32_t SelfTest_GetNominalTimerCount(void)
{
    return (SIGNAL_SELFTEST_SYSOSC_NOMINAL_HZ +
               (SIGNAL_SELFTEST_SAMPLE_RATE_HZ / 2U)) /
           SIGNAL_SELFTEST_SAMPLE_RATE_HZ;
}

static void SelfTest_Fail(selftest_failure_t failure)
{
    g_selftest_failure = failure;
    g_module_status = SignalADC_GetStatus();
    g_acceptance_complete = true;
    g_acceptance_pass = false;
    __BKPT(0);

    while (1) {
        __WFI();
    }
}

int main(void)
{
    uint32_t size_index;
    uint32_t block_index;
    const signal_adc_dma_config_t adc_config = {
        .sample_rate_hz = SIGNAL_SELFTEST_SAMPLE_RATE_HZ,
        .timer_clock_hz = SIGNAL_SELFTEST_SYSOSC_NOMINAL_HZ,
        .timer_max_count = 65536U,
    };

    SYSCFG_DL_init();

    g_failed_sample_count = 0U;
    g_failed_block = UINT32_MAX;
    g_last_result = SIGNAL_RESULT_OK;
    g_module_status = MODULE_IDLE;
    g_selftest_failure = SELFTEST_FAILURE_NONE;
    g_acceptance_complete = false;
    g_acceptance_pass = false;
    g_fcc_done = false;
    g_fcc_pass = false;
    g_completed_blocks = 0U;
    g_total_completed_blocks = 0U;
    g_completed_sizes = 0U;
    g_wfe_return_count = 0U;
    g_wfe_completed_blocks = 0U;

    if (!SelfTest_MeasureSYSOSC()) {
        SelfTest_Fail(SELFTEST_FAILURE_CLOCK_REFERENCE);
    }

    g_estimated_trigger_rate_hz =
        g_measured_sysosc_hz / SelfTest_GetNominalTimerCount();

    g_last_result = SignalADC_Init(&adc_config);
    if (g_last_result != SIGNAL_RESULT_OK) {
        SelfTest_Fail(SELFTEST_FAILURE_INIT);
    }

    g_configured_trigger_rate_hz =
        SignalADC_GetConfiguredTriggerRate();
    if (g_configured_trigger_rate_hz !=
        SIGNAL_SELFTEST_SAMPLE_RATE_HZ) {
        SelfTest_Fail(SELFTEST_FAILURE_TRIGGER_RATE);
    }

    for (size_index = 0U;
         size_index < (sizeof(kSampleCounts) / sizeof(kSampleCounts[0]));
         size_index++) {
        g_current_sample_count = kSampleCounts[size_index];
        g_completed_blocks = 0U;

        for (block_index = 0U;
             block_index < SIGNAL_SELFTEST_BLOCKS_PER_SIZE;
             block_index++) {
            g_failed_sample_count = g_current_sample_count;
            g_failed_block = block_index;
            SelfTest_FillSentinel(g_current_sample_count);

            g_last_result = SignalADC_Start(
                g_adc_buffer, g_current_sample_count);
            if (g_last_result != SIGNAL_RESULT_OK) {
                SelfTest_Fail(SELFTEST_FAILURE_START);
            }

            while (!SignalADC_IsFinished()) {
                __WFE();
                g_wfe_return_count++;
            }
            g_wfe_completed_blocks++;

            g_module_status = SignalADC_GetStatus();
            if (g_module_status != MODULE_DONE) {
                SelfTest_Fail(SELFTEST_FAILURE_STATUS);
            }
            if ((SignalADC_GetBuffer() != g_adc_buffer) ||
                (SignalADC_GetSampleCount() != g_current_sample_count)) {
                SelfTest_Fail(SELFTEST_FAILURE_METADATA);
            }

            SelfTest_CalculateStats(g_current_sample_count);
            if (g_sentinel_residue_count != 0U) {
                SelfTest_Fail(SELFTEST_FAILURE_SENTINEL);
            }
            if ((g_adc_raw_min == 0U) && (g_adc_raw_max == 0U)) {
                SelfTest_Fail(SELFTEST_FAILURE_ALL_ZERO);
            }
            if ((g_adc_raw_min == 4095U) &&
                (g_adc_raw_max == 4095U)) {
                SelfTest_Fail(SELFTEST_FAILURE_ALL_FULL_SCALE);
            }
            if (!g_mean_reasonable) {
                SelfTest_Fail(SELFTEST_FAILURE_MEAN_RANGE);
            }

            g_completed_blocks = block_index + 1U;
            g_total_completed_blocks++;
        }
        g_completed_sizes = size_index + 1U;
    }

    g_failed_sample_count = 0U;
    g_failed_block = UINT32_MAX;
    g_acceptance_complete = true;
    g_acceptance_pass = true;
    __BKPT(0);

    while (1) {
        __WFI();
    }
}
