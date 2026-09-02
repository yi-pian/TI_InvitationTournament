# MODULE CARD

MODULE: Median Filter

CATEGORY: DSP / Robust Filtering

功能：输出局部窗口中位数，去除少量孤立尖峰。

输入：输入/输出数组、奇数 `window_size`、调用者 workspace。

输出：滤波数组，单位不变。

是否原地处理：NO。

依赖：公共算法状态码。

典型用途：ADC 偶发跳码、盐椒型毛刺。

不要用于：真实窄脉冲、上升沿、需要保持频谱/线性相位幅频关系。

计算量：MEDIUM，当前小窗口插入排序约 O(N·W²)。

RAM：输出 `4*N` + workspace `4*W` 字节。

Benefits：对单个极大/极小异常值很强，不会像均值一样被拖走。

Trade-offs：非线性、会改变频谱和边沿、窗口大时计算量快速增加。

可连接：`ADC_ToVoltage -> Median -> Robust measurement`。

状态：PC_VERIFIED。2026-08-07 通过五点脉冲毛刺测试；未实板验证。
