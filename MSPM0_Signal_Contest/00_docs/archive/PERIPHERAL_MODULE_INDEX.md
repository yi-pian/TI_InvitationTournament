# 外设模块总索引

范围固定为 `01_bsp`、`02_acquisition`、`06_generator`、`07_signal_frontend`。空目录 `adc_single`、`sweep`、`opa_pga` 不是模块，也不计数。当前共有 **40 个正式外设模块**：39 个 BUILD_VERIFIED，1 个 BOARD_VERIFIED。

状态含义：BUILD 表示严格编译通过；BOARD 表示已有明确实板证据；两者都不自动等于比赛全场景验证。

## 01_bsp（11）

| 模块/路径 | 作用与主要 API | 依赖 | 固定硬件资源 | Demo / SysConfig | 状态 |
|---|---|---|---|---|---|
| adc / `01_bsp/adc` | 校验 ADC 配置、回调读取 raw；`ValidateConfig/ReadRaw` | common | 无，平台回调注入 | 无 / 无 | BUILD |
| comparator / `01_bsp/comparator` | 校验并应用比较器配置；`ValidateConfig/Apply` | common | 无，平台回调注入 | P05/P06 可作配置基础 | BUILD |
| dac / `01_bsp/dac` | 电压转码、raw 写 DAC；`VoltageToRaw/WriteRaw` | common | 无，写回调注入 | P03/P04/P06 可作配置基础 | BUILD |
| dma / `01_bsp/dma` | 校验、启动、停止抽象 DMA 传输 | common | 无，平台回调注入 | profiles 分配 DMA，但未绑定此抽象 | BUILD |
| gpamp / `01_bsp/gpamp` | 校验并应用 GPAMP 配置 | common | 无，平台回调注入 | 无 / 无 | BUILD |
| gpio / `01_bsp/gpio` | `Write/Read/Toggle` | common | 无，端口回调注入 | 无 / 无 | BUILD |
| opa / `01_bsp/opa` | OPA 增益计算和配置应用 | common | 无，平台回调注入 | 无 / 无 | BUILD |
| system_clock / `01_bsp/system_clock` | 时钟合法性和 Timer period 计算 | common | 无 | profiles 使用 32 MHz 默认树 | BUILD |
| timer / `01_bsp/timer` | 设率、启停、读计数 | common | 无，平台回调注入 | profiles 分配 Timer，但未绑定此抽象 | BUILD |
| uart / `01_bsp/uart` | 二进制、字符串写与读 | common | 无，平台回调注入 | P01..P06：UART0 PA10/PA11 | BUILD |
| vref / `01_bsp/vref` | 计算有效参考电压 | common | 无 | 无 / 无 | BUILD |

## 02_acquisition（9）

| 模块/路径 | 作用与主要 API | 依赖 | 固定硬件资源 | Demo / SysConfig | 状态 |
|---|---|---|---|---|---|
| adc_basic | 通过 BSP ADC 连续读取一个 block | bsp/adc, common | 无 | 无 / 无 | BUILD |
| adc_continuous | 连续采集状态机和 frame 提交 | common、采集回调 | 无 | 无 / 无 | BUILD |
| adc_dma | Timer→Event→ADC0→DMA→RAM；`Init/SetSampleRate/Start/Stop` | DriverLib、生成的 `ti_msp_dl_config.h`、common | 当前实现宏约定 ADC0/DMA0/TIMG0 | 3 个 Demo；P01/P04 兼容 | **BOARD** |
| adc_dual_sync | 对交织数据做双通道拆分 | common | 无；不负责启动两个 ADC | P02/P06 仅提供硬件配置 | BUILD |
| adc_pingpong_dma | 双缓冲所有权和完成切换 | common | 无；DMA ISR 由应用接入 | 无 / 无 | BUILD |
| adc_ring_buffer | 固定内存环形缓冲 | common | 无 | 无 / 无 | BUILD |
| adc_timer_trigger | 用注入回调协调 Timer 触发 ADC | common | 无 | 无 / 无 | BUILD |
| timer_capture | 对时间戳求差和平均周期 | common | 无；不含捕获 ISR | P05/P06 仅提供硬件配置 | BUILD |
| trigger_capture | 在 ADC 数组中找阈值触发点并截帧 | common | 无 | 无 / 无 | BUILD |

