/* tft_waveform 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_tft_waveform.h"

void tft_waveform_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static float signaltftwaveform_getenvelopecolumn_arg0[16] = {0};
    static size_t signaltftwaveform_getenvelopecolumn_arg1 = 0U;
    static uint16_t signaltftwaveform_getenvelopecolumn_arg2 = 0U;
    static uint16_t signaltftwaveform_getenvelopecolumn_arg3 = 0U;
    static float signaltftwaveform_getenvelopecolumn_arg4[16] = {0};
    static float signaltftwaveform_getenvelopecolumn_arg5[16] = {0};
    /* ===== 调用 SignalTFTWaveform_GetEnvelopeColumn：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalTFTWaveform_GetEnvelopeColumn(signaltftwaveform_getenvelopecolumn_arg0, signaltftwaveform_getenvelopecolumn_arg1, signaltftwaveform_getenvelopecolumn_arg2, signaltftwaveform_getenvelopecolumn_arg3, signaltftwaveform_getenvelopecolumn_arg4, signaltftwaveform_getenvelopecolumn_arg5);

    /* ===== 调用 SignalTFTWaveform_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalTFTWaveform_GetModuleStatus();

    static tft_ili9341_t signaltftwaveform_draw_arg0 = {0};
    static float signaltftwaveform_draw_arg1[16] = {0};
    static size_t signaltftwaveform_draw_arg2 = 0U;
    static signal_tft_waveform_config_t signaltftwaveform_draw_arg3 = {0};
    static signal_tft_waveform_result_t signaltftwaveform_draw_arg4 = {0};
    /* ===== 调用 SignalTFTWaveform_Draw：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalTFTWaveform_Draw(&signaltftwaveform_draw_arg0, signaltftwaveform_draw_arg1, signaltftwaveform_draw_arg2, &signaltftwaveform_draw_arg3, &signaltftwaveform_draw_arg4);

    static float signaltftwaveform_mapy_arg0 = 0.0f;
    static float signaltftwaveform_mapy_arg1 = 0.0f;
    static float signaltftwaveform_mapy_arg2 = 0.0f;
    static int32_t signaltftwaveform_mapy_arg3 = 0U;
    static uint16_t signaltftwaveform_mapy_arg4 = 0U;
    static int32_t signaltftwaveform_mapy_arg5[16] = {0};
    /* ===== 调用 SignalTFTWaveform_MapY：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalTFTWaveform_MapY(signaltftwaveform_mapy_arg0, signaltftwaveform_mapy_arg1, signaltftwaveform_mapy_arg2, signaltftwaveform_mapy_arg3, signaltftwaveform_mapy_arg4, signaltftwaveform_mapy_arg5);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

