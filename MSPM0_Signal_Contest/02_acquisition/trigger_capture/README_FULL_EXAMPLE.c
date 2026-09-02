/* 全功能示例：展示软件触发查找、提取与成熟度查询。 */
#include <stddef.h>
#include <stdint.h>

#include "signal_trigger_capture.h"

void SignalTrigger_FullExample(void)
{
    static const uint16_t raw[] = {1000U, 1500U, 1900U, 2110U, 2600U, 2800U};
    uint16_t segment[4];
    size_t trigger_index;
    signal_module_status_t maturity;
    const signal_trigger_config_t config = {
        .level = 2048U,
        .hysteresis = 16U,
        .edge = SIGNAL_TRIGGER_RISING
    };

    if (SignalTrigger_Find(raw, sizeof(raw) / sizeof(raw[0]), &config, 0U,
            &trigger_index) != SIGNAL_RESULT_OK) {
        return;
    }
    if (SignalTrigger_Extract(raw, sizeof(raw) / sizeof(raw[0]),
            trigger_index, 1U, segment,
            sizeof(segment) / sizeof(segment[0])) != SIGNAL_RESULT_OK) {
        return;
    }
    maturity = SignalTrigger_GetModuleStatus();
    (void)maturity;
}
