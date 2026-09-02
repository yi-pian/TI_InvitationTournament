# 本机 SDK 与官方例程盘点

盘点日期：2026-08-07。

## 已发现工具

| 工具 | 路径 | 版本/状态 |
|---|---|---|
| MSPM0 SDK | `C:\ti\mspm0_sdk_2_11_00_07` | 2.11.00.07 |
| SysConfig（CCS 当前实际调用） | `D:\TI\CCS\ccs\utils\sysconfig_1.28.0` | 1.28.0；由 Clean/Rebuild 命令行确认 |
| SysConfig（旧独立安装） | `C:\ti\sysconfig_1.26.2` | 1.26.2+4477；不是当前 CCS 构建调用版本 |
| TI Arm Clang（CCS 当前实际编译器） | `D:\ti\CCS\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS` | 5.1.1.LTS |
| TI Arm Clang（旧独立安装） | `C:\ti\ti_cgt_arm_llvm_4.0.2.LTS` | 4.0.2.LTS；不是当前 CCS 编译器 |
| CCS | `D:\ti\CCS` | 21.0.0，当前工程生成记录确认使用 TI Arm Clang 5.1.1.LTS |

当前工作区已建立 SysConfig/CCS 验收工程、正式模块和 PC 回归测试。LP-MSPM0G3507 手册、原理图和本机 SDK 例程用于核对板级连接、Event、ADC 和 DMA 资源。

## 与第一模块直接相关的官方例程

| 官方例程 | 取证内容 |
|---|---|
| `driverlib/adc12_triggered_by_timer_event` | Timer 零事件发布、ADC Event subscriber、PA25/ADC0.2 |
| `driverlib/adc12_14bit_resolution` | MEM0 result address、16 位 DMA、每帧重新使能 ADC DMA |
| `driverlib/adc12_max_freq_dma` | ADC DMA done 中断、1024 点块采集、PB25/ADC0.4 高速链 |
| `msp_subsystems/adc_simultaneous_sample` | Timer + Event + ADC + DMA 的完整硬件链和重复采集流程 |
| `msp_subsystems/adc_dma_ping_pong` | 后续连续实时采集的参考，不在本阶段实现 |

本模块使用的 DriverLib API 均已在 SDK 2.11.00.07 头文件或上述例程中确认，包括：

```text
DL_TimerG_setLoadValue / setTimerCount / startCounter / stopCounter
DL_ADC12_getMemResultAddress / enableDMA / disableDMA
DL_ADC12_enableConversions / disableConversions
DL_DMA_setSrcAddr / setDestAddr / setTransferSize
DL_DMA_enableChannel / disableChannel
```

## 板级选择

首个 Demo 使用 PA25/ADC0.2（J1.2），原因是可直接从 BoosterPack 接口输入，接线简单。LP-MSPM0G3507 的高速模拟链是 P1/底部输入 -> OPA2365 -> PB25/ADC0.4；该链需要确认 J13 模拟供电和板上连接，留给高速采集验收阶段。

## 当前验证结论

- `adc_dma_onboard_selftest.syscfg`：CCS 捆绑 SysConfig 1.28.0 生成通过；Timer/Event/ADC/DMA 路由使用 SDK 生成宏，未手改生成文件。
- 当前 CCS 实际编译器是 TI Arm Clang 5.1.1.LTS；4.0.2.LTS 仅为旧独立安装，不再作为当前环境结论。
- ADC_DMA 已在 LP-MSPM0G3507 上用板载 TMP6131 完成 LEVEL 1：N=256/512/1024/2048/4096，每种 100 帧，共 500/500 帧；WFE 唤醒 500 次，哨兵残留 0，FCC PASS，末帧 min/max/mean=2073/2076/2074。
- 因此只有 `02_acquisition/adc_dma` 可标记 `MODULE_STATUS_BOARD_VERIFIED`；PA25 动态模拟输入和 100/200/500 kSPS 动态响应仍待验证。
- 纯算法与通用适配层以 PC 回归、TI Arm Clang 5.1.1.LTS `-Wall -Werror` 和链接检查为构建证据；不等于外设已经上板。
