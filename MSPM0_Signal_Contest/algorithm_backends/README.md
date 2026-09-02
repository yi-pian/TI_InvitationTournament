# Algorithm Backends

## 当前政策

比赛目标环境从工程创建开始就包含 MSPM0 SDK 自带 CMSIS-DSP 1.16.2。普通数组数学、FFT、FIR、Biquad、RMS、Min/Max、Magnitude 和 Correlation 直接使用 CMSIS-DSP，不再为“是否有 CMSIS”建立一层选型抽象。

本目录只保留三类内容：

- `cmsis_dsp/`：已存在 Application 需要的薄兼容 Glue，以及基准测试入口；不是另一份 DSP 核心。
- `reference/`：PC golden/truth comparison；禁止作为新比赛目标默认后端。
- `iqmath/`：MATHACL/RTS 特殊后端实验；只有真实板上 benchmark 证明有价值时使用。

## 新代码怎么写

普通算法直接：

```c
#include "arm_math.h"
```

然后按 `00_docs/CMSIS_DSP_CONTEST_COOKBOOK.md` 调用当前 SDK API。需要高精度峰值插值、稳健估计、谐波识别、THD、校准等竞赛能力时，再连接正式 Contest-specific Module。

## FFT 默认

Q15 是 `DEFAULT_CANDIDATE`，不是已证明最快。Q31/F32 的精度、RAM 和目标链接数据已经记录；执行周期仍需 LP-MSPM0G3507 板测。Reference C 只用于 PC 对照。

## 禁止

- 不再新增 Ref/Q15/Q31/F32 四份普通算法核心。
- 不把 CMSIS 依赖作为模块“能不能选”的条件。
- 不把 PC 数值 PASS 或 target link PASS 写成板上周期结论。
- 不为统一 CMSIS 而删除确有竞赛精度价值的自建算法。
