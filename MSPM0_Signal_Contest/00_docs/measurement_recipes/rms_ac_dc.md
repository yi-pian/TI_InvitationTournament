# RMS / AC RMS / DC Offset：能量与偏置

本文件包含三个独立但常一起使用的 Recipe。

## Recipe A：总 RMS

- 输入：`voltage_v[N]`。
- 输出：`rms_v=sqrt(mean(x^2))`，包含 DC。

```text
raw → 电压与ADC校准 → 削顶检查 → 平方累加 → 除N → 开方
```

总 RMS 对应整段信号的总均方能量。不要先 RemoveDC；否则测到的是 AC RMS。普通实现为 O(N)、O(1)，直接使用 [RMS Direct Recipe](../recipes/rms.md)。极端离群点不是目标时可用 `SignalRobustRMS_Process(remove_dc=0)`，但必须报告 winsorized 点数。

## Recipe B：AC RMS

- 输入：`voltage_v[N]`。
- 输出：`mean_voltage_v` 和 `ac_rms_v=sqrt(mean((x-mean)^2))`。

```text
电压帧 → 第一遍求均值 → 第二遍求离均差平方 → 平均并开方
```

两遍法让 DC 与 AC 定义清楚，也避免修改输入。可直接使用 [AC RMS Direct Recipe](../recipes/ac_rms.md)。`RemoveDC → RMS` 是等价备选，适合后续还要 FFT；只为一个数值时 AC RMS Recipe 更省 buffer。

## Recipe C：DC 偏置

- 输入：`voltage_v[N]`。
- 输出：`dc_offset_v=mean(x)`，以及可选标准差/置信度。

```text
电压帧 → 检查稳定区间 → Mean
          └→ 需要稳定性时：Welford Statistics → mean/stddev
```

DC 测量不能先 RemoveDC。对“大 DC + 小纹波”需要同时看波动时，复用 `SignalStatistics_Process` 的 Welford 实现比 `mean(x²)-mean(x)²` 数值更稳定。

## 推荐、备选和失效

| 目标 | 默认 | 备选 | 失效/误用 |
|---|---|---|---|
| 总有效值/总功率 | 普通 RMS | RobustRMS | 去 DC 后仍叫总 RMS |
| 交流纹波/正弦幅值比例 | AC RMS | RemoveDC→RMS | 记录不足整周期造成残余均值 |
| 偏置/静态电压 | Mean | Statistics/多帧 Median | RemoveDC；信号在帧内漂移 |

## 采样与周期

- 周期信号最好相干采样；不能相干时至少覆盖 5～20 周期，让端点误差变小。低频不足一周期时，AC RMS 会把趋势误当交流，DC 也只代表这一小段。
- 测宽带噪声 RMS 时，Fs、模拟抗混叠滤波器和测量带宽必须一起说明；RMS 会把带内所有噪声都计入。
- 只测静态 DC 不需要高速 DMA；多次软件触发 ADC 后平均通常更简单，但硬件采集方案不属于算法库。

## 抗噪声、校准与精度

- 随机噪声：延长观察时间或对多帧均方值平均；直接平均多个 RMS 有轻微统计偏差，精度要求高时先平均能量再开方。
- 离群点：先用 MAD 判定是否确为异常；真实脉冲/过冲/谐波不能删除。
- ADC 增益/偏置、VREF、分压和前端增益都会影响绝对伏特值；RMS 比值在同一量程下可消去部分固定增益误差。

## MCU / RAM 与 Primitive

- 三条基础链均 O(N)、O(1)；Direct Recipe 优先。
- `SignalStatistics_Process`：O(N)、O(1)。
- `SignalRobustRMS_Process`：需要 `float workspace[N]`。
- 可选 ADC Gain/Offset Calibration、Clipping Detect。

Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | 总 RMS、AC RMS 和 DC Offset |
| 2 | 输入 | 校准后的 `voltage_v[N]` |
| 3 | 输出 | `rms_v`、`ac_rms_v`、`dc_offset_v`，单位 V |
| 4 | 完整逻辑链 | 见 Recipe A/B/C，三种定义不可互换 |
| 5 | 步骤原因 | 总 RMS 保留 DC；AC RMS 两遍中心化；DC 只求均值/稳定性 |
| 6 | 默认算法 | 严格 C Direct Recipe，两遍 AC RMS |
| 7 | 可选增强 | Welford Statistics、RobustRMS、多帧能量平均 |
| 8 | 适用条件 | 测量带宽、记录区间和是否包含 DC 已定义 |
| 9 | 不适用条件 | 不足周期的漂移帧、削顶、把 Robust 结果冒充真实尾部能量 |
| 10 | 采样率 | 覆盖目标带宽并配合抗混叠滤波 |
| 11 | 点数/周期数 | 周期信号建议 5～20 周期；DC 以稳定时间与噪声要求确定 N |
| 12 | 抗噪 | 多帧能量平均；确认异常非目标后才 MAD/Robust |
| 13 | 精度增强 | 相干记录、校准 VREF/增益/偏置、用 double/补偿累加作离线对照 |
| 14 | 计算量 | O(N) |
| 15 | RAM | Direct Recipe O(1)；RobustRMS 另需 `4N` 字节 workspace |
| 16 | Primitive | Direct RMS/AC RMS/Mean、Statistics、RobustRMS、ADC Calibration |
| 17 | 仓库路径 | `00_docs/recipes/{mean,rms,ac_rms}.md`、`03_measurement/statistics`、`05_precision/robust_rms` |
| 18 | 伪代码 | `validate -> choose total/ac/dc definition -> accumulate -> calibrate -> quality gate` |
| 19 | MCU 调用 | 将 Cookbook 的 `recipe_rms`/`recipe_ac_rms` Direct Copy 块复制进应用后调用 |
| 20 | 失败排查 | 查是否误去 DC、N=0、记录不足周期、溢出/NaN、VREF 和量程状态 |

```c
float dc_v = 0.0f;
float ac_rms_v = recipe_ac_rms(voltage_v, N, &dc_v); /* 来自 ac_rms.md DIRECT_COPY */
float total_rms_v = recipe_rms(voltage_v, N);         /* 来自 rms.md DIRECT_COPY */
```
