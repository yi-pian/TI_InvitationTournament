# Algorithm Backend Benchmark

本目录只验证算法后端，不连接 ADC、DMA 或任何比赛应用。

运行：

```text
gmake clean
gmake test
```

PC 数值测试把四种 FFT 后端与独立 double 精度 radix-2 参考进行比较，覆盖 N=512/1024/2048/4096，以及 clean sine、noisy sine、harmonic sine、two tone、DC offset sine、clipped sine。另有 Hann 后已知 0.5 V peak 的幅值恢复和数据 Adapter 测试。

PC 只能验证数值，不能给出 MSPM0G3507 的真实周期数。周期栏必须保持 `PENDING_BOARD`，直到开发板计时完成。
