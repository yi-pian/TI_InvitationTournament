# 主程序自写组合逻辑逐行说明

本文件只解释 `signal_contest_template_final/main.c` 中没有直接从模块 README 原样复制、而是为了把多个模块串成第四题仪器而补写的少量逻辑。正式模块的算法和硬件驱动仍由各自 `.c/.h` 完成。

## 1. 数字预输入：`App_BeginInput`、`App_AppendInput`、`App_CommitInput`

```c
static void App_BeginInput(app_edit_target_t target)
{
    g_edit_target = target;
    g_input_length = 0U;
    g_input[0] = '\0';
}
```

逐行解释：

1. `target` 表示本次要修改的是频率、幅度、偏置、触发电平还是预触发点数。
2. `g_edit_target = target` 进入输入状态；主循环随后把数字键当作参数，而不是页面快捷键。
3. `g_input_length = 0U` 清空本次输入的字符数。
4. `g_input[0] = '\0'` 把字符数组置为空字符串，保证后续解析从头开始。

```c
static void App_AppendInput(char symbol)
{
    if ((symbol < '0') || (symbol > '9') ||
        (g_edit_target == APP_EDIT_NONE) ||
        (g_input_length + 1U >= APP_INPUT_CAPACITY)) return;
    g_input[g_input_length++] = symbol;
    g_input[g_input_length] = '\0';
}
```

1. 第一行条件拒绝非数字、未进入输入状态以及会超出缓冲区的按键。
2. `g_input[g_input_length++] = symbol` 把新数字放到末尾，再让长度加一。
3. 下一行补字符串结束符，使 `g_input` 能传给解析函数。

```c
static void App_CommitInput(void)
{
    uint32_t value;
    if ((g_edit_target == APP_EDIT_NONE) ||
        !App_ParseUint(g_input, g_input_length, &value)) return;
    /* 按 target 分支并做题目范围检查 */
    g_edit_target = APP_EDIT_NONE;
}
```

1. `value` 保存字符串转换后的无符号整数。
2. 未在输入或字符串非法时直接返回，不改变旧参数。
3. `App_ParseUint` 逐字符检查并防止整数溢出。
4. 分支只在频率 200～20000 Hz、幅度 10～1500 mV、偏置不超过 3000 mV、触发码不超过 4095、预触发点小于帧长时写入全局变量。
5. 提交后清除编辑状态；频率/幅度/偏置改变时置 `g_wave_dirty`，让主循环重新生成波表。

## 2. 键盘到功能的映射：`App_ProcessKey`

1. `g_edit_target != APP_EDIT_NONE` 时，数字键进入预输入，`#` 提交，`D` 取消；这段优先级保证输入数字不会误触发页面快捷键。
2. `1~4` 写入 `g_waveform`，分别代表正弦、方波、三角波、锯齿波，并立即请求重新启动 DDS/DAC DMA。
3. `5~7` 写入 `g_filter`，选择 RAW、Median、Hampel；`8/9/0/#` 写入 `g_window`，选择 Rectangular、Hann、Hamming、Blackman。
4. `A` 把 `g_page` 加一并对 7 个页面取模，然后只重画静态标题和蓝色边框。
5. 普通页面的 `B/C/*` 分别启动频率、幅度、偏置输入；CAPTURE 页不再临时修改捕获参数，避免等待时误操作。
6. CAPTURE 页的 `D` 调用组合模块 `Arm`，`1/2/3` 调用 `SelectSlot` 后回放，`#` 调用 `ReplaySelected`；CAL 页的 `D` 调用 `App_Calibrate`。

## 3. 单次捕获组合模块

旧版 main 中的 `g_capture_valid`、DMA 块序号、触发待处理索引、基线裁剪、三个槽位状态和安全绘图已经全部移入 `signal_single_capture_replay`。main 只保存一个 `g_single_capture` 模块对象和模块配置使用的缓冲区。

`App_ClearCaptureTrigger()` 在武装前清除 COMP/NVIC 旧标志；`App_ConsumeCaptureTrigger()` 在主循环读取硬件兜底标志；`GROUP1_IRQHandler()` 调用 `SignalSingleCaptureReplay_NotifyTrigger()`。这三个函数必须留在 main，因为 `SIGNAL_COMP_INST` 与 IRQ 宏由本工程 SysConfig 生成。

`App_ArmCapture()`、`App_ServiceCapture()` 和 `App_ReplayCapture()` 都是一行模块 API 包装，不再包含捕获算法。页面用 `GetSelected()` 判断槽位是否有效，用 `DrawSelectedST7789()` 显示；因此 main 不访问模块内部的长度、采样率、有效标志和绘图坐标。

## 4. 页面刷新

