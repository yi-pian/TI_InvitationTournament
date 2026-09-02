# 比赛常用功能代码手册

本手册收录“模块已经提供接口，但比赛 `main.c` 仍经常需要少量组合逻辑”的功能。
目标是现场可以按下面顺序搬运：

1. 先复制模块目录中列出的 `.c/.h/.inc`；
2. 按模块 README 配置 SysConfig，并重新 Generate；
3. 从模块的 `README_MINIMAL_EXAMPLE.c` 复制初始化和正常调用顺序；
4. 再复制本手册对应的应用层小段代码；
5. 最后把示例中的数据变量替换成题目已有变量。

本文件的代码只属于应用层组合逻辑，不修改冻结的模块 `.c/.h`，也不修改
`Debug/ti_msp_dl_config.c/.h`。示例中的 `g_tft`、`g_result` 等名字需要替换为你的
工程变量；模块 API 名称和调用时机保持不变。

## 1. 现有 README 盘点

| 比赛功能 | 现有模块 README 状态 | 本手册补充内容 |
|---|---|---|
| 动态采样率 | `adc_dual_sync` README 第 20 节已有 `SetSampleRate/GetConfiguredRate` 和现场闭环 | 频率到 Fs 的最小闭环、限幅、帧边界和实际 Fs 传递 |
| 动态 Y 轴/自动量程 | `tft_waveform` README 第 19 节已有 fixed/auto scale、`MapY` 和现场闭环 | 带余量的量程计算、时间轴范围、轴标签刷新策略 |
| TFT 局部刷新 | `tft_ili9341` 已有 dirty-region 原则和 24_C 案例 | 可直接套用的 dirty 标志模板 |
| TFT 屏幕分页 | ILI9341 有 API/原则；ST7789 README 第 9 节现有完整图形分页状态机 | 页面枚举、翻页键、静态/动态绘制闭环 |
| 矩阵键盘扫描 | `matrix_keypad_4x4` 已有扫描、消抖、鬼键过滤、`ReadNewSymbol` | 第 20 节同步提供数字预输入、退格、确认、取消、溢出保护 |
| 参数安全生效 | 分散在各题目文档中 | 编辑态/提交态/取消态三态模板 |
| 采集超时与错误 | 各采集 README 有状态码说明 | 有限等待、超时退出、UI 显示错误而不死循环 |

## 2. 动态采样率：根据输入频率选择 Fs

### 2.1 模块中已经提供的代码

从 `02_acquisition/adc_dual_sync/README_MINIMAL_EXAMPLE.c` 复制双路 ADC 的
`Init -> Start -> IsFinished -> GetChannelA/B` 流程。从模块 README 复制并保留：

```c
SignalDualADC_SetSampleRate(target_sample_rate_hz);
actual_sample_rate_hz = SignalDualADC_GetConfiguredRate();
```

`SetSampleRate()` 只能在初始化后、上一帧完成或尚未启动下一帧时调用；返回成功后，
算法必须使用 `GetConfiguredRate()` 得到的整数化 Fs，不能继续使用目标值猜测。

### 2.2 `main.c` 只需补的应用逻辑

