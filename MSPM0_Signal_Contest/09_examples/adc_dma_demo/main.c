/**
 * @file main.c
 * @brief ADC_DMA 实板准入 Demo：连续完成 100 帧采集并生成原始码统计。
 *
 * 本文件中的 min/max/mean、哨兵检查和验收状态均只属于 Demo/test 层，
 * 不是正式 measurement 模块，也不进入 signal_adc_dma.c。
 */

#include <stdbool.h>
#include <stdint.h>

#include "signal_adc_dma.h"
#include "signal_config.h"
#include "ti_msp_dl_config.h"

/** 验收失败位置，供 CCS Expressions 窗口直接观察。 */
typedef enum {
    VALIDATION_FAILURE_NONE = 0,
    VALIDATION_FAILURE_INIT,
    VALIDATION_FAILURE_START,
    VALIDATION_FAILURE_STATUS,
    VALIDATION_FAILURE_BUFFER_METADATA,
    VALIDATION_FAILURE_BUFFER_NOT_FILLED,
    VALIDATION_FAILURE_TRIGGER_RATE
} validation_failure_t;

static uint16_t g_adc_buffer[SIGNAL_SAMPLE_COUNT];

volatile uint16_t g_adc_raw_min;
volatile uint16_t g_adc_raw_max;
volatile uint32_t g_adc_raw_mean;
volatile uint32_t g_configured_trigger_rate_hz;
volatile uint32_t g_completed_blocks;
volatile uint32_t g_failed_block;
volatile signal_result_t g_last_result;
volatile signal_status_t g_last_module_status;
volatile validation_failure_t g_validation_failure;
volatile bool g_acceptance_complete;
volatile bool g_acceptance_pass;

#if SIGNAL_VALIDATION_TRIGGER_OUTPUT_ENABLE
/**
 * @brief 把 PA12 复用为 TIMG0_CCP0，并在同一 Timer ZERO_EVENT 上硬件翻转。
 *
 * ADC 仍是 Event channel 1 的唯一订阅者。这里直接观察产生该 Event 的
 * TIMG0 零事件，避免逐点 ISR，也不会占用 ADC 的 Event 路由。
 */
static void Validation_InitTriggerOutput(void)
{
    DL_GPIO_initPeripheralOutputFunction(
        VALIDATION_TRIGGER_GPIO_TRIGGER_TOGGLE_IOMUX,
        IOMUX_PINCM34_PF_TIMG0_CCP0);
    DL_GPIO_enableOutput(VALIDATION_TRIGGER_GPIO_PORT,
        VALIDATION_TRIGGER_GPIO_TRIGGER_TOGGLE_PIN);

    DL_TimerG_setCaptureCompareOutCtl(SIGNAL_SAMPLE_TIMER_INST,
        DL_TIMER_CC_OCTL_INIT_VAL_LOW,
        DL_TIMER_CC_OCTL_INV_OUT_DISABLED,
        DL_TIMER_CC_OCTL_SRC_FUNCVAL,
        DL_TIMER_CC_0_INDEX);
    DL_TimerG_setCaptureCompareAction(SIGNAL_SAMPLE_TIMER_INST,
        DL_TIMER_CC_ZACT_CCP_TOGGLE,
        DL_TIMER_CC_0_INDEX);
    DL_TimerG_setCCPDirection(
        SIGNAL_SAMPLE_TIMER_INST, DL_TIMER_CC0_OUTPUT);
}
#endif

/**
 * @brief 用 12-bit ADC 不可能产生的 0xFFFF 填充缓冲区，以发现 DMA 未覆盖点。
 * @param buffer 待填充缓冲区。
 * @param sample_count 元素数。
 */
static void Validation_FillSentinel(uint16_t *buffer, uint16_t sample_count)
{
    uint16_t i;

    for (i = 0U; i < sample_count; i++) {
        buffer[i] = UINT16_MAX;
    }
}

/**
 * @brief 计算一帧原始 ADC 码的 min、max 和四舍五入整数 mean。
 * @param samples 已完成 DMA 搬运的缓冲区。
 * @param sample_count 元素数。
 * @return 所有点均落在 12-bit 范围 0..4095 时返回 true。
 * @note 该函数是验收 Demo 私有统计，不属于 ADC_DMA 正式接口。
 */
