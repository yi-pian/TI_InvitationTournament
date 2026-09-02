# CMSIS-DSP Contest Smoke Test

这个工程只回答一个问题：当前 MSPM0 SDK、SysConfig 和 TI Arm Clang 工程能否直接编译并链接 CMSIS-DSP。

覆盖四项：基础向量加法、F32 RMS、F32 Min/Max、256 点 Q15 CFFT + 复数幅值。工程使用与 TI 官方 `cmsis_dsp_empty` 相同的关键配置：`ARM_MATH_CM0`、CMSIS Core/DSP include，以及 `ProjectConfig.genLibCMSIS = true`。

Build 通过只表示 `BUILD_VERIFIED`。没有运行开发板时，`g_smoke_complete` 和数值结果均不得写成 `BOARD_VERIFIED`。