```c
#define APP_POINTS_PER_CYCLE       (20U)       /* 每周期至少保留 20 点。 */
#define APP_MIN_SAMPLE_RATE_HZ     (20000U)    /* 低于此值时不利于边沿和显示。 */
#define APP_MAX_SAMPLE_RATE_HZ     (200000U)   /* 按当前 Timer/ADC 能力设上限。 */

static uint32_t g_sample_rate_hz = 100000U;    /* 后续算法实际使用的 Fs。 */

static void App_UpdateSampleRate(uint32_t measured_frequency_hz)
{
    uint32_t target_sample_rate_hz;             /* 频率换算后的目标 Fs。 */
    uint32_t actual_sample_rate_hz;             /* Timer 整数分频后的 Fs。 */

    if (measured_frequency_hz == 0U) return;     /* 频率无效时保持上一有效 Fs。 */
    if (measured_frequency_hz >
        APP_MAX_SAMPLE_RATE_HZ / APP_POINTS_PER_CYCLE) return;
                                                   /* 乘法前先防止超过上限。 */

    target_sample_rate_hz = measured_frequency_hz * APP_POINTS_PER_CYCLE;
                                                   /* 让每周期约有 20 个采样点。 */
    if (target_sample_rate_hz < APP_MIN_SAMPLE_RATE_HZ) {
        target_sample_rate_hz = APP_MIN_SAMPLE_RATE_HZ;
    }
                                                   /* 太低时提升到测量下限。 */
    if (target_sample_rate_hz > APP_MAX_SAMPLE_RATE_HZ) {
        target_sample_rate_hz = APP_MAX_SAMPLE_RATE_HZ;
    }
                                                   /* 保护 Timer 和 DMA 的上限。 */

    if (SignalDualADC_SetSampleRate(target_sample_rate_hz) !=
        SIGNAL_RESULT_OK) return;                 /* 忙或越界时不覆盖旧配置。 */

    actual_sample_rate_hz = SignalDualADC_GetConfiguredRate();
                                                   /* 读取真实整数化配置值。 */
    if (actual_sample_rate_hz != 0U) {
        g_sample_rate_hz = actual_sample_rate_hz;
    }
                                                   /* 时间轴、FFT、相位统一使用它。 */
}
```

### 2.3 调用位置

在 `SignalDualADC_IsFinished()` 为真、已经处理完上一帧、准备下一次
`SignalDualADC_Start()` 之前调用 `App_UpdateSampleRate()`。不要在 DMA/ADC ISR 中调用，
也不要在 `MODULE_RUNNING` 状态中调用。

### 2.4 这一段中哪些是自写

- `SignalDualADC_SetSampleRate()` 和 `GetConfiguredRate()`：模块 README 已提供，直接复制。
- `App_UpdateSampleRate()`、三个宏和 `g_sample_rate_hz`：应用层自写；负责比例、限幅、
  无效频率保持旧值和把实际 Fs 交给后续算法。
- `APP_POINTS_PER_CYCLE` 不是固定真理。方波边沿、FFT 分辨率或题目窗口有要求时，按题目
  重新选择，并在步骤文档中写明理由。

## 3. 动态坐标轴与自动量程

### 3.1 模块中已经提供的代码

从 `01_bsp/tft_waveform/README_MINIMAL_EXAMPLE.c` 复制
`signal_tft_waveform_config_t` 和 `SignalTFTWaveform_Draw()`。需要固定示波器量程时使用
`SIGNAL_TFT_WAVEFORM_FIXED_SCALE`；只想看清未知幅值时使用
`SIGNAL_TFT_WAVEFORM_AUTO_SCALE`。

### 3.2 应用层带余量 Y 轴

```c
static bool App_SelectDisplayScale(const float *samples,
                                   size_t count,
                                   float *minimum_value,
                                   float *maximum_value)
{
    size_t i;                                     /* 当前扫描下标。 */
    float minimum;                                /* 原始数据最小值。 */
    float maximum;                                /* 原始数据最大值。 */
    float span;                                   /* 原始数据峰峰范围。 */
    float margin;                                 /* 给边界留出的显示余量。 */

    if ((samples == NULL) || (count == 0U) ||
        (minimum_value == NULL) || (maximum_value == NULL)) {
        return false;                             /* 参数不完整时不改输出。 */
    }

    minimum = samples[0];                         /* 用第一个有效点初始化。 */
    maximum = samples[0];
    for (i = 1U; i < count; ++i) {                /* 扫描剩余样本。 */
        if (samples[i] < minimum) minimum = samples[i];
        if (samples[i] > maximum) maximum = samples[i];
    }

    span = maximum - minimum;                     /* 计算原始峰峰值。 */
    if (span < 0.1F) {                            /* 平坦信号避免除零/挤成一条线。 */
        minimum -= 0.05F;
        maximum += 0.05F;
    } else {
        margin = span * 0.1F;                     /* 上下各留 10% 空白。 */
        minimum -= margin;
        maximum += margin;
    }

    *minimum_value = minimum;                     /* 输出给 waveform config。 */
    *maximum_value = maximum;
    return true;                                  /* 表示量程有效。 */
}
```

