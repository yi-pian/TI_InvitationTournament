# TI Arm Clang Target Build/Link Smoke

运行 `build_target_matrix.ps1` 会在本目录的 `build/` 内生成 SysConfig 输出、`.out`、`.map` 和日志。脚本只读取本机 SDK，不写 SDK 示例目录，也不写系统集成应用。

它验证：

- Reference C、CMSIS Q15/Q31/F32 的 N=512/1024/2048/4096 编译和最终链接；
- IQMath RTS 和 IQMath MATHACL 两个库配置能否通过 TI Arm Clang 最终链接；
- 公开 float FFT API 的真实静态 buffer 是否超过 32 KB SRAM。

该测试没有下载到开发板，因此只能标记 TARGET_BUILD_VERIFIED，不能标记 BOARD_RUNTIME_VERIFIED。

若 SysConfig 因受限环境不能写 TI 用户缓存，可运行 `build_target_offline.ps1`。它只读使用 SDK 官方 startup、linker cmd 和预编译库，不执行 SysConfig；因此其状态要写成“离线 TARGET BUILD/LINK VERIFIED”，不能冒充“本轮 SysConfig generate verified”。
