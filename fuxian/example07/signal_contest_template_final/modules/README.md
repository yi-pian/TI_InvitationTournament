# modules

把模块 README 列出的 `.c/.h/.inc` 和少量公共头文件复制到本目录。

本母版 Include Path 已包含 `${PROJECT_ROOT}/modules`。复制后：

1. 在 CCS Project Explorer 右键工程并 Refresh；
2. 确认新 `.c` 文件没有 Exclude from Build；
3. 在 `main.c` include 模块主头文件；
4. 立即 Build 一次。

不要把正式仓库的 Platform/Adapter 整目录搬进来；只复制当前模块 README 明列的文件。
## example07 未知信号通道自动补偿组合补充

### 模块选择顺序

1. `signal_dual_adc_mspm0g3507.*`：同步采集未知网络输入、输出。
2. `signal_dac_dma_mspm0g3507.*`、`signal_dac_wave_table.*`、`signal_sine.*`：产生测试/预补偿波形。
3. `signal_frequency_sweep.*`：生成 0.5 kHz～10 kHz 扫频点。
4. `signal_lock_in.*`：逐频点测量输入/输出幅值和相位。
5. `signal_frequency_response_correction.*`：对测量结果插值并生成逆响应。
6. `signal_tft_st7789.*`、`signal_tft_st7789_font.*`、`signal_tft_st7789_mspm0g3507.*`：ST7789 和 8×16 字库显示。
7. `signal_matrix_keypad_4x4.*`：A/D 翻页、1 扫频、2 1 kHz 补偿、3 谐波补偿。
8. `signal_dds.*`：把 1024 点正弦/谐波查表重采样为 100 点 DAC DMA 周期缓冲，保证循环播放时波表边界相位连续。

### SysConfig 补全

本题在原空母版上新增 `DAC12`、第二个 `TIMER` 和两路 ADC/DMA。DAC12 打开 Analog Output、Amplifier、FIFO、DMA done，FIFO trigger=`HWTRIG0`，DMA=`SIGNAL_DAC_DMA/DMA_CH1`，输出脚按器件合法 PinMux 选择 `PA15`；`SIGNAL_DAC_TIMER` 使用 `TIMG6`、2 us 周期、Event publisher channel 3。双 ADC 使用 `TIMG0`、2 us、ADC A DMA_CH0、ADC B DMA_CH2。上述字段与 DAC DMA、Dual ADC README 的硬件链路一致；差异只有本题把 ADC B 从示例占用的 DMA_CH1 改为 DMA_CH2，以避开 DAC DMA_CH1，并把扫频范围固定为 0.5 kHz～10 kHz。

### main 粘贴边界

`App_Capture`、`SignalLockIn_Process`、`SignalFrequencySweep_Generate`、`SignalSine_Generate`、`SignalDACDMA_MSPM0_SetUpdateRate/Start/Stop`、`SignalFrequencyResponseCorrection_Process`、ST7789 绘图和键盘扫描均按 README 的调用形状复制。自己编写的只有：输入/输出数组换算、增益/相位相减、逆响应表填充、按键状态机、三谐波有限长度叠加和显示分页。

### DMA 完成中断补充（与 22_X 保持一致）

双 ADC 模块靠 `DMA_IRQHandler()` 在 ADC A、B 的 DMA 块都完成后，将采集状态改为完成。集成库 `MSPM0_Signal_Contest/02_acquisition/adc_dual_sync` 已按 22_X 的做法，在 `SignalDualADC_Init()` 内开启 DMA_CH0 和 DMA_CH2 的完成中断位；example07 已重新从该集成库版本同步：

```c
DL_DMA_enableInterrupt(DMA, SIGNAL_DUAL_ADC_A_DMA_MASK |
    SIGNAL_DUAL_ADC_B_DMA_MASK);
```

这是模块自身必须完成的中断初始化，`main.c` 不再需要额外补丁，也不改变 SysConfig 的 DMA 通道分配。若误从旧 example05 副本复制、缺少它时，按 `1` 会停在第一个 500 Hz 扫频点，按 `3` 会停在第一个 1 kHz 谐波测量点。除集成库已有的这一处修复外，其他模块 `.c/.h/.inc` 没有改动。

### DDS 调用补充（原模块 README 未覆盖的组合场景）

除已记录的双 ADC DMA 中断缺陷修复外，模块文件不改动；本组合 README 补齐调用步骤：先用 `SignalDACDMA_MSPM0_SetUpdateRate(round(frequency_hz * 100))` 把 DAC 更新率设为目标频率的 100 倍；读取 `SignalDACDMA_MSPM0_GetConfiguredRate()` 后，以 `实际更新率/100` 为实际输出频率。再用 `SignalSine_Generate(&wave, 0.5f, amplitude, phase_cycles)` 生成 1024 点查表，调用 `SignalDDS_Init(&dds, lut, 1024, actual_frequency_hz, actual_update_rate_hz, 0)`，最后用 `SignalDDS_Fill(&dds, dac, 100)` 生成一个完整周期的 DAC DMA 缓冲。随后按 DAC DMA README 调用 `SignalDACDMA_MSPM0_Start(dac, 100, true)`。扫频测量采用返回的实际输出频率作为 Lock-In 参考；1/2/3 次谐波则先在 `lut` 合成一个基波周期，再按相同的 100 点周期规则播放。片内 DAC 最高工作到 1 MSPS；因此 10 kHz 是本配置的最高测试频点，不能再提高周期点数。
