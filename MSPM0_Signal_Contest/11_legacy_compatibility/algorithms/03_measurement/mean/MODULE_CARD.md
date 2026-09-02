# MODULE CARD

RECOMMENDED_LEVEL: LEVEL_A_DIRECT_RECIPE；本卡片描述旧兼容 API，新工程见 `00_docs/recipes/mean.md`。

MODULE: Mean

CATEGORY: Measurement / Statistics

功能：计算样本算术平均值。

输入：`const float samples[]`、`count`；单位由输入决定。

输出：`mean_value`，与输入相同单位。

是否原地处理：不适用；不修改输入。

依赖：公共算法状态码。

典型用途：DC 电压、RemoveDC 的概念验证、多次重复测量平均。

不要用于：先 RemoveDC 后还想恢复原 DC；快速变化信号却用很长记录宣称“瞬时值”。

计算量：LOW，O(N)。

RAM：内部 O(1)，约少量标量；无输出数组。

Benefits：降低零均值随机噪声对 DC 估计的影响；补偿求和减小累计误差。

Trade-offs：平均会抹平时间变化，延迟随记录长度增大。

可连接：`ADC_ToVoltage -> Mean -> DC result`。

状态：PC_VERIFIED。2026-08-07 通过 PC GCC 严格编译和真值测试；尚未 BOARD_VERIFIED。
