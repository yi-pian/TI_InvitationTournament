# MODULE CARD

RECOMMENDED_LEVEL: INTERNAL_CHILD_ALIAS；用户应调用父级 Window Dispatcher。

MODULE: Hamming Window

CATEGORY: DSP / Window

功能：余弦窗，两端约 0.08 而非 0；支持原地。

输入：float samples、count；输出：windowed float、coherent_gain。

主瓣趋势：与 Hann 同量级。旁瓣趋势：第一旁瓣通常低于 Hann，但远端衰减趋势不同。相干增益由函数实算。

Benefits：常用于一般谱分析。Trade-offs：仍有主瓣加宽和幅值修正。

不要用于：只凭名字认为全面优于 Hann。

计算量 MEDIUM；RAM O(1)。状态：PC_VERIFIED；未实板。
