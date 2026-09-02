MODULE: Backend Adapter

RECOMMENDED_LEVEL: INTERNAL_SUPPORT；不作为用户功能选择入口。

CATEGORY: DSP / Data Adapter

功能：在 ADC RAW、float 物理量、Q15 和 Q30 能量之间做统一、饱和且单位明确的转换。

输入：`uint16_t ADC RAW`、`float` 或 `int16_t Q15`。

输出：`int16_t Q15`、`float`、`uint64_t Q30 sum`。

是否原地处理：NO。

依赖：统一算法状态码；不依赖 CMSIS-DSP。

典型用途：ADC RAW → Q15 → CMSIS Q15 FFT；Q15 → float 测量结果。

不要用于：把未去除中点偏置的单极性 ADC 码直接解释成交流 Q15。

计算量：LOW，O(N)。

RAM：除调用者输入/输出外为 O(1)。

状态：PC_VERIFIED（由 backend benchmark adapter tests 验证）；BOARD_RUNTIME_VERIFIED 尚未进行。