### 3.3 应用层 X 轴时间范围

```c
static float App_GetWindowMilliseconds(size_t display_count,
                                       uint32_t actual_sample_rate_hz)
{
    if ((display_count == 0U) || (actual_sample_rate_hz == 0U)) {
        return 0.0F;                              /* 无点数或无 Fs 时不计算。 */
    }
    return 1000.0F * (float)display_count /
        (float)actual_sample_rate_hz;             /* ms = N / Fs × 1000。 */
}
```

轴的 min/max 只用于显示映射，不能用于反推 Vpp、相位或频率；测量仍使用原始数组。
轴线、单位和刻度属于静态布局，建议只在量程真的变化时重画，避免每帧闪烁。

### 3.4 这一段中哪些是自写

- `SignalTFTWaveform_Draw()`、fixed/auto scale 枚举：模块 README 已提供。
- `App_SelectDisplayScale()` 与 `App_GetWindowMilliseconds()`：应用层自写，分别计算
  显示 Y 轴和 X 轴时间范围。
- 轴标签的 `DrawString/DrawFloat` 调用：应用层自写；标签刷新应使用 dirty 标志，不能
  把坐标轴绘制塞进 ADC ISR。

## 4. TFT 屏幕分页与局部刷新

### 4.1 页面状态机

```c
typedef enum {
    APP_PAGE_WAVEFORM = 0,                        /* 第 0 页：时域波形。 */
    APP_PAGE_SPECTRUM,                            /* 第 1 页：频谱。 */
    APP_PAGE_SETTINGS,                            /* 第 2 页：参数。 */
    APP_PAGE_COUNT                                /* 页面总数，不能当页面使用。 */
} app_page_t;

static app_page_t g_page = APP_PAGE_WAVEFORM;     /* 当前页面。 */
static bool g_page_dirty = true;                  /* true 表示需要重画静态布局。 */

static void App_PageNext(void)
{
    g_page = (app_page_t)(((uint32_t)g_page + 1U) %
        (uint32_t)APP_PAGE_COUNT);                /* 到末页后回到第 0 页。 */
    g_page_dirty = true;                          /* 页面变了，下一轮重画一次。 */
}

static void App_PagePrevious(void)
{
    if (g_page == APP_PAGE_WAVEFORM) {
        g_page = (app_page_t)(APP_PAGE_COUNT - 1U); /* 第 0 页向前回到末页。 */
    } else {
        g_page = (app_page_t)((uint32_t)g_page - 1U); /* 普通向前翻页。 */
    }
    g_page_dirty = true;                          /* 请求一次静态重绘。 */
}

static void App_HandlePageKey(char symbol)
{
    if (symbol == 'A') App_PagePrevious();        /* A：上一页。 */
    if (symbol == 'D') App_PageNext();            /* D：下一页。 */
}
```

### 4.2 主循环绘制顺序

```c
char symbol;                                      /* 键盘输出的稳定字符。 */

if (SignalMatrixKeypad4x4_ReadNewSymbol(&symbol) ==
    SIGNAL_RESULT_OK) {
    App_HandlePageKey(symbol);                    /* 只改变状态，不在这里画屏。 */
}

if (g_page_dirty) {
    TFT_ILI9341_FillScreen(&g_tft, TFT_ILI9341_BLACK);
    App_DrawPageStatic(g_page);                   /* 自己画标题、轴、单位和标签。 */
    g_page_dirty = false;                         /* 静态页面已经完成。 */
}

App_RenderPageDynamic(g_page);                    /* 只刷新当前页动态区域。 */
```

