/* frequency_response_correction 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_frequency_response_correction.h"

void frequency_response_correction_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_frequency_response_correction_point_t signalfrequencyresponsecorrection_process_arg0 = {0};
    static uint32_t signalfrequencyresponsecorrection_process_arg1 = 0U;
    static float signalfrequencyresponsecorrection_process_arg2 = 0.0f;
    static float signalfrequencyresponsecorrection_process_arg3 = 0.0f;
    static float signalfrequencyresponsecorrection_process_arg4 = 0.0f;
    static signal_frc_interpolation_t signalfrequencyresponsecorrection_process_arg5 = 0U;
    static signal_frc_range_policy_t signalfrequencyresponsecorrection_process_arg6 = 0U;
    static signal_frequency_response_correction_result_t signalfrequencyresponsecorrection_process_arg7 = {0};
    /* ===== 调用 SignalFrequencyResponseCorrection_Process：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalFrequencyResponseCorrection_Process(&signalfrequencyresponsecorrection_process_arg0, signalfrequencyresponsecorrection_process_arg1, signalfrequencyresponsecorrection_process_arg2, signalfrequencyresponsecorrection_process_arg3, signalfrequencyresponsecorrection_process_arg4, signalfrequencyresponsecorrection_process_arg5, signalfrequencyresponsecorrection_process_arg6, &signalfrequencyresponsecorrection_process_arg7);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

