# FFT memory budget for 32 KB SRAM

MSPM0G3507 SRAM 共 32 KB。当前 `signal_fft` 是原地 single-precision complex FFT，每点 8 bytes；单边 float 幅度谱是 `N/2+1`点；ADC raw 每点 2 bytes。`SignalSpectrumAnalyzer_Analyze` 现场计算 Hann 系数，所以不需要独立 window buffer。

## 保守完整链：raw + float 时域 + FFT + magnitude

下表为缓冲未复用的最清晰组合，并为栈预留 1024 bytes。这个栈值是设计预算，不是所有数学库路径的实测最大栈；最终仍要用 `.map` 和栈水位检查。

| N | ADC raw `2N` | 处理 float `4N` | complex FFT `8N` | magnitude `2N+4` | window | stack 预留 | 合计 | 结论 |
|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 512 | 1,024 | 2,048 | 4,096 | 1,028 | 0 | 1,024 | 9,220 B | 安全 |
| 1024 | 2,048 | 4,096 | 8,192 | 2,052 | 0 | 1,024 | 17,412 B | 推荐默认 |
| 2048 | 4,096 | 8,192 | 16,384 | 4,100 | 0 | 1,024 | 33,796 B | 已超 32 KB |
| 4096 | 8,192 | 16,384 | 32,768 | 8,196 | 0 | 1,024 | 66,564 B | 不可用 |

若改成预生成 float Hann 系数表，还要另加 `4N`：512/1024/2048/4096 分别增加 2/4/8/16 KB。当前应用组合不保留该表。

## 复用缓冲的两种方案

| N | 采集后释放 raw，保留 float+FFT+magnitude+1KB stack `14N+1028` | 未来直接 raw->complex，FFT+magnitude+1KB stack `10N+1028` |
|---:|---:|---:|
| 512 | 8,196 B | 6,148 B |
| 1024 | 15,364 B | 11,268 B |
| 2048 | 29,700 B | 21,508 B |
| 4096 | 58,372 B | 41,988 B |

- 当前 `SignalSpectrumAnalyzer_Analyze` 需要 float 输入与 complex 输出同时有效；不要在未证明重叠安全时把两者指向同一区。
- N=2048 只能在释放/复用 raw、减少其他 BSS，且 `.map` 证明栈与 SDK 状态仍有空间时使用。
- N=4096 的 complex 数组单独已占 32 KB；即使不要 raw、magnitude 和窗表也无栈空间，当前 float 实现禁止上板组合。

## 当前允许方案

- N=512：快速刷新、低 RAM。
- N=1024：比赛默认 FFT，分辨率与内存平衡最好。
- N=2048：条件可用，必须有应用级 `.map` 和栈验证。
- N=4096：ADC 块采集本身已实板支持，但当前 float FFT 不支持板上组合。未来可评估 Q15/CMSIS-DSP、分段算法或外部 RAM。

bin 宽度 `Δf = Fs/N`。增加 N 只缩小 bin 间隔，不会自动解决时钟误差、非相干泄漏、低 SNR 或峰值偏差。
