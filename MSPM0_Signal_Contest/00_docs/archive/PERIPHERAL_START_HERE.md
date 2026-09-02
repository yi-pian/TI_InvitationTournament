# 外设库 10 分钟上手

## 1. 确认工具版本

看 `TOOLCHAIN_ENVIRONMENT.md`。当前基线是 CCS 21.0.0、TI Arm Clang 5.1.1.LTS、SDK 2.11.00.07、SysConfig 1.28.0。

## 2. 先跑不烧板检查

在仓库根目录执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_peripheral_library.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_peripheral_profiles.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_existing_adc_demos.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\check_peripheral_api_freeze.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_projectspec_paths.ps1
```

前 3 个脚本调用本机 TI 工具链；最后一个只检查 public header 是否被无记录修改。

## 3. 按题目选择 profile

| 需求 | 从这里开始 |
|---|---|
| 单 ADC | PROFILE_01_ADC_CAPTURE |
| 双 ADC | PROFILE_02_DUAL_ADC |
| DAC 输出 | PROFILE_03_DAC_GENERATOR |
| ADC + DAC | PROFILE_04_ADC_DAC |
| 边沿/频率捕获 | PROFILE_05_FREQUENCY |
| 多链路组合 | PROFILE_06_FULL_SIGNAL |

资源细节看 `HARDWARE_RESOURCE_MAP.md`，叠加前看 `PERIPHERAL_CONFLICT_MATRIX.md`。

## 4. 复制应用模板

复制 `08_applications/peripheral_system_template` 为新的比赛 application。先在 `signal_hw_config.h` 选 profile、N、Fs、Vref 和 feature；再把选中 profile 的 `profile.syscfg` 复制到 CCS application 工程。

模板只有四段职责：

```text
hardware config -> peripheral init/start -> algorithm hook -> output/debug
```

算法 hook 是空接口，不包含正式算法实现。

## 5. 在 CCS 中加入唯一源文件

正式模块只有一个 source of truth。通过 linked file 或工程外 source path 引入实际 `.c`，include search path 指向真实模块目录，例如：

```text
${PROJECT_ROOT}/../../../../02_acquisition/adc_dma
${PROJECT_ROOT}/../../../../01_bsp/common
```

Project Explorer 中的 `modules/adc_dma` 只是虚拟目录。不要复制头文件/源码到 application，不要在 `main.c` 写 `../../../../signal_adc_dma.h`。

## 6. 生成、Clean、Rebuild

1. 打开 `.syscfg` 并保存/Generate；
2. Project → Clean；
3. Project → Rebuild；
4. Console 确认 TI Arm Clang 5.1.1、`-Wall -Werror` 和真实 `-I` 路径；
5. SysConfig info 与 warning 分开记录。

## 7. 接算法

只把 `signal_u16_frame_t`、双 frame、capture timestamps 或 DAC wave table 交给算法任务。接口规则见 `HARDWARE_ALGORITHM_CONTRACT.md`。

## 8. 状态不要越级

- build/link 通过：BUILD_VERIFIED。
- 烧板并按清单通过：BOARD_VERIFIED。
- 赛题输入范围、速率、幅度、连续运行都通过：才考虑 CONTEST_VERIFIED。

目前只有 ADC_DMA 板载 TMP6131 自测有 BOARD 证据，其余 profile 不得写成硬件已通过。

## 情况 1：采一个模拟信号

- Profile：P01 ADC_CAPTURE。
- Include：`signal_adc_dma.h`、`signal_status.h`；搜索路径指向 `02_acquisition/adc_dma` 与 `01_bsp/common`。
- 配置：`sample_rate_hz`、`timer_clock_hz`、`timer_max_count`；N 由 `SignalADC_Start(buffer,N)` 传入；ADC channel/pin 在 `.syscfg` 改。
- 顺序：`SYSCFG_DL_init → SignalADC_Init → SignalADC_Start → WFE/IsFinished → GetBuffer/GetSampleCount`。

## 情况 2：同时采两个信号

- Profile：P02 DUAL_ADC。
- 硬件：ADC0/PA25/DMA0 与 ADC1/PA17/DMA1，共用 TIMG0、Event1/2。
- 当前 `adc_dual_sync` 没有 `Init/Start/GetChannelA/B` 硬件 API；必须由薄 application adapter 装两个 DMA并汇合完成状态。
- 算法收到两个 count/rate 相同的 `signal_u16_frame_t`，不看到 ADC instance。

## 情况 3：产生波形

- Profile：P03 DAC_GENERATOR。
- 模块：`dac_wave_table` + 所需 shape/DDS + `dac_dma` callback wrapper + 硬件 adapter。
- 顺序：静态波表生成 → 配置 DMA source/count → 使能 DAC FIFO → 启动 TIMG6。
- 输出率在 Timer/DAC adapter 中集中配置，不写进波形数学函数。

## 情况 4：测方波频率

- Profile：P05 FREQUENCY。
- 硬件：PA27→COMP0→Event4→TIMG6 Capture。
- 模块：比较器 frontend 构造阈值，应用 ISR 存 timestamps，`timer_capture` 求 delta/mean period。
- 当前没有完整捕获 ISR adapter，属于需要人工接入的 BUILD 级路径。

## 情况 5：检测过零

- Profile：仍从 P05 开始。
- 使用 `comparator_zero_cross` 根据虚地电压构造配置，再经 BSP comparator adapter 应用。
- 过零 Event 可进入 Timer Capture 或仅通知应用；阈值、迟滞、滤波必须按实际噪声实板调整。
- 不要用 ADC 数组的 `trigger_capture` 冒充硬件 Comparator 过零路径。
