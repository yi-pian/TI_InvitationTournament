# MODULE CARD

RECOMMENDED_LEVEL: INTERNAL_CHILD_ALIAS；用户应调用父级 Window Dispatcher。

MODULE: Hann Window

CATEGORY: DSP / Window

功能：用两端为 0 的平滑余弦权重降低泄漏；支持原地；依赖 Window Dispatcher。

输入：float samples、count；输出：windowed float、coherent_gain。

主瓣趋势：比矩形宽。旁瓣趋势：明显低于矩形。相干增益：对称定义为 `(N-1)/(2N)`，接近但不硬等于 0.5。

Benefits：未知单音的稳妥默认。Trade-offs：主瓣变宽、幅值需 CG 修正。

不要用于：严格相干且必须分开很近频率时未比较矩形。

计算量 MEDIUM；RAM O(1)。可连接：`RemoveDC -> Hann -> FFT`。

状态：PC_VERIFIED；完整 0.5 Vpeak FFT 链通过。
