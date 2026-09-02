# MODULE CARD

RECOMMENDED_LEVEL: INTERNAL_CHILD_ALIAS；用户应调用父级 Window Dispatcher。

MODULE: Blackman Window

CATEGORY: DSP / Window

功能：用两项余弦把远旁瓣压得更低；支持原地。

输入：float samples、count；输出：windowed float、coherent_gain。

主瓣趋势：四种中最宽。旁瓣趋势：四种中最低。相干增益约 0.42，函数实算。

Benefits：强谱线旁观察弱分量。Trade-offs：相邻频率更难分开、幅值修正、每点两次 cosf。

不要用于：近邻分辨优先却没有评估主瓣重叠。

计算量 MEDIUM/HIGH；RAM O(1)。状态：PC_VERIFIED；未实板。
