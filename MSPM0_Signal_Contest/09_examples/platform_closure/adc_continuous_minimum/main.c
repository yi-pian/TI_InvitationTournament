#include <stddef.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_adc_continuous.h"

static const uint16_t g_frame[] = {1000U, 1100U, 1200U, 1300U};
volatile uint32_t g_callback_count;
volatile uint32_t g_callback_sequence;
volatile int32_t g_adc_continuous_status;

static void consume_frame(void *context, const uint16_t *samples,
    size_t count, uint32_t sequence)
{
    volatile uint32_t *callback_count = (volatile uint32_t *) context;
    if ((samples != NULL) && (count != 0U)) {
        *callback_count += 1U;
        g_callback_sequence = sequence;
    }
}

int main(void)
{
    signal_adc_continuous_t continuous;

    SYSCFG_DL_init();
    g_adc_continuous_status = (int32_t) SignalADCContinuous_Init(
        &continuous, consume_frame, (void *) &g_callback_count);
    if (g_adc_continuous_status == (int32_t) SIGNAL_RESULT_OK) {
        g_adc_continuous_status = (int32_t) SignalADCContinuous_Start(
            &continuous);
    }
    if (g_adc_continuous_status == (int32_t) SIGNAL_RESULT_OK) {
        g_adc_continuous_status = (int32_t) SignalADCContinuous_SubmitFrame(
            &continuous, g_frame, sizeof(g_frame) / sizeof(g_frame[0]));
    }
    if (g_adc_continuous_status == (int32_t) SIGNAL_RESULT_OK) {
        g_adc_continuous_status = (int32_t) SignalADCContinuous_Stop(
            &continuous);
    }
    while (1) __WFI();
}
