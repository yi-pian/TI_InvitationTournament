# 工具链环境基线

本文记录本仓库在 2026-08-07 实际使用并验证过的环境。它是工程基线，不是对任意版本的兼容承诺。

| 项目 | 实际值 | 本机位置/证据 |
|---|---|---|
| 开发板 | LP-MSPM0G3507 | MSPM0G3507，LQFP-64(PM) |
| Code Composer Studio | 21.0.0 | `D:\TI\CCS`，安装包目录 `CCS_21.0.0.00014_win` |
| TI Arm Clang | **5.1.1.LTS** | `D:\TI\CCS\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS` |
| 编译目标 | `arm-ti-none-eabi` | `tiarmclang --version` |
| MSPM0 SDK | 2.11.00.07 | `C:\TI\mspm0_sdk_2_11_00_07` |
| SysConfig | 1.28.0+4696 | `D:\TI\CCS\ccs\utils\sysconfig_1.28.0` |
| RTOS | NoRTOS | 所有当前 profile 均为 `--rtos nortos` |

先前文档中出现的 TI Arm Clang 4.0.2.LTS 不是当前实机环境，不能再作为验收依据。SDK 元数据声明最低 SysConfig 为 1.26.0；本仓库实际生成和验证使用 CCS 自带 1.28.0，不使用本机另装的 1.26.2 作为发布基线。

## 已执行验证

- `adc_dma_onboard_selftest` 已用其 CCS 实际生成的 Makefile执行 Clean + Rebuild。
- 实际编译命令为 TI Arm Clang 5.1.1.LTS，并带 `-Wall -Werror`。
- 编译命令中的模块搜索路径为真实物理目录：

```text
-I.../MSPM0_Signal_Contest/02_acquisition/adc_dma
-I.../MSPM0_Signal_Contest/01_bsp/common
```

- 六套 `09_examples/integration_profiles` 已由 SysConfig 1.28.0 生成，并完成 Cortex-M0+ 目标端编译和链接。
- `09_examples` 内全部 3 个 `.projectspec` 已通过静态路径检查：linked file 存在、物理 include path 正确、没有把虚拟 `modules/*` 当作 include path。

可重复运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_peripheral_profiles.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\validate_projectspec_paths.ps1
```

## CCS 重新导入规则

`.projectspec` 中 `targetDirectory="modules/..."` 只创建 Project Explorer 虚拟文件夹，不是磁盘上的 include 路径。正式头文件必须通过 `compilerBuildOptions` 中的真实相对路径搜索，源码通过 linked file 指向唯一 source of truth。

现有 ADC_DMA 三个 Demo 的正确形式是：

```text
-I${PROJECT_ROOT}/../../../../02_acquisition/adc_dma
-I${PROJECT_ROOT}/../../../../01_bsp/common
```

修改 `.projectspec` 后应删除 CCS 中旧的工程引用（不要删除磁盘内容），再从 `.projectspec` 重新导入；随后打开 `.syscfg` 生成、Clean Project、Rebuild Project，并以 Console 中的实际 `tiarmclang` 命令为最终证据。

## 工具提示与错误的区分

六套 profile 当前没有 SysConfig error、编译 warning 或链接 warning。SysConfig 会输出以下 `info`，它们必须保留在风险记录中：

- ADC 自动掉电模式可能需要把唤醒时间计入采样窗口；当前 profile 明确使用 manual power-down，但工具仍给出通用提示。
- TIMG6/TIMG7 在 STOP/STANDBY 不保留寄存器；应用若进入这些低功耗模式，必须保存并恢复配置，或重新调用生成的初始化函数。
- DMA_CH0..2 被工具标记为 Full Channel，这是预期资源分配。

## 不属于本基线的结论

SysConfig 生成成功和目标端链接成功不等于实板功能通过。除 ADC_DMA 板载 TMP6131 自测外，其余 profile 当前均为 BUILD_VERIFIED，必须在后续硬件阶段单独升级状态。