1. `App_DrawStaticPage` 只在上电和按 `A` 翻页时执行 `FillScreen`、边框和标题绘制。
2. `App_DrawDynamicPage` 先清除边框内部的动态区域，再根据 `g_page` 显示测量值、频谱指标、鲁棒指标或捕获曲线，并在底部显示实际采样率和 DAC 输出频率。
3. `SysTick_Handler` 设置 250 ms 显示节拍；main 只有在 `g_display_due` 置位时才重画动态区域，避免 ST7789 被每个 ADC 帧连续刷屏。
4. 页面切换、参数确认和功能操作会主动置位 `g_display_due`，因此用户操作不必等待完整的显示周期。
5. `App_DrawValue` 把浮点结果转换成毫伏或整数显示；它只负责显示格式，不参与任何测量计算。

## 5. 主循环逐行说明

```c
while (1) {
    App_ProcessQueuedKeys();
    if (g_wave_dirty) {
        if (App_ApplyWaveform() == SIGNAL_RESULT_OK) g_wave_dirty = false;
    }
    g_sample_rate_hz = App_SelectSampleRate(g_output_frequency_hz);
    if (App_AcquireFrame() == SIGNAL_RESULT_OK) {
        /* ADC code -> voltage -> calibration -> all analysis modules */
    }
    if (g_measurement_valid && g_display_due) {
        g_display_due = false;
        g_display_elapsed_ms = 0U;
        App_DrawDynamicPage();
    }
    __WFI();
}
```

1. `SysTick_Handler` 每 5 ms 调用键盘模块的一键接口；main 每轮从环形队列取出稳定的新按键并进入应用状态机。
2. 波形参数改变后只重建一次 DDS/DAC 缓冲，避免每轮重复初始化输出。
3. `App_SelectSampleRate` 按输出频率在 40 kSPS/100 kSPS 间选择采样档位。
4. `App_AcquireFrame` 启动双 ADC DMA，并用 `__WFI` 等待硬件完成。
5. 成帧后依次调用 ADC To Voltage、校准、基本测量、三路测频、FFT、鲁棒测量、Sine Fit 和 Lock-In；公式不在 main 重写。
6. `g_measurement_valid` 防止首帧完成前显示未初始化结果；`g_display_due` 将显示频率限制为约 4 Hz。
7. `__WFI` 在下一次 DMA、SysTick 或其他中断前让 CPU 休眠，减少无意义的忙等。

## 6. 与 README 的边界

- 直接复制：模块的 include、配置结构体、`Init/Start/Process/GetResult` 调用和错误返回。
- 自己编写：页面枚举、键盘映射、数字预输入、参数范围检查、模块调用顺序、结果汇总、COMP SysConfig 回调和屏幕局部刷新。捕获/裁剪/槽位/绘图/复现算法已经整理进正式组合模块。
- 正式模块 `.c/.h` 没有改动；界面使用新增的 `signal_tft_st7789_font.c/.h`。它把 22_X 的 ILI9341 ASCII 字模交给 ST7789 的 `DrawMonoBitmap` 输出，不改变原屏幕驱动。
# 本次新增的按键与局部刷新组合逻辑

`g_key_queue[]` 是 8 字节按键环形队列，`g_key_head` 指向写入位置，`g_key_tail` 指向待读位置。SysTick 中断只负责调用集成键盘模块的 `SignalMatrixKeypad4x4_ReadNewSymbol()`；读到稳定字符后调用 `App_QueueKey()` 入队，避免在中断里执行页面切换、ADC 算法或 SPI 绘图。

`App_QueueKey()` 先计算下一个队列位置；若下一个位置等于尾指针，表示队列已满，直接丢弃本次按键，防止覆盖尚未处理的按键。否则把字符写入当前头位置，再移动头指针。

`App_ProcessQueuedKeys()` 在主循环中比较头尾指针；不相等时取出一个字符、移动尾指针，并调用原有 `App_ProcessKey()`。因此按键处理仍在主循环完成，和 22_X 的“定时扫描、主循环处理”结构一致。

`SysTick_Handler()` 每 1 ms 进入一次。`scan_milliseconds` 累计到 5 后扫描一次键盘；`g_display_elapsed_ms` 累计到 250 后置 `g_display_due`，请求一次屏幕更新。中断只做计数、扫描和入队，不执行耗时算法。

`App_DrawValue()` 保留左侧标签，只用 `TFT_ST7789_FillRect(x+54,y,104,16,BLACK)` 清除数字旧内容，再用 `TFT_ST7789_FONT_8X16` 绘制新值，因此修改频率、幅度等参数时不会刷新整屏。`App_DrawStatus()` 对状态字符串使用同样的局部清除策略。

`main()` 初始化后调用 `SysTick_Config(CPUCLK_FREQ / 1000U)` 建立 1 ms 节拍；循环首先调用 `App_ProcessQueuedKeys()`，只有 `g_display_due` 为真时才调用 `App_DrawDynamicPage()`。页面切换仍调用 `App_DrawStaticPage()`，这是有意的整页重绘；同一页面的动态数值和曲线只做局部刷新。
