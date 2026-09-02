/* 最小示例：周期读取旋转编码器状态并处理增量。 */
#include <stdbool.h>
#include <stdint.h>

#include "signal_rotary_encoder.h"
#include "ti_msp_dl_config.h"

static signal_rotary_encoder_t g_encoder;

static signal_result_t ReadA(void *context, bool *high)
{
    (void)context;
    *high = (DL_GPIO_readPins(ENCODER_A_PORT, ENCODER_A_PIN) != 0U);
    return SIGNAL_RESULT_OK;
}

static signal_result_t ReadB(void *context, bool *high)
{
    (void)context;
    *high = (DL_GPIO_readPins(ENCODER_B_PORT, ENCODER_B_PIN) != 0U);
    return SIGNAL_RESULT_OK;
}

static signal_result_t ReadSW(void *context, bool *high)
{
    (void)context;
    *high = (DL_GPIO_readPins(ENCODER_SW_PORT, ENCODER_SW_PIN) != 0U);
    return SIGNAL_RESULT_OK;
}

int main(void)
{
    signal_rotary_encoder_event_t event;
    const signal_rotary_encoder_config_t config = {
        NULL, ReadA, ReadB, ReadSW, 4U, 3U, true
    };

    SYSCFG_DL_init();
    (void)SignalRotaryEncoder_Init(&g_encoder, &config);
    for (;;) {
        if (SignalRotaryEncoder_Update(&g_encoder, &event) ==
            SIGNAL_RESULT_OK) {
            /* event.step_delta changes the selected value; button confirms it. */
        }
        DL_Common_delayCycles(160000U); /* Example only: about 5 ms at 32 MHz. */
    }
}
