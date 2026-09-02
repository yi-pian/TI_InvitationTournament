# Measurement Recipe 编写规范

只有新增/审查测量逻辑链时读取。正式入口：`MSPM0_Signal_Contest/00_docs/MEASUREMENT_RECIPE_INDEX.md`。

## 三层边界

- Primitive：独立可复用、输入输出清楚、值得单测的数值算法，适合 `.c/.h`。
- Recipe：多个 Primitive + 少量标量公式组成完整测量方法，默认 Markdown。
- Application：Recipe + 采集/发生/显示/控制/外设构成赛题功能。

不要为 10～30 行一次性公式创建大型 config/context/result/Init 模块。新工程正式算法唯一来源是 `MSPM0_Signal_Contest/03_measurement/04_dsp/05_precision`；简单算法优先 Recipe。

## 每条 Recipe 的 20 项契约

1. 用途。
2. 输入及单位/ownership。
3. 输出及单位/语义。
4. 完整逻辑链。
5. 每一步存在的原因。
6. 默认推荐算法。
7. 可选增强算法。
8. 适用条件。
9. 不适用条件。
10. 采样率建议。
11. 点数/周期数建议。
12. 抗噪方法。
13. 精度增强方法。
14. 计算量。
15. RAM 需求和生命周期。
16. 调用的现有 Primitive。
17. 当前仓库对应路径。
18. 伪代码。
19. MCU 调用示例；不存在的 API 必须明确标注应用伪代码。
20. 常见失败与排查。

## API 与状态

Recipe 中的 `Signal*` 名必须能在正式 `.h/.c` 找到；运行 `MSPM0_Signal_Contest/tools/algorithm/recipe_validation/recipe_validation.py` 和 `MSPM0_Signal_Contest/tools/api_check/api_check.py`。不存在的 `edge_timing`、LUT、phase unwrap 等只能列缺口，不能写成可调用 API。

Recipe 默认 `DRAFT`。Primitive 的 `PC_VERIFIED` 不会自动升级 Recipe，更不会升级 Application/Board/Contest。

## 方法升级

先使用可解释的最低复杂度方法。只有默认方法在有记录的数据上不满足误差/噪声/时延要求，才升级，并记录默认误差、CPU、peak live RAM、失败样本与增强收益。DRAFT 高级算法、未扫初值的拟合、未查 map 的大 FFT 不作为现场默认。