`App_DrawPageStatic()` 和 `App_RenderPageDynamic()` 是应用层绘图函数：前者只在翻页或
模式改变时调用，后者使用 ILI9341 的 `FillRect` 清除固定字段后再调用 `DrawInt32`、
`DrawFloat`、`DrawLine` 或 waveform helper。键盘模块只负责输出字符，不能在扫描回调或
中断里调用 TFT。

### 4.3 这一段中哪些是自写

- `ReadNewSymbol()`：矩阵键盘模块的固定引脚便利接口，按模块 README 复制。
- `app_page_t`、`g_page`、`g_page_dirty`、翻页函数和主循环分支：应用层自写。
- `FillScreen`、`FillRect`、`DrawString` 等：ILI9341 模块 README 已有 API，按静态/动态
  区域分别复制；不要每一帧调用 `FillScreen`。

## 5. 矩阵键盘数字预输入：退格、确认、取消

这个模板适合输入频率、采样率、幅值阈值等整数参数。数字先写入缓冲区，按 `#` 后才
改变真实配置；因此输入过程中可以按 `*` 退格、按 `D` 取消，不会把半截数字送进 ADC
或 DDS。

```c
#define APP_INPUT_CAPACITY   (8U)                 /* 最多 7 个字符，最后一格放 '\0'。 */
#define APP_INPUT_MAX_VALUE  (200000U)            /* 题目允许的最大整数。 */

static char g_input_buffer[APP_INPUT_CAPACITY];   /* 屏幕和解析共同使用的文本缓冲。 */
static size_t g_input_length;                     /* 当前已经输入的字符数。 */
static uint32_t g_input_old_value;                /* 取消时恢复的旧值。 */
static uint32_t g_target_value = 100000U;         /* 只有确认后才更新的真实参数。 */
static bool g_input_active;                       /* true 表示正在编辑。 */

static bool App_ParseUint32(const char *text,
                            size_t length,
                            uint32_t *value)
{
    size_t i;                                     /* 当前字符下标。 */
    uint32_t result = 0U;                         /* 逐位累积的数值。 */

    if ((text == NULL) || (value == NULL) || (length == 0U)) {
        return false;                             /* 空输入或空指针无效。 */
    }
    for (i = 0U; i < length; ++i) {
        uint32_t digit;                           /* 当前字符对应的数字。 */
        if ((text[i] < '0') || (text[i] > '9')) return false;
        digit = (uint32_t)(text[i] - '0');
        if (result > (APP_INPUT_MAX_VALUE - digit) / 10U) return false;
        result = result * 10U + digit;            /* 乘 10 后加当前位。 */
    }
    *value = result;                               /* 输出解析结果。 */
    return true;                                  /* 输入格式和范围均有效。 */
}

static void App_InputBegin(void)
{
    g_input_length = 0U;                          /* 清空旧的预输入字符。 */
    g_input_buffer[0] = '\0';                      /* 保证它是 C 字符串。 */
    g_input_old_value = g_target_value;           /* 保存取消时要恢复的值。 */
    g_input_active = true;                         /* 进入编辑态。 */
}

static void App_InputAppendDigit(char symbol)
{
    if ((symbol < '0') || (symbol > '9')) return;  /* 只接受数字键。 */
    if (!g_input_active) App_InputBegin();        /* 第一个数字自动开始输入。 */
    if (g_input_length + 1U >= APP_INPUT_CAPACITY) return;
                                                    /* 预留末尾 '\0'，防止越界。 */
    g_input_buffer[g_input_length++] = symbol;    /* 追加一位字符。 */
    g_input_buffer[g_input_length] = '\0';        /* 每次追加后保持字符串有效。 */
}

static void App_InputBackspace(void)
{
    if (!g_input_active || (g_input_length == 0U)) return;
    --g_input_length;                              /* 删除最后一位。 */
    g_input_buffer[g_input_length] = '\0';        /* 截断字符串。 */
}

static void App_InputCommit(void)
{
    uint32_t value;                                /* 本次确认得到的新值。 */
    if (!g_input_active) return;                   /* 没有编辑内容时不提交。 */
    if (App_ParseUint32(g_input_buffer, g_input_length, &value)) {
        g_target_value = value;                   /* 只有解析成功才写真实参数。 */
        g_input_active = false;                   /* 退出编辑态。 */
    }
}

static void App_InputCancel(void)
{
    if (!g_input_active) return;                    /* 非编辑态按取消不改变当前值。 */
    g_target_value = g_input_old_value;            /* 恢复进入编辑前的值。 */
    g_input_length = 0U;
    g_input_buffer[0] = '\0';
    g_input_active = false;                        /* 退出编辑态且不改变配置。 */
}

static void App_HandleInputKey(char symbol)
{
    if ((symbol >= '0') && (symbol <= '9')) App_InputAppendDigit(symbol);
    if (symbol == '*') App_InputBackspace();       /* *：退格。 */
    if (symbol == '#') App_InputCommit();          /* #：确认生效。 */
    if (symbol == 'D') App_InputCancel();          /* D：取消并恢复旧值。 */
}
```

