# 算法选择手册

> 不知道“直接写还是复制模块”时，先看 [SIGNAL_ALGORITHM_COOKBOOK.md](SIGNAL_ALGORITHM_COOKBOOK.md)。本手册继续解释方法选择；其中 Mean/Vpp/RMS/AC RMS/ADC To Voltage/Remove DC/普通 Peak/Multi-Cycle Average 表示相应 **Direct Recipe**，不是要求复制旧模块。

## 我想测：直流电压

- 推荐：`ADC_ToVoltage -> Mean`。
- 若噪声明显：增加采样点数后做 Mean；前提是信号在这段时间内基本不变。
- 不要先 RemoveDC，因为它会把你要测的直流量删掉。

## 我想测：峰峰值 Vpp

- 波形干净、采样覆盖峰谷：`MinMax/Vpp`。
- 有偶发 ADC 毛刺：`Hampel -> RobustVPP/Quantile`（本库已 PC_VERIFIED）。
- 尖峰本身就是待测信号：不能 Hampel/Median，应该保留尖峰并提高带宽与采样率。

## 我想测：RMS

- 包含直流功率的总 RMS：`RMS`。
- 只要交流分量：`AC_RMS`，或 `RemoveDC -> RMS`。
- 非周期记录：RMS 仍可计算，但结果只代表这段记录；不要声称它是稳定周期的长期 RMS。

## 我想测：频率

| 信号情况 | 推荐方法 | 原因 |
|---|---|---|
| 方波、边沿干净 | Timer Capture（硬件模块） | 不必先 ADC，边沿时间精度高 |
| 正弦、SNR 高 | ZeroCross + LinearInterpolation + MultiCycleAverage | 直观、RAM 少、精度高 |
| 正弦、有噪声 | Hann + FFT + PeakInterpolation | 不容易被单次假过零欺骗 |
| 严重失真但周期稳定 | Autocorrelation 或 FFT 基波搜索 | 不依赖单一阈值边沿；本库两条链均已 PC 验证 |
| 频率缓慢变化 | 缩短记录或滑动估计 | 过长平均会抹掉变化 |

过零法若输入有 1.65 V 偏置，选择其中心阈值，或先 RemoveDC 后用 0 V 阈值。

## 我想滤：孤立尖峰

- 少量孤立毛刺：Median。
- 希望根据局部中位数与 MAD 自动判异常：Hampel。
- 不要用于真实脉冲、突发或尖锐边沿测量；这些算法会把有效瞬态当异常。

## 我想降低：随机白噪声

- 允许降低带宽：Moving Average / FIR Low-pass。
- 重复周期：MultiCycleAverage。
- 只测某个已知频率：Lock-in；若参考与采样相干，能用积分时间换窄带 SNR。
- 不要只看“更平滑”；必须检查目标频率的幅值和相位是否被改变。

## 我想分析：频谱

- 周期与记录严格相干：Rectangular 窗分辨相邻频率最好。
- 一般未知频率：Hann 是比赛默认稳妥选择。
- 需要更低旁瓣观察弱分量：Blackman，但主瓣更宽。
- 窗后测幅值必须做 WindowGainCorrection。

## 我想分析：谐波/THD

- 推荐：`RemoveDC -> Hann -> FFT -> MultiBinEnergy -> Harmonic -> THD`。
- 不要在 THD 前加会压低 2~5 次谐波的低通；那是在改变被测对象。
- 基础版本只在近似相干采样时使用单 bin；竞赛版本应对每个谐波积分多个 bin。

## 我想测：两路相位差

- 纯正弦、SNR 高：ZeroCross Phase。
- 已做 FFT、关注某个频率：FFT Phase。
- 波形不一定正弦但形状相似：Cross Correlation Phase。
- 双通道非同时采样时必须先补偿 channel delay，否则算法再精确也会有固定相位误差。

## 我想精确拟合：单一正弦

- 频率已知可靠：SineFit3，直接给幅值、相位、DC 和残差。
- 只有可靠粗频率：SineFit4 在很窄范围局部搜索；CPU 高，先离线验证初值范围。
- 频率未知或多音：先 FFT；不要用 SineFit4 当全频谱搜索器。
- residual 很大：说明频率错、波形失真/削顶、多音或帧内频漂，不能只看拟合出来的漂亮数字。

## 现场快速决策顺序

1. 先确认数据单位和偏置。
2. 明确要保留 DC、谐波、尖峰还是瞬态。
3. 再决定 RemoveDC、滤波、窗口。
4. 最后选择测量器和精度增强模块。

没有一个“万能预处理”适合所有题目。
