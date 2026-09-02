# XY / Bode Display Recipe

类型：`RECIPE`。两种图都只需要 TFT 基础图元，不值得再造大型 graphics framework。

## 1. XY 显示

输入是同步的 `x[i]`、`y[i]`；两路必须同采样时刻或经过延时校准：

```text
[MODULE] Dual ADC Sync
→ 两路 [RECIPE] ADC To Voltage
→ 可选 [MODULE] Channel Delay Calibration
→ [RECIPE] fixed/auto x-y mapping
→ [MODULE] TFT DrawLine
```

映射：`screen_x` 随 X 增大向右，`screen_y` 随 Y 增大向上；相邻 `(x[i], y[i])` 连线。N 大时按轨迹抽点即可，但尖锐转角/稀有区域需用分桶保留 extrema。混沌/相图类任务还要保证前端带宽和双通道同步，屏幕形状不是算法分类证据。

## 2. Bode 显示

输入为有效扫频点 `{frequency_hz, gain_db, phase_deg}`：

```text
[MODULE] Frequency Sweep + DDS + DAC DMA
→ DUT
→ [MODULE] Dual ADC Sync（推荐参考/响应双路）
→ [RECIPE] Frequency Response
→ [RECIPE] Bode mapping
→ [MODULE] TFT
```

- Log X：`x ∝ log10(f/f_min)`；若扫频本身线性，也可以显示 log 轴，但点密度会不均。
- Gain Y：固定 dB 范围；绘 `0 dB`、`-3 dB` 基准。
- Phase Y：独立 plot 或明确第二坐标区；避免无法读数的双 Y 轴重叠。
- 无效/未锁定点不连线，显示 gap；不能用 0 dB/0° 冒充。

## 3. Cutoff / marker

截止频率应来自测量 Recipe 的 crossing 插值，不从屏幕像素反推。屏幕只显示计算结果：`fc` marker、gain、phase、valid flag。

## 4. RAM / 刷新

每点保存 3 个 float 约 `12×point_count` bytes，另加 valid/status；可扫一点画一点，但最终重标坐标/重绘时保留结果数组更方便。扫频状态推进优先于刷新，TFT 只在一个点完整测量并保存后更新。

