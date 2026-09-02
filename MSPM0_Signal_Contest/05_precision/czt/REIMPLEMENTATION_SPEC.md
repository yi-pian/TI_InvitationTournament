# CZT clean reimplementation specification

状态：`SPECIFICATION_COMPLETE`。类型：`SOURCE_LOST → CLEAN_REIMPLEMENTATION`。

## Scope

本轮建立 **unit-circle、实输入、直接计算** 的可验证正确性基线：

```text
X(f_k)=Σ x[n] exp(-j2π f_k n/Fs)
f_k=f_start+k·f_step
```

它是 CZT 在单位圆等间隔频率弧段上的子集，允许任意起点、步长和输出点数。依据：[Rabiner, Schafer, Rader, The Chirp z-Transform Algorithm, 1969](https://web.ece.ucsb.edu/Faculty/Rabiner/ece259/Reprints/015_czt.pdf)。原论文允许圆/螺旋路径并用卷积加速；当前模块不声称已经实现一般复平面半径或 Bluestein FFT backend。

- 输入：有限 real samples、Fs、起始频率、步长、输出容量。
- 输出：未归一化 `signal_complex_f32_t[M]`，符号与正式 FFT 相同。
- 频率限制：首末频点均在 `[-Fs/2, Fs/2]`；允许正/负步长。
- 复杂度：O(NM)，O(1) 额外 RAM；M0+ 上只适合窄带小 M。宽带或大 M 应使用 FFT/未来 Bluestein backend。
- 预先检查所有输入和保守溢出界限；参数失败时 output 不变。

旧 `SignalCZT_Execute` 的一般性和 buffer 契约未知，新 `SignalCZT_UnitCircleRealDirect` 不做兼容猜测。验证以 Python `cmath.exp` 直接和式为 oracle，覆盖多音、DC、负频率、容量与 Nyquist 边界。
