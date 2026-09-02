# Quinn second estimator clean reimplementation specification

状态：`SPECIFICATION_COMPLETE`。类型：`SOURCE_LOST → CLEAN_REIMPLEMENTATION`；不是旧源码恢复。

## Contract

- 输入/输出、DFT 符号、矩形窗与孤立单音限制与 Jacobsen 模块相同。
- 实现锁定 **Quinn second estimator**，不再使用含糊的“Quinn”称呼。
- 复杂度 O(1)，但包含除法和 `logf`；低资源场景先考虑 Jacobsen。

定义 `β-=Re{X[k-1]/X[k]}`、`β+=Re{X[k+1]/X[k]}`，计算两侧偏移，再使用 Quinn 1997 的非线性 `tau` 修正。公式交叉核对自 [Quinn 1997, DOI 10.1109/78.558515](https://doi.org/10.1109/78.558515) 及作者算法对比页提供的 [`quin2` 代码](https://www.ericjacobsen.org/fe2/quin2.txt)。

拒绝条件：中心不是局部最大、中心功率为零、比值奇异、log 域非法、非有限值或输出超过半 bin。旧 `SignalQuinn_Interpolate` 签名未知；新 API 是 `SignalQuinnSecond_Process`，明确不做 drop-in 兼容。

验证：已知复指数真频率扫描、Python 公式 oracle、C/Python 同输入、零谱/边界/NaN。历史验证不继承。
