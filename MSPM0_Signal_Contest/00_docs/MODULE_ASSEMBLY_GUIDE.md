# Module Assembly Guide

> 可选工具书，不是总入口。先用 [CONTEST_IMPLEMENTATION_GUIDE.md](CONTEST_IMPLEMENTATION_GUIDE.md) 判断实现层级；本指南只帮助 B 复杂硬件模块与 C 算法模块的数据流拼接。

本指南假设复杂/算法模块已经选好，只讲怎样拼。固定 DAC、GPIO、单点 ADC 等 A Direct DriverLib 功能不要套用本文的 module/platform 骨架。

## 拼装方法摘要

1. **画数据流**：每个箭头标注 C 类型、长度、单位和 Fs。
2. **打开每个 README 与 `.h`**：记录 init/process/get-result API、配置结构、workspace 要求和是否可原地。
3. **查 Interface Matrix**：每个相邻模块必须是直接连接或插入明确 Adapter。
4. **确定 Buffer**：列出 raw、float、complex、magnitude、events、DMA table 的类型和元素数。
5. **确定初始化顺序**：复杂硬件才是 SysConfig init → platform/driver init → start；纯算法只准备 state/workspace/result。
6. **确定处理顺序**：严格按数据流调用，不在中间偷偷改单位或数组形状。
7. **加入正式源码**：projectspec/source manifest 引用原模块 `.c`，不要复制模块到应用目录。
8. **加入 include path 与 `.h`**：只包含应用真正使用的头文件；生成宏从 `ti_msp_dl_config.h` 读取。
9. **集中参数**：Fs/N/VREF/阈值/窗口/DDS 放 `signal_config.h`；功能裁剪可放 `signal_features.h`。
10. **Build 与 map**：SysConfig generate、full compile、final link；检查 `.out/.map`、大 Buffer 和余量。

## 初始化顺序模板

```c
SYSCFG_DL_init();                    /* pin/clock/DMA/Timer/Event source of truth */
SignalADC_Init(&adc_config);         /* acquisition/platform modules */
SignalDACPlatform_Init(rate, CPUCLK_FREQ); /* only when DAC is used */
/* Initialize stateful algorithms: DDS/FIR/IIR/etc. */

while (1) {
    SignalADC_Start(raw, SAMPLE_COUNT);
    while (!SignalADC_IsFinished()) { __WFI(); }
    /* Adapter and algorithm calls follow the drawn data flow. */
}
```

精确函数参数以模块头文件为准；不要根据此骨架猜函数名。

## 拼装示例链

### ADC raw capture

```text
Analog input → ADC_DMA → uint16_t raw[N] + count + configured Fs
```

初始化：`SYSCFG_DL_init` → `SignalADC_Init`。处理：`SignalADC_Start` → wait → `SignalADC_GetConfiguredTriggerRate`。

### ADC voltage measurement

```text
ADC_DMA → RawToVoltage Adapter → float voltage[N] → Measurement
```

Measurement 可以是 Mean、MinMax、VPP、RMS 或 ACRMS。VREF、bits、scale、offset 必须来自 `signal_config.h`，而不是散在循环里。

### FFT assembly

```text
ADC_DMA → RawToVoltage → RemoveDC → Window
        → FFT → Magnitude → optional GainCorrection/Peak
```

Buffer：raw `uint16_t[N]`、voltage `float[N]`、FFT `signal_complex_f32_t[N]`、magnitude `float[N/2+1]`。FFT Backend 由 projectspec/compile define 选择，应用只调用 `SignalFFT_*`。

### THD assembly

```text
ADC_DMA → RawToVoltage → RemoveDC → Window → FFT → Magnitude
        → Harmonic/MultiBinEnergy → THD
```

先确认最高谐波没有越过 Nyquist；Harmonic result 直接传给 THD，不在 `main.c` 重算公式。

### Dual-channel phase assembly

```text
DualADC Platform → raw A/B → two RawToVoltage calls → RemoveDC A/B
                 ├→ FFT A/B → Phase From FFT Bin
                 └→ Correlation → Phase From Correlation Lag
```

两路必须共用 N、configured Fs、window 和开始时刻。`PROFILE_02_DUAL_ADC` 是最小可参考 SysConfig。

### DDS output assembly

```text
WaveTable → DDS Init/SetFrequency/Fill → DAC DMA wrapper
          → DAC DMA Platform → Timer/Event/DMA/DAC
```

初始化 DAC platform 和 DDS 后再启动 DMA。改变频率时按模块 README 的 stop/set/fill/start 顺序执行。

### ADC + DAC assembly

```text
DDS/DAC DMA → external path → ADC DMA → selected algorithms → result
```

使用 `PROFILE_04_ADC_DAC`：ADC 用 DMA0/TIMG0/Event1，DAC 用 DMA1/TIMG6/Event3，不要让两条链复用同一资源。

### Trigger capture and replay assembly

```text
ADC DMA → Ring Buffer → Trigger Find → Segment
        → Arbitrary Resample/Normalize → DAC DMA
```

若需要保存 N 个 ring 元素，storage 分配 N+1。AutoRange replay 改变绝对 amplitude/offset。

## 在工程里引用模块

- `.c`：从正式模块原路径加入 projectspec/source list。
- `.h`：加入对应目录 include path，在应用源中 include 主头文件。
- 算法：只从 `../MSPM0_Signal_Contest/` 加入。
- SysConfig：复制最接近的 `09_examples/integration_profiles/PROFILE_0x_*/profile.syscfg` 作为资源参考，不复制生成文件。
- 禁止手工编辑 `ti_msp_dl_config.c/h`、`device_linker.cmd`、`.o/.out/.map`。

完整可编译连接参考位于 `08_applications/`；它们是 Reference Assembly Example，不是赛题方案。
