# 学习与验收顺序

每一步只新增一个核心能力；前一步没有独立验收，不进入下一步。

| # | 主题 | 需要掌握 | 最小 Demo | 验收标准 |
|---:|---|---|---|---|
| 1 | GPIO/UART | pinmux、电平、波特率、阻塞/中断 | LED + UART echo | 示波器/终端与配置一致 |
| 2 | Timer | 时钟、分频、LOAD、周期 | 周期翻转 GPIO | 实测周期误差可解释 |
| 3 | Timer Capture | 边沿、计数溢出 | 测方波周期 | 多个频率点误差记录 |
| 4 | ADC Basic | 通道、参考、采样时间、码值 | 单次 DC 采样 | 0.5/1.0/2.0 V 码值合理 |
| 5 | Timer Trigger ADC | Event 发布/订阅 | 定时采一个点 | 触发间隔可测 |
| 6 | DMA | 源/目标、宽度、递增、长度 | RAM -> RAM | 数据与长度完全一致 |
| 7 | ADC + DMA | Timer->ADC->DMA | `adc_dma_demo` | N 点、100 次重启、配置触发率与外测节拍正确 |
| 8 | ADC 高速采样 | OPA2365、转换时间、抗混叠 | PB25 高速采集 | 多档 Fs 波形无明显异常 |
| 9 | OPA / GPAMP | 内外部连接、共模、增益 | Buffer/PGA | 增益和摆幅符合预算 |
| 10 | 基础幅值算法 | Mean/Max/Min/Vpp/RMS | 测试向量 | 与离线参考误差一致 |
| 11 | 过零测频 | DC、阈值、迟滞、噪声 | ZeroCross | 多周期计数稳定 |
| 12 | 插值测频 | 线性插值、误差传播 | Interpolation | 相同 N 下优于整点过零 |
| 13 | FFT | bin、复数、缩放、RAM | 单音 FFT | 主峰 bin 正确 |
| 14 | Window | 泄漏、相干增益 | Rect/Hann | 非整 bin 泄漏对比 |
| 15 | FFT 峰值插值 | 三点抛物线 | Peak demo | 频率误差下降 |
| 16 | Harmonic / THD | 基波、邻 bin 能量 | 谐波测试波 | 与信号源/离线参考一致 |
| 17 | Dual ADC / Phase | 同步触发、通道时延 | 双通道相位 | 0/45/90 度误差记录 |
| 18 | DAC | 参考、码值、缓冲 | DAC DC | 万用表/示波器值正确 |
| 19 | DDS | 相位累加、查表、重建滤波 | Sine DDS | 频率/幅度/杂散可测 |
| 20 | Trigger / Ring Buffer | 前触发、后触发、回绕 | 阈值捕获 | 触发位置稳定 |
| 21 | Waveform Replay | 周期提取、幅度映射 | Capture->DAC | 重放波形可解释 |
| 22 | 综合信号分析仪 | 调度、资源冲突、误差预算 | 集成工程 | 全链路可重复验收 |

当前完成到第 7 步的源码、生成和编译链接；第 7 步只有通过 `HARDWARE_ACCEPTANCE_TEST.md` 的实板测试后才算完成，在此之前不进入第 8 步。
