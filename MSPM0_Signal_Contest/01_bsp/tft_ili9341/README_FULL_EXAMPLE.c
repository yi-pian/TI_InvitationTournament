/* tft_ili9341 全功能示例：与当前 public header 的全部 Signal* API 一一对应。 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "signal_tft_ili9341.h"

void tft_ili9341_FullExample(void)
{
    /* 这里的对象和数组只是接口演示；比赛接入时改成题目真实的配置、buffer 和单位。 */
    /* ===== 调用 SignalTFTILI9341_GetModuleStatus：先阅读 README 的前置状态和参数单位 ===== */
    (void)SignalTFTILI9341_GetModuleStatus();

    /* 成功后在这里读取输出对象/数组，并交给下一个测量或显示模块。 */
}

