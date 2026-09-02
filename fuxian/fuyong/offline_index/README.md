# fuyong 离线复用索引

这是面向断网比赛现场的查阅目录：它不新增功能、模块或工程，只把当前 `fuyong` 教学工程已有的 `main.c` 静态函数、COPY 区、README 与 backup 综合 Example 映射整理为可快速决策的手册。

## 现场使用步骤

1. 先看 [CONTEST_REQUIREMENT_INDEX.md](CONTEST_REQUIREMENT_INDEX.md)，按题目目标选择测频、幅值、相位、频谱、DDS 等方案。
2. 再看 [TOPIC_TO_FUNCTION_INDEX.md](TOPIC_TO_FUNCTION_INDEX.md)，定位对应教学工程和推荐函数。
3. 在 [FUNCTION_INDEX.md](FUNCTION_INDEX.md) 查看函数实际输入、输出、模块和前置函数。
4. 按 [COPY_ORDER_GUIDE.md](COPY_ORDER_GUIDE.md) 的顺序复制完整 `static` 函数与明确列出的变量/模块。
5. 最后查 [FUNCTION_DEPENDENCY_INDEX.md](FUNCTION_DEPENDENCY_INDEX.md)，合并同一帧重复的采集、ADC→电压、去 DC 和 FFT 数据链。

## 文件说明

| 文件 | 现场用途 |
|---|---|
| [FUNCTION_INDEX.md](FUNCTION_INDEX.md) | 按函数查用途、输入输出、来源和复制前置。 |
| [FUNCTION_DEPENDENCY_INDEX.md](FUNCTION_DEPENDENCY_INDEX.md) | 判断数据流、函数依赖与“同帧是否只能执行一次”。 |
| [SIGNAL_CHAIN_RECIPES.md](SIGNAL_CHAIN_RECIPES.md) | 直接按常见比赛任务拼接数据链。 |
| [TOPIC_TO_FUNCTION_INDEX.md](TOPIC_TO_FUNCTION_INDEX.md) | 从“我要做什么”反查工程和函数。 |
| [CONTEST_REQUIREMENT_INDEX.md](CONTEST_REQUIREMENT_INDEX.md) | 按电赛题意选择适合的测量/输出方案。 |
| [QUICK_LOOKUP_TABLE.md](QUICK_LOOKUP_TABLE.md) | 一页式速查。 |
| [EXAMPLE_REUSE_MAP.md](EXAMPLE_REUSE_MAP.md) | 综合 Example 与教学函数边界的对应关系。 |
| [COPY_ORDER_GUIDE.md](COPY_ORDER_GUIDE.md) | 各方案应复制哪些完整函数，以及如何避免重复计算。 |

## 三条现场规则

1. `adc_samples` 是 `uint16_t` ADC code；`voltage_samples` 才是 `float`、单位 V；不要混用。
2. 同一帧的 ADC DMA、ADC→电压、去 DC、FFT 等公共节点只保留一次；尤其 `RunFFTCommon()` 每帧只能一次。
3. 本索引描述的是当前教学工程的真实函数边界。综合题有校准、动态采样率、捕获回放或特殊 UI 时，应保留其 `App_*` 题目逻辑，参见 [EXAMPLE_REUSE_MAP.md](EXAMPLE_REUSE_MAP.md)。