## 06_generator（11）

| 模块/路径 | 作用与主要 API | 依赖 | 固定硬件资源 | Demo / SysConfig | 状态 |
|---|---|---|---|---|---|
| am_modulation | 对已有 carrier/message 数组做 AM | common | 无 | 无 / 无 | BUILD |
| arbitrary_wave | 任意表线性重采样 | common | 无 | 无 / 无 | BUILD |
| dac_dc | 以电压设置 DAC DC | bsp/dac | 无，DAC 写回调注入 | P03/P04/P06 可作硬件配置 | BUILD |
| dac_dma | DMA 输出状态包装器 | common、start/stop 回调 | 无；不是 TI DMA 驱动实现 | P03/P04/P06 仅提供硬件配置 | BUILD |
| dac_wave_table | 波表描述、校验、归一化转 raw | common | 无 | 无 / 无 | BUILD |
| dds | 相位累加、改频、取样/填充 | common | 无 | 无 / 无 | BUILD |
| frequency_sweep | 生成线性/对数频率序列 | common | 无 | 无 / 无 | BUILD |
| sawtooth | 生成锯齿 raw 波表 | dac_wave_table | 无 | 无 / 无 | BUILD |
| sine | 生成正弦 raw 波表 | dac_wave_table | 无 | 无 / 无 | BUILD |
| square | 生成方波 raw 波表 | dac_wave_table | 无 | 无 / 无 | BUILD |
| triangle | 生成三角 raw 波表 | dac_wave_table | 无 | 无 / 无 | BUILD |

## 07_signal_frontend（9）

| 模块/路径 | 作用与主要 API | 依赖 | 固定硬件资源 | Demo / SysConfig | 状态 |
|---|---|---|---|---|---|
| comparator_threshold | 构造指定阈值的比较器配置 | bsp/comparator | 无；不直接写 COMP 寄存器 | P05/P06 可作配置基础 | BUILD |
| comparator_zero_cross | 构造虚地过零比较配置 | bsp/comparator | 无 | P05/P06 可作配置基础 | BUILD |
| gpamp_buffer | 构造 GPAMP buffer 配置 | bsp/gpamp | 无 | 无 / 无 | BUILD |
| gpamp_gain | 构造带增益 GPAMP 配置 | bsp/gpamp | 无 | 无 / 无 | BUILD |
| opa_buffer | 构造 OPA voltage follower 配置 | bsp/opa | 无 | 无 / 无 | BUILD |
| opa_dac_bias | 计算 OPA + DAC 偏置所需 DAC 电压 | common | 无 | 无 / 无 | BUILD |
| opa_inverting | 构造反相 OPA 配置 | bsp/opa | 无 | 无 / 无 | BUILD |
| opa_noninverting_pga | 构造非反相 PGA 配置 | bsp/opa | 无 | 无 / 无 | BUILD |
| opa_to_adc | 检查 OPA 输出是否落入 ADC 输入范围 | common | 无 | 无 / 无 | BUILD |

## 状态边界

- `adc_dma` 的 BOARD 证据只覆盖 LP-MSPM0G3507 板载 TMP6131、N=256..4096、每个 N 100 帧；PA25 动态模拟输入仍未验证。
- P01..P06 已完成 SysConfig/compile/link，不代表对应抽象 BSP 已有 DriverLib adapter。
- `adc_dual_sync`、`dac_dma`、`timer_capture` 的名称描述数据流职责，不应误读为已经包含完整硬件启动代码。
- 任何模块升级为 BOARD 或 CONTEST 状态都必须追加可复现证据，不允许只改枚举返回值。
