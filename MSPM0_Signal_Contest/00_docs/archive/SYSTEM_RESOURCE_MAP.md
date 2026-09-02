# System Resource Map

更新时间：2026-08-08。MCU 为 MSPM0G3507，构建按 128 KiB Flash、32 KiB SRAM 计算。资源来自真实 SysConfig 与 TI Arm Clang `.map`，并通过 `tiarmsize` 交叉校验。Stack 统一保留 512 B；运行时最大栈水位仍为 `PENDING_BOARD`。

## Hardware Profiles

| Profile | ADC / Pins | DMA | Timer / Event | DAC | Comparator | UART | OPA/GPAMP |
|---|---|---|---|---|---|---|---|
| P01 ADC Capture | ADC0 PA25 | DMA0 | TIMG0 触发 | — | — | UART0 | 未用 |
| P02 Dual ADC | ADC0 PA25 + ADC1 PA17 | DMA0 + DMA1 | TIMG0 共触发 | — | — | UART0 | 未用 |
| P03 DAC Generator | — | DMA1 | TIMG6 + DAC Event | DAC0 PA15 | — | UART0 | 未用 |
| P04 ADC + DAC | ADC0 PA25 | DMA0 + DMA1 | TIMG0 ADC，TIMG6 DAC/Event | DAC0 PA15 | — | UART0 | 未用 |
| P05 Frequency Capture | — | — | TIMG6 Capture，Event channel 4 | — | COMP0 PA27 | UART0 | 未用 |
| P06 Full Signal | ADC0 PA25 + ADC1 PA17 | DMA0 ADC-A、DMA2 ADC-B、DMA1 DAC | TIMG0 ADC、TIMG6 DAC、TIMG7 Capture、Event 4 | DAC0 PA15 | COMP0 PA27 | UART0 PA10/PA11 | 未用 |

P06 是母模板的“资源超集”，不是要求所有外设同时运行。若真题只需单 ADC，应复制模板后裁剪为 P01，避免无意义占用。

## Application 资源占用

| Application | Profile | Hardware resource | Flash used / remain | SRAM used / remain | 主要静态 Buffer |
|---|---|---|---:|---:|---|
| Signal Meter | P01 | ADC0, DMA0, TIMG0, UART0 | 7,656 / 123,416 B | 14,926 / 17,842 B | events 6,156; voltage 4,096; positions 2,052; raw 2,048 B |
| Frequency A | P05 | COMP0, Event4, TIMG6 Capture, UART0 | 1,944 / 129,128 B | 757 / 32,011 B | timestamps 小数组 |
| Frequency B | P01 | ADC0, DMA0, TIMG0, UART0 | 6,264 / 124,808 B | 14,896 / 17,872 B | events 6,156; voltage 4,096; positions 2,052; raw 2,048 B |
| Frequency C Q31 | P01 | ADC0, DMA0, TIMG0, UART0 | 89,368 / 41,704 B | 16,936 / 15,832 B | FFT 8,192; voltage 4,096; magnitude 2,052; raw 2,048 B |
| Spectrum Q31 | P01 | ADC0, DMA0, TIMG0, UART0 | 89,320 / 41,752 B | 17,045 / 15,723 B | 同上 |
| THD Q31 | P01 | ADC0, DMA0, TIMG0, UART0 | 90,776 / 40,296 B | 16,961 / 15,807 B | 同上 |
| Phase Q31, N=512 | P02 | dual ADC/DMA, TIMG0, UART0 | 89,168 / 41,904 B | 15,392 / 17,376 B | FFT A/B 各 4,096; voltage A/B 各 2,048; raw A/B 各 1,024 B |
| DDS Generator | P03 | DAC0, DMA1, TIMG6/Event, UART0 | 10,560 / 120,512 B | 3,244 / 29,524 B | DMA block 2,000; table 512 B |
| Sweep Analyzer | P04 | ADC0/DMA0/TIMG0 + DAC0/DMA1/TIMG6 | 18,352 / 112,720 B | 9,687 / 23,081 B | voltage 4,096; raw 2,048; DDS block 2,000; table 512 B |
| Wave Capture Replay | P04 | 同上 | 7,456 / 123,616 B | 18,173 / 14,595 B | ring 4,098; period 4,096; ordered 4,096; DMA capture 4,096; replay 1,024 B |
| Signal Analyzer Spectrum | P02 | dual ADC/DMA, TIMG0, UART0 | 91,032 / 40,040 B | 9,999 / 22,769 B | FFT A 4,096; voltage A 2,048; magnitude 1,028; raw A/B 各 1,024 B |
| Contest Template Basic | P06 | Full Signal 资源超集 | 8,880 / 122,192 B | 9,505 / 23,263 B | events 3,084; voltage 2,048; positions 1,028; raw A/B 各 1,024 B |

## Multi-Profile 最坏资源

| Target / Profile | Flash | SRAM | SRAM remain | 结论 |
|---|---:|---:|---:|---|
| Signal Analyzer Basic | 7,872 | 8,987 | 23,781 | PASS |
| Signal Analyzer Frequency | 7,872 | 8,987 | 23,781 | PASS |
| Signal Analyzer Spectrum | 91,032 | 9,999 | 22,769 | PASS |
| Signal Analyzer THD | 91,048 | 9,999 | 22,769 | PASS |
| Signal Analyzer Phase | 89,200 | 15,631 | 17,137 | PASS |
| Contest Template Basic | 8,880 | 9,505 | 23,263 | PASS |
| Contest Template Spectrum | 90,584 | 10,517 | 22,251 | PASS |
| Contest Template THD | 92,056 | 10,517 | 22,251 | PASS |
| Contest Template Phase | 90,176 | 16,149 | 16,619 | PASS |

## 冲突与限制

- P04/P06 固定 DMA0=ADC-A、DMA1=DAC；P06 使用 DMA2=ADC-B。更换 channel 必须改 SysConfig 与 Adapter 宏。
- P05/P06 Comparator Capture 使用 Event channel 4；不能与另一个同 channel producer 随意合并。
- P02 的 ADC 双通道同步与 P06 的全资源并存均只完成 build/link，真实 skew/触发时序为 `PENDING_BOARD`。
- N=1024 Phase 可链接但 SRAM 余量仅 3,040 B；默认采用 N=512。
- N=2048 Simple FFT 完整应用已经链接失败；4096 不作为当前目标。
- OPA/GPAMP 当前最终应用未占用。真题启用后必须重新做 SysConfig conflict check 和 full link。
