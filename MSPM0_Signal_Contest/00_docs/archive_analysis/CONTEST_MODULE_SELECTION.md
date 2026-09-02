# Contest module selection

| 题目需求 | 首选链路 | 何时换方案 | 主要风险 |
|---|---|---|---|
| 看波形、DC、Vpp、RMS | ADC_DMA -> adc_to_voltage -> oscilloscope | 需要无缝刷新时换 pingpong | RAM、量程、毛刺对 Vpp 的影响 |
| 低/中频正弦高精度频率 | ADC_DMA -> remove_dc -> frequency_interpolation | 噪声大时加多周期平均；带宽更高时 Timer Capture | 过零阈值、采样率误差 |
| 方波/比较器输出频率 | Comparator -> Timer Capture -> frequency_timer_capture | 输入边沿慢时先整形/迟滞 | 回绕、抖动、Timer 时钟误差 |
| 频谱主峰 | ADC_DMA -> remove_dc -> Hann -> FFT -> magnitude -> peak -> parabolic | 只要频率且 RAM 紧张时改过零 | 泄漏、窗增益、N 与 SRAM |
| 谐波/THD | 频谱链 -> harmonic -> THD | 非相干时加 multi-bin energy | 基波 bin 错误、噪声底、混叠 |
| 双通道相位 | adc_dual_sync -> voltage/remove_dc -> correlation -> phase | 已知纯正弦且资源紧时可用过零时差 | 同步性、通道固定延时 |
| 固定波形输出 | wave table -> DAC DMA | 需任意频率时用 DDS | DAC 更新率、重构滤波、码值范围 |
| 任意频率正弦 | sine table -> DDS -> DAC DMA | 极低失真时增大表或做校准 | 相位截断、镜像、RAM |
| 扫频响应 | frequency_sweep -> DDS/DAC -> DUT -> ADC -> amplitude/phase | 无环回硬件时只能 PC 验算法 | 稳态等待、同步、频响校准 |
| 捕获回放 | trigger_capture -> arbitrary_wave -> DAC DMA | 连续长记录用 ring/pingpong | 触发位置、缩放、双缓冲 |

## 决策顺序

1. 能用 Timer Capture 解决的数字边沿问题，不先上 FFT。
2. 能用过零插值解决的单频问题，不先占用 10N bytes FFT 工作区。
3. 需要谐波、多个频率或低 SNR 时再使用 FFT。
4. 双通道只在题目明确要求相位/传递函数时加入。
5. 连续采集只在处理必须无缝时加入；普通单次测量优先块采集。
