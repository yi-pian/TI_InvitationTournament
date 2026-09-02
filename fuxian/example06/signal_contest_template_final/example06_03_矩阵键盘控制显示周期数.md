# example06-03：矩阵键盘控制显示周期数

## 比赛动作

先按 `01_bsp/matrix_keypad_4x4/README.md` 配置一个 `GPIO_KEYPAD`：R1..R4 为 PB16、
PB0、PB7、PB17 输出且初值高；C1..C4 为 PB18、PB13、PB20、PB4 上拉输入。复制
`signal_matrix_keypad_4x4.c/.h`，不改模块文件。

README 的固定 MSPM0G3507 便利 API 是 `SignalMatrixKeypad4x4_ReadNewSymbol()`，因此
没有在 main 重写行列扫描、消抖或鬼键过滤。

## main 复制与自写内容

- `SysTick_Handler()` 使用与 `22_X` 相同的方法：SysTick 每 1 ms 进入，累计到 5 ms 时
  调用一次 `SignalMatrixKeypad4x4_ReadNewSymbol()`。
- 只有数字字符 `'1'`～`'5'` 才改变 `g_display_periods`；其他键忽略。这是本题唯一的
  键盘应用逻辑。
- 键盘扫描位于中断，主循环不再等待显示完成才扫描；中断只更新周期数，不访问 TFT、FFT
  或 ADC 数据。

## 逐行解释

202-212：每 5 ms 调用模块完成固定引脚扫描；只有新的稳定按键才返回字符。
189-194：检查字符范围并把 ASCII 数字转成 1～5 的显示周期数。波形窗口按该值计算采样跨度。
