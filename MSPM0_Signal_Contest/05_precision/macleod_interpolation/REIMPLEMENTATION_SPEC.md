# Macleod clean reimplementation specification

状态：`SPECIFICATION_COMPLETE`。类型：`SOURCE_LOST → CLEAN_REIMPLEMENTATION`；不是旧源码恢复。

## Contract

- 输入：矩形窗复 FFT 的局部最大峰及左右邻 bin，Fs、N。
- 输出：fractional/interpolated bin 与 Hz。
- 仅用于孤立单音；重叠峰、强谐波、非矩形窗和峰选错均可失效。
- O(1) 时间/RAM，包含一次平方根。

先以中心峰相位对齐：`R[n]=Re{X[n]conj(X[k])}`，再计算：

```text
γ=(R[k-1]-R[k+1])/(2R[k]+R[k-1]+R[k+1])
δ=2γ/(1+sqrt(1+8γ²))
```

第二式是论文形式 `(sqrt(1+8γ²)-1)/(4γ)` 的数值稳定等价式。依据：[Macleod, IEEE TSP 46(1), 1998, DOI 10.1109/78.651200](https://doi.org/10.1109/78.651200) 与 [Jacobsen 对原论文公式的核对](https://www.ericjacobsen.org/fe2/fe2.htm)。

旧 API 签名未知；新 `SignalMacleod_Process` 是 breaking clean API。验证覆盖 fractional-bin 扫描、Python oracle、C float 差分及错误路径，历史状态不继承。
