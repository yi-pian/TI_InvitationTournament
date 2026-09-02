# 外设冲突矩阵

符号：✅ 已有 profile 证明资源可同时分配；⚠ 默认分配冲突或需要额外 adapter/重分配；❌ 两个冻结 profile 不能原样叠加；? 尚未建立足够明确的 pin/profile 证据。

| 组合 | 单 ADC | 双 ADC | DAC DMA | Timer Capture | OPA | GPAMP | Comparator | UART0 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 单 ADC | — | ❌ | ✅ | ✅ | ? | ⚠ | ✅ | ✅ |
| 双 ADC | ❌ | — | ✅ | ✅ | ? | ⚠ | ✅ | ✅ |
| DAC DMA | ✅ | ✅ | — | ⚠ | ⚠ | ? | ✅ | ✅ |
| Timer Capture | ✅ | ✅ | ⚠ | — | ? | ? | ✅ | ✅ |
| OPA | ? | ? | ⚠ | ? | — | ? | ? | ✅ |
| GPAMP | ⚠ | ⚠ | ? | ? | ? | — | ? | ✅ |
| Comparator | ✅ | ✅ | ✅ | ✅ | ? | ? | — | ✅ |
| UART0 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | — |

## 冲突解释与处理

| 组合 | 原因 | 处理方式 |
|---|---|---|
| 单 ADC + 双 ADC | P01 和 P02 都占 ADC0、PA25、TIMG0、DMA_CH0、Event1 | 不同时启用两个采集 profile；需要双通道时直接选择 P02/P06，并把单通道算法接到 channel A |
| DAC DMA + Timer Capture | P03 默认 DAC=TIMG6，P05 默认 Capture=TIMG6 | P06 已验证重分配方案：DAC 保留 TIMG6，Capture 改 TIMG7 |
| DAC DMA + OPA DAC bias | 两者可能同时要求唯一 DAC0；OPA bias 还可能要求内部 DAC route，与 PA15 外部 DAC_OUT 语义不同 | 明确比赛模式二选一，或重新设计偏置来源；不能仅靠改函数名复用同一个 DAC0 |
| ADC + GPAMP | 官方 GPAMP→ADC 示例需要把 GPAMP 输出接到 ADC 通道；若单独 ADC 流同时占 ADC0，需配置 MEM sequence/另一个 ADC | 先在专用 SysConfig profile 中选择合法内部/外部 route，再决定共享 ADC0 或改 ADC1 |
| Dual ADC + GPAMP | 两个 ADC 实例都已占用，增加 GPAMP 输出采集通常需要同 ADC 的额外 MEM 或改变采集结构 | 不在 P06 上盲加；按赛题删掉不用的 ADC 通道或建立 MEM sequence |
| OPA/GPAMP 与其他模拟模块 | 当前没有冻结 pin/route，不能仅凭模块可编译推断硬件可共存 | 在 CCS SysConfig 中加入真实实例和 pin 后，以 error-free 生成结果更新矩阵 |

## 不是硬件冲突但容易误判的接口问题

- `adc_dual_sync` 仅拆数据，不会自动启动 P02 的两个 DMA。
- `dac_dma` 仅包装注入回调，不会自动操作 P03 的 DMA_CH1。
- `timer_capture` 只处理时间戳，不会安装 P05 的 TIMG6 ISR。
- BSP ADC 与 ADC_DMA 都使用 `SignalADC_` 前缀。两者同时包含可能产生类型/API 认知冲突，应由应用 adapter 隔离。
- UART CSV 输出很慢，不能在高采样率 DMA ISR 内逐点打印；它不占硬件采样资源，但会占 CPU 时间和串口带宽。

## 通过 profile 证明的组合

- P04：ADC0 + DAC0 + 两个 Timer + 两个 DMA + UART。
- P06：双 ADC + DAC0 + Comparator + Timer Capture + 三个 Full DMA + UART。
- 证明范围是 SysConfig 生成、`-Wall -Werror` 编译和链接；未烧板的组合仍保持 BUILD_VERIFIED。