代码中每一条 `if` 都是边界保护：不接受非数字、不允许缓冲区越界、不允许数值溢出，
并且只在 `App_InputCommit()` 成功后改写 `g_target_value`。显示预输入时直接把
`g_input_buffer` 交给 `TFT_ILI9341_DrawString()`；先用 `FillRect` 清固定宽度，再画新
字符串，避免从 `100000` 改成 `9` 后残留旧字符。

## 6. 采集超时与错误状态

下面是主循环的有限等待模板。`g_system_ms` 由已有的 1 ms SysTick 递增；中断只递增
计数，不做 TFT、FFT 或浮点计算。

```c
static volatile uint32_t g_system_ms;              /* 1 ms 系统节拍。 */

static bool App_WaitForDualADC(uint32_t timeout_ms)
{
    uint32_t start_ms = g_system_ms;               /* 记录等待起点。 */
    while (!SignalDualADC_IsFinished()) {          /* 等待双 DMA 同时完成。 */
        if ((g_system_ms - start_ms) >= timeout_ms) {
            SignalDualADC_Stop();                 /* 超时立即停止硬件。 */
            return false;                         /* 告诉主循环本帧无效。 */
        }
        __WFI();                                  /* 空闲等待中断，降低 CPU 占用。 */
    }
    return true;                                  /* 两路都完成且未超时。 */
}
```

超时后应在主循环显示 `ADC TIMEOUT` 或进入错误页，并等待用户按键重新开始；不要用
无限 `while (!IsFinished())` 把键盘和 UI 永久锁死。

## 7. 比赛现场统一检查清单

- **动态 Fs：** 只在帧边界调用 `SetSampleRate()`；后续时间轴、FFT bin、相位计算都用
  `GetConfiguredRate()`。
- **动态坐标轴：** 自动量程只改变显示映射，不能改变测量结果或被当成固定 V/div。
- **分页：** 翻页键只改页面状态；翻页时整页清屏一次，页面内部只刷新 dirty 区域。
- **预输入：** 数字先进入缓冲区；`*` 退格、`#` 确认、`D` 取消；确认前不改硬件参数。
- **中断：** ISR 只置标志、递增计数或发布 buffer；TFT、FFT、浮点和大循环全部放在
  主循环或低频任务。
- **边界：** 所有数组检查 `NULL/count/capacity`，所有除法检查分母，所有等待设置超时。
- **文档：** 步骤文档中分别写“模块复制代码”“SysConfig 生成内容”“应用层自写代码”，
  并解释每个全局变量和每个状态转换的作用。
