# MODULE CARD: TFT Waveform

| 项目 | 内容 |
|---|---|
| 目录 | `01_bsp/tft_waveform` |
| 层级 | BSP / TFT 显示辅助 |
| 作用 | 把 float 波形映射到 ILI9341 绘图区，提供普通抽点和每列 min-max envelope |
| 输入/输出 | 触发/时基已选好的 `float samples[N]` → TFT 折线或包络；返回实际范围和绘制列数 |
| 主头文件 | `signal_tft_waveform.h` |
| 依赖 | `signal_tft_ili9341.h/.c`、`signal_status.h` |
| SysConfig | 复用 TFT ILI9341 的 SPI/GPIO 配置；本辅助不新增外设 |
| RAM | 动态分配 0；O(1) 额外工作区，不建 framebuffer/列数组 |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 构建证据 | PC 映射、边界、窄脉冲包络、平坦自动量程及 mock draw PASS；TI Arm Clang 在真实 TFT SysConfig profile 下完整链接 PASS，细节见 README |
| 硬件声明 | 未观察真实屏幕，不能写 BOARD_VERIFIED |

24_C 用法：周期信号按硬件频率显示三周期，复合信号按 FFT 基波设 X 轴，猝发使用 marker 锁存的完整窗口并优先 min-max envelope；V/ms 轴标签由应用层绘制。
