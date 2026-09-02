# MODULE CARD

MODULE: Statistics

CATEGORY: Measurement / Statistics

功能：一次扫描得到 count、均值、极值、总体/样本方差和标准差。

输入：`const float samples[]`、`count`。

输出：`signal_statistics_result_t`；方差单位为输入单位²，其余幅值统计保持输入单位。

是否原地处理：不适用；不修改输入。

依赖：`sqrtf`、公共算法状态码。

典型用途：噪声估计、数据质量摘要、PC/板端验收。

不要用于：把标准差直接当 RMS；非平稳信号却只用一组统计量描述全部细节。

计算量：LOW，O(N)。

RAM：O(1)。

Benefits：Welford 方法在“大 DC + 小波动”下比 `mean(x²)-mean(x)²` 更稳定。

Trade-offs：只给总体摘要，丢失时间顺序；异常点仍会影响均值和方差。

可连接：`ADC_ToVoltage -> Statistics -> quality/result`。

状态：PC_VERIFIED。2026-08-07 通过已知均值/方差测试；未实板验证。
