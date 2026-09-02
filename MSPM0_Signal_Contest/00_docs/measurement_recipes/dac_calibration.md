# DAC Calibration：DAC 输出校准

## 输入与输出

- 输入：DAC code/目标电压、由独立校准仪表或已校准 ADC 测得的实际输出、量程/负载/参考/温度。
- 输出：`Vout=a*code+b` 的正向模型、`code=(Vdesired-b)/a` 的反向换算，或非线性 LUT；残差与安全码范围。

## 两点逻辑链

```text
固定负载与VREF → 输出安全低码 → 等待建立 → 测真实Vlow
→ 输出安全高码 → 测真实Vhigh
→ a=(Vhigh-Vlow)/(code_high-code_low)
→ b=Vlow-a*code_low
→ desired_v → round((desired_v-b)/a) → 限幅到安全code
→ 用多个独立中间码验证DNL/INL与负载误差
```

不要用 0 和满量程端点作为唯一参考而忽略输出缓冲器的摆幅限制；低/高码应位于硬件可线性输出区。

## 多点/LUT 分支

```text
递增code表 → 每点稳健测Vout → 检查单调性
→ 保存 code→V LUT
→ 产生目标电压时做反向区间查找+线性插值
→ 最终四舍五入并再次限幅
```

若存在迟滞，升码和降码要分别标定；单一 LUT 不能描述双值关系。

## 推荐、备选与失效

- 近似线性、只需一般精度：两点 affine Recipe。
- 分段非线性且单调：LUT + interpolation。
- 高频波形：静态 code 校准之后还要做 DAC/重建滤波器的幅频与相频校准。
- 用板上 ADC 回读时，ADC 必须先独立校准；否则只能校准“DAC+ADC 环路”，无法分离两者误差。
- 负载、电源、VREF 或输出缓冲改变后原参数可能失效。

适用条件是输出在选定负载下单调且能稳定建立；存在明显迟滞或非单调时，普通单值 LUT 也不适用。

## 采样与精度

- 每个码点等待 DAC、运放和 ADC 建立，再取多点 Median/Mean。
- 静态 DAC 标定没有周期要求；每个 code 建议至少 `N>=32` 个稳定回读样本，波形幅值校准则每个测试频率至少覆盖 5～20 周期。
- 记录量化不确定度；目标电压即使模型精确也只能落到相邻 DAC code。
- 波形幅值校准还需检查 DAC_DMA 更新率、零阶保持、输出滤波器和负载。

## MCU / RAM 与现有 Primitive

- 两点正反算 O(1)；LUT 查找 O(log K) 或 O(K)，RAM 约每点 8 字节（code/voltage，可压缩但先保证清楚）。
- 可复用 ADC Calibration、MAD/Statistics；现有 ADC gain/offset 类型可帮助校正“测量值到真值”，但 DAC code 反解仍应在本 Recipe 明确完成。
- 通用 `calibration_lut_1d` 是 P0 候选，不能为 DAC 再复制一份专用插值循环。

Recipe 状态：`DRAFT`。

## 20 项执行契约

| # | 字段 | 本 Recipe 的约束 |
|---|---|---|
| 1 | 用途 | DAC code↔真实静态输出电压及波形幅值前置校准 |
| 2 | 输入 | code/desired V、独立实测 Vout、负载/VREF/温度 |
| 3 | 输出 | `Vout=a*code+b`、反算 code、LUT、残差/安全码范围 |
| 4 | 完整逻辑链 | 两点正/反算；中间码验证；必要时单调 LUT |
| 5 | 步骤原因 | 避开端点摆幅、等待建立、独立验证 DNL/INL/负载误差 |
| 6 | 默认算法 | 线性区两点 affine + code round/clamp |
| 7 | 可选增强 | 多点 code→V LUT、升/降方向双表、动态频响校准 |
| 8 | 适用条件 | 负载/VREF/缓冲固定，输出在单调线性或可表述区 |
| 9 | 不适用条件 | 迟滞未分向、板上 ADC 未校准却声称分离 DAC 误差 |
| 10 | 采样率 | 静态点按 settling；动态波形另按更新率/带宽 Recipe |
| 11 | 点数/周期数 | 至少低/高+多个独立中间码；每点多次稳定测量 |
| 12 | 抗噪 | 每码点 Median/Mean，重复升降扫查迟滞 |
| 13 | 精度增强 | 增加关键区码点、独立仪表、负载/温度分表、动态频响补偿 |
| 14 | 计算量 | affine O(1)，LUT O(log K) |
| 15 | RAM | affine O(1)，LUT 约每点 8 字节 |
| 16 | Primitive | ADC Calibration、MAD/Statistics；DAC 反解与 LUT 当前 Recipe-local |
| 17 | 仓库路径 | 本 Recipe、`adc_calibration.md`；硬件输出 API 查目标 DAC README/.h |
| 18 | 伪代码 | `set code -> settle -> measure -> affine -> validate -> inverse+round+clamp` |
| 19 | MCU 调用 | 下方为应用公式，不猜 DAC 硬件 API |
| 20 | 失败排查 | 查 code 安全范围、负载/VREF、settling、ADC 校准、单调性、量化/限幅 |

```c
/* APPLICATION RECIPE：DAC 写码函数必须从目标 DriverLib/driver .h 获取。 */
float code_f = (desired_v - offset_v) / volts_per_code;
uint32_t code = (uint32_t)lroundf(code_f); /* 再按已验证安全码范围 clamp */
```
