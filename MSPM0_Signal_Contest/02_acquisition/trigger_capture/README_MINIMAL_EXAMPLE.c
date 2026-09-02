/* 最小示例：在已经采好的 raw 数组中找上升沿并截取一小段。 */
#include <stddef.h>
#include <stdint.h>

#include "signal_trigger_capture.h"

void SignalTrigger_MinimalExample(void)
{
    static const uint16_t raw[] = {1000U, 1500U, 1900U, 2110U, 2600U, 2800U};
    uint16_t segment[4];
    size_t trigger_index;
    const signal_trigger_config_t config = {
        .level = 2048U,      /* 12-bit ADC 中点附近的触发电平。 */
        .hysteresis = 16U,   /* 抑制阈值附近的小噪声。 */
        .edge = SIGNAL_TRIGGER_RISING
    };

    if (SignalTrigger_Find(raw, sizeof(raw) / sizeof(raw[0]), &config, 0U,
            &trigger_index) == SIGNAL_RESULT_OK) {
        (void)SignalTrigger_Extract(raw, sizeof(raw) / sizeof(raw[0]),
            trigger_index, 1U, segment, sizeof(segment) / sizeof(segment[0]));
    }
}
