# Module Card：MSPM0G3507 Platform Adapter

- 类型：Platform / Adapter
- 路径：`08_applications/common/mspm0g3507/`
- 作用：把正式通用 BSP callback 与 MSPM0G3507 DriverLib/SysConfig 连接起来，并集中承载 Capture ISR 与 TFT SPI glue。
- 输入：BSP descriptor、generated instance/pin/channel、V/Hz/tick/code 配置。
- 输出：真实外设调用、ADC raw、capture tick、UART/GPIO/DAC 动作。
- 依赖：TI MSPM0 SDK DriverLib、CMSIS Core、`01_bsp/common` 和具体 BSP 头文件。
- SysConfig：需要；按具体功能选择 PROFILE_07/01/03/05/06 或 TFT syscfg。
- 验证：十条最小工程完成 Documentation/API check + SysConfig + TI Arm Clang compile + final link；`BUILD_VERIFIED`，未 BOARD_VERIFIED。
- 已知限制：OPA/GPAMP 因公开 API 无法表达真实离散硬件配置，未提供假 adapter。
