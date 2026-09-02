# System Recipe Selection

先按题目“必须输出什么”选主 Recipe，再根据波形、噪声和延迟切换后端。不要把所有算法同时打开。

```text
需要测什么？
├─ DC / 偏置 ─────────────── Recipe 01
├─ Vpp
│  ├─ 必须保留真实尖峰 ───── Recipe 02 普通链
│  └─ 已确认只有孤立毛刺 ─── Recipe 02/14 鲁棒链
├─ RMS
│  ├─ 含 DC 的总有效值 ───── Recipe 03 Total RMS
│  └─ 只要交流分量 ───────── Recipe 03 AC RMS
├─ Frequency
│  ├─ 方波/脉冲、干净边沿、低延迟 ─ Recipe 05 Comparator Capture
│  ├─ 正弦、高 SNR、高精度 ───────── Recipe 04 ZeroCross Interpolation
│  ├─ 噪声大/波形失真/多音 ───────── Recipe 06 FFT
│  └─ 非正弦周期且主频谱不稳定 ───── Recipe 15 Autocorrelation
├─ Spectrum / peaks ──────── Recipe 07
├─ Harmonic / THD ────────── Recipe 08
├─ Dual-channel phase ────── Recipe 09
├─ Generate waveform ─────── Recipe 10
├─ Filter/amplifier response  Recipe 11
├─ Burst / trigger capture ── Recipe 12
├─ Capture and replay ─────── Recipe 13
└─ Weak signal ────────────── Recipe 15
```

## 频率方法决策

| 条件 | 首选 | 不首选的原因 |
|---|---|---|
| 方波、脉冲、比较器阈值可靠 | Method A / Recipe 05 | ADC 块处理延迟更大 |
| 正弦、高 SNR、需亚采样插值 | Method B / Recipe 04 | FFT 分辨率受 `Fs/N` 约束 |
| 噪声较大、波形失真、频率未知 | Method C / Recipe 06 | ZeroCross 容易多触发 |
| 失真周期信号、基波不一定是最大谱线 | Autocorrelation | 简单峰值可能选错谐波 |
| 极低延迟 | Capture/Time-domain | 必须接受对边沿和阈值的依赖 |

## 点数与 Backend 快选

| 需求 | N | FFT Backend | 说明 |
|---|---:|---|---|
| 低延迟、双通道相位 | 512 | CMSIS Q31 | 已完成 full link，余量充足 |
| 普通单通道频谱/THD | 1024 | CMSIS Q31 | 默认比赛配置 |
| 1024 双通道相位 | 1024 | CMSIS Q31 | 可链接但 SRAM 仅余 3,040 B，谨慎 |
| 2048 Simple FFT | — | — | 完整 Frequency C 已因 `.bss` 超 32 KiB 链接失败 |
| 4096 Simple FFT | — | — | `UNSUPPORTED_BY_CURRENT_SIMPLE_FFT_API` |

## 开始动作

1. 在 `SYSTEM_RECIPES.md` 打开对应 Recipe。
2. 从 Recipe 的“对应应用”复制应用或复制 `signal_contest_template`。
3. 只改集中配置；涉及 pin/ADC/DMA/Timer/Event 时再改 SysConfig。
4. 运行 SysConfig generate、TI Arm Clang full compile、final link，并读 `.map`。
5. PC Truth 不替代开发板与仪器验证；板上状态保持 `PENDING_BOARD`，直到真实完成。
