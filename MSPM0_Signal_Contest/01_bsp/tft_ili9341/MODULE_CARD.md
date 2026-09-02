# MODULE CARD: TFT ILI9341

| 项目 | 内容 |
|---|---|
| 目录 | `01_bsp/tft_ili9341` |
| 层级 | BSP / 外部显示设备 |
| 作用 | 通过回调式 SPI/GPIO 驱动 240×320 ILI9341，提供基础图形、RGB565、4 套 ASCII 字体和自定义单色字模输出 |
| 输入/输出 | 坐标、RGB565、像素数组、ASCII/数值/1-bit 字模 → SPI 命令/数据；返回 `signal_result_t` |
| 主头文件 | `signal_tft_ili9341.h` |
| 依赖 | `signal_status.h`；应用提供 SPI/GPIO/delay 回调 |
| SysConfig | 需要：SPI mode 0、8-bit、MSB first，以及 CS/DC/RESET/BL GPIO（按接线选配） |
| RAM | 动态分配 0；无全屏 framebuffer；字库位于 Flash；绘制函数栈缓冲 128 B |
| 状态 | `MODULE_STATUS_BUILD_VERIFIED` |
| 构建证据 | 4 套×95 个 ASCII 及“电”“子”字模源数据检查 PASS；PC mock PASS；TI Arm Clang `-Wall -Werror` PASS；44 模块聚合链接 PASS；含文字 TFT 示例 SysConfig/compile/full link PASS（Flash 21,016 B，SRAM 597 B） |
| 硬件声明 | 本轮未观察实屏显示，状态不高于 BUILD_VERIFIED |
| 唯一源码 | 应用 projectspec 链接本目录正式 `.c/.h` 和内部字模 `.inc`；示例不保存第二份驱动或字库 |

24_C 用法：静态坐标轴、单位、网格和标签只绘制一次；主循环仅清除/重画波形区域和固定宽度数字字段，ISR 不调用 TFT/SPI。