static bool Validation_CalculateRawStats(
    const uint16_t *samples, uint16_t sample_count)
{
    uint16_t i;
    uint16_t minimum = UINT16_MAX;
    uint16_t maximum = 0U;
    uint32_t sum = 0U;

    for (i = 0U; i < sample_count; i++) {
        const uint16_t sample = samples[i];

        if (sample > 4095U) {
            return false;
        }
        if (sample < minimum) {
            minimum = sample;
        }
        if (sample > maximum) {
            maximum = sample;
        }
        sum += sample;
    }

    g_adc_raw_min = minimum;
    g_adc_raw_max = maximum;
    g_adc_raw_mean = (sum + ((uint32_t) sample_count / 2U)) /
                     (uint32_t) sample_count;
    return true;
}

/**
 * @brief 用与模块独立的同一整数公式计算本配置应得到的 Timer 事件触发率。
 * @return 由 CPUCLK_FREQ 和目标采样率推导的配置触发率，单位 Hz。
 */
static uint32_t Validation_GetExpectedTriggerRate(void)
{
    const uint32_t timer_count =
        (CPUCLK_FREQ + (SIGNAL_SAMPLE_RATE_HZ / 2U)) /
        SIGNAL_SAMPLE_RATE_HZ;

    return CPUCLK_FREQ / timer_count;
}

/**
 * @brief 记录验收失败现场并停在调试断点。
 * @param failure 失败阶段。
 * @return 不返回。
 */
static void Validation_Fail(validation_failure_t failure)
{
    g_validation_failure = failure;
    g_last_module_status = SignalADC_GetStatus();
    g_acceptance_complete = true;
    g_acceptance_pass = false;
    __BKPT(0);

    while (1) {
        __WFI();
    }
}

/**
 * @brief 初始化硬件后执行固定次数的 Start -> Done 重复采集验收。
 * @return 嵌入式入口不返回。
 */
int main(void)
{
    uint32_t block_index;
    const signal_adc_dma_config_t adc_config = {
        .sample_rate_hz = SIGNAL_SAMPLE_RATE_HZ,
        .timer_clock_hz = CPUCLK_FREQ,
        .timer_max_count = 65536U,
    };

    SYSCFG_DL_init();

#if SIGNAL_VALIDATION_TRIGGER_OUTPUT_ENABLE
    Validation_InitTriggerOutput();
#endif

    g_adc_raw_min = 0U;
    g_adc_raw_max = 0U;
    g_adc_raw_mean = 0U;
    g_configured_trigger_rate_hz = 0U;
    g_completed_blocks = 0U;
    g_failed_block = UINT32_MAX;
    g_last_result = SIGNAL_RESULT_OK;
    g_last_module_status = MODULE_IDLE;
    g_validation_failure = VALIDATION_FAILURE_NONE;
    g_acceptance_complete = false;
    g_acceptance_pass = false;

    g_last_result = SignalADC_Init(&adc_config);
    if (g_last_result != SIGNAL_RESULT_OK) {
        Validation_Fail(VALIDATION_FAILURE_INIT);
    }

    g_configured_trigger_rate_hz =
        SignalADC_GetConfiguredTriggerRate();
    if (g_configured_trigger_rate_hz !=
        Validation_GetExpectedTriggerRate()) {
        Validation_Fail(VALIDATION_FAILURE_TRIGGER_RATE);
    }

    for (block_index = 0U;
         block_index < SIGNAL_ACCEPTANCE_BLOCK_COUNT;
         block_index++) {
        g_failed_block = block_index;
        Validation_FillSentinel(g_adc_buffer, SIGNAL_SAMPLE_COUNT);

        g_last_result = SignalADC_Start(g_adc_buffer, SIGNAL_SAMPLE_COUNT);
        if (g_last_result != SIGNAL_RESULT_OK) {
            Validation_Fail(VALIDATION_FAILURE_START);
        }

        while (!SignalADC_IsFinished()) {
            __WFE();
        }

        g_last_module_status = SignalADC_GetStatus();
        if (g_last_module_status != MODULE_DONE) {
            Validation_Fail(VALIDATION_FAILURE_STATUS);
        }
        if ((SignalADC_GetBuffer() != g_adc_buffer) ||
            (SignalADC_GetSampleCount() != SIGNAL_SAMPLE_COUNT)) {
            Validation_Fail(VALIDATION_FAILURE_BUFFER_METADATA);
        }
        if (!Validation_CalculateRawStats(
                g_adc_buffer, SIGNAL_SAMPLE_COUNT)) {
            Validation_Fail(VALIDATION_FAILURE_BUFFER_NOT_FILLED);
        }

        g_completed_blocks = block_index + 1U;
    }

    g_failed_block = UINT32_MAX;
    g_acceptance_complete = true;
    g_acceptance_pass = true;
    __BKPT(0);

    while (1) {
        __WFI();
    }
}
