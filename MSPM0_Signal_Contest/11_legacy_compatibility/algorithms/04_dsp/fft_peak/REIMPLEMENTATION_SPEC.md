# FFT Peak clean reimplementation specification

状态：`SPECIFICATION_COMPLETE`。类型：`SOURCE_LOST → CLEAN_REIMPLEMENTATION`。

## Design

该模块不再复制“找数组最大值”算法，而是组合正式 `PeakDetect`：在闭区间 `[first_bin,last_bin]` 找首次最大值，再用 `f=bin·Fs/N` 增加频率语义。

- 输入：非负有限 magnitude、数组长度、搜索区间、Fs、FFT N。
- 输出：离散 bin、峰值（单位继承输入）、对应 Hz。
- O(K) 时间、O(1) RAM；不做 fractional-bin 插值。
- 排除 DC 应由调用者把 `first_bin` 设为 1；若要亚 bin 精度，后接 Parabolic/Jacobsen/Quinn/Macleod，不能把离散峰当精修结果。

旧 `SignalFFTPeak_Find` 的结构体和状态类型已丢失。新 `SignalFFTPeak_Process` 使用统一算法状态并复用 `SignalPeakDetect_Process`，属于 breaking clean API。测试覆盖首个并列峰、搜索边界、NaN/负 magnitude、非法 Fs/N。
