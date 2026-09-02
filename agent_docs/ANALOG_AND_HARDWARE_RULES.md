# 模拟与硬件分析规则

只有题目涉及 ADC、DAC、DDS、OPA/PGA、Comparator、滤波、弱信号或外部模拟器件时读取。

## 证据顺序

1. 当前模块 README 与公开 `.h`。
2. 目标 `.syscfg`、生成 header 与原理图。
3. 仓库本地 `LP-MSPM0G3507.pdf`、板卡资料、exact device datasheet。
4. 没有 exact 资料时，通用工程知识必须标为待 Datasheet 确认。

禁止凭模型记忆给出具体芯片 Pin、供电范围、logic level、SPI mode、寄存器、采样率、ENOB、GBW、SR 或失真指标。

## 必查项

- ADC：输入范围、reference、resolution/ENOB、Fs、每周期点数、源阻/建立、同步与抗混叠。
- DAC/DDS：更新率、点/周期、settling、slew、负载、重建滤波、SFDR/THD；Software DDS + DAC 不等于外置 DDS。
- OPA/PGA：闭环增益、`required GBW ≈ f × gain × margin`、`SR=2πfVpk`、共模/摆幅、offset/noise、负载与稳定性。
- Comparator/Capture：阈值、迟滞、传播延迟、输入噪声、Timer clock/overflow。
- 滤波：截止/带宽、阶数、幅相、源/负载阻抗、公差与群延迟。
- 电气安全：供电、共地、输入保护、负压/过压、5 V logic、上拉、反向供电、背光/负载电流。

## 测量链原则

数据换算必须说明：ADC code→引脚电压→前端反算→DUT 物理量；VREF、gain/offset、量程、频响和通道 delay 都有版本/条件。削顶、带宽不足、量程切换过渡和错误接线不能靠算法修复。

频率响应测量优先同步测 Vin/Vout，或至少做 thru/fixture 校准；只相信 DDS 设定幅值会把信号源频响混入 DUT。弱信号优先 LockIn/SineFit 等有明确条件的方法，不能无条件靠更大的 FFT。

## 板级验证

Build 正确与硬件正确分开。逐级验证：电源/共地/静态 Pin→最小外设→单模块→已知输入/输出→完整 Pipeline→min/typ/max 指标。记录仪器、接线、条件、实际值和失败现象；没有记录不能升级 `BOARD_VERIFIED`。

