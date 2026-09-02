/* dac_wave_table 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_dac_wave_table.h"

void dac_wave_table_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_dac_wave_table_t signaldacwavetable_validate_arg0 = {0};
    /* ===== 调用 SignalDACWaveTable_Validate：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDACWaveTable_Validate(&signaldacwavetable_validate_arg0);

    /* ===== 调用 SignalDACWaveTable_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDACWaveTable_GetModuleStatus();

    static float signaldacwavetable_normalizedtoraw_arg0 = 0.0f;
    static uint8_t signaldacwavetable_normalizedtoraw_arg1 = 0U;
    static float signaldacwavetable_normalizedtoraw_arg2 = 0.0f;
    static float signaldacwavetable_normalizedtoraw_arg3 = 0.0f;
    static uint16_t signaldacwavetable_normalizedtoraw_arg4[16] = {0};
    /* ===== 调用 SignalDACWaveTable_NormalizedToRaw：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalDACWaveTable_NormalizedToRaw(signaldacwavetable_normalizedtoraw_arg0, signaldacwavetable_normalizedtoraw_arg1, signaldacwavetable_normalizedtoraw_arg2, signaldacwavetable_normalizedtoraw_arg3, signaldacwavetable_normalizedtoraw_arg4);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

