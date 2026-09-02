/* thd 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_thd.h"

void thd_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    static signal_harmonic_result_t signalthd_process_arg0 = {0};
    static signal_thd_result_t signalthd_process_arg1 = {0};
    /* ===== 调用 SignalTHD_Process：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalTHD_Process(&signalthd_process_arg0, &signalthd_process_arg1);

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

