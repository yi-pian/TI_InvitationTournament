# 算法实现等级规则

这份规则回答一个问题：一个计算应该直接写在应用层，保留成简单函数，还是值得作为正式算法模块？判断依据是比赛现场使用成本，不是仓库里是否已经存在 `.c/.h`。

## LEVEL A：DIRECT RECIPE

适合 5～30 行左右、没有状态、没有 workspace、公式和边界都容易看懂的计算。

使用方式：打开 [SIGNAL_ALGORITHM_COOKBOOK.md](SIGNAL_ALGORITHM_COOKBOOK.md)，进入对应 Recipe，把“比赛现场直接复制这一段”粘贴到 `main.c` 或 `signal_processing.c`。不需要复制模块源码，不需要公共状态码，不需要 Init。

典型内容：平均值、Min/Max、Vpp、普通 RMS、AC RMS、Remove DC、ADC code 转电压、简单比例/偏移、归一化、阈值判断、简单多周期平均。

## LEVEL B：SIMPLE HELPER

适合仍有复用价值、但不需要对象生命周期的单一函数。公开入口应保持为“传入数组和少量参数，立即得到输出”；不应有 Init、Reset、状态机或隐藏动态内存。

当前推荐入口只有：

- `Moving Average`：滑动窗口边界和运行和复用价值明确。
- `Window Gain Correction`：DC/Nyquist 的单边幅值规则容易写错，保留一个无状态 Helper 比每次现场重写更安全。

## LEVEL C：REAL ALGORITHM MODULE

适合实现复杂、数值细节重要、需要 workspace/状态/多输出、涉及 Backend，或在多个 Application 中反复使用的算法。

使用方式：按该模块 README 的复制清单复制正式 `.c/.h`，再按 README 建立变量、调用和验证。算法模块本身不修改 SysConfig。

典型内容：FFT、Window Dispatcher、FFT Magnitude/标度、插值、FIR/IIR、Correlation、Autocorrelation、Hampel/MAD、Harmonic/THD、Phase、Sine Fit、Calibration。

## 判定问题

依次问：

1. 正确实现是否不超过约 30 行？
2. 是否没有跨帧状态、Init/Reset、workspace 和容量协商？
3. 是否没有 Backend、定点缩放和复杂数值稳定问题？
4. 输入输出是否能用一两个标量或数组直接说明？
5. 调用模块是否比粘贴公式需要更多新概念？

前四项大多为“是”，并且第 5 项为“是”，优先 Level A。只有一个稳定的无状态函数边界值得复用时用 Level B。其余用 Level C。

## 兼容策略

原有 Level A `.c/.h` 暂不删除，因为已经 BUILD_VERIFIED 的 Application 仍在调用它们。它们被标为 `COMPATIBILITY_API`：

- 旧工程可以继续使用；
- 新比赛工程默认不再复制；
- 不再给它们增加 config/context/result 层；
- 后续只有完成调用方迁移和回归测试后，才考虑物理删除。

这不是同时维护两套算法：推荐实现只有 Recipe；旧 `.c/.h` 只是冻结的兼容入口。

## 验证状态

Recipe 代码通过编译和 PC 真值测试只能写 `PC_VERIFIED`。没有真实开发板和信号源验证，不得写 `BOARD_VERIFIED` 或 `CONTEST_VERIFIED`。
