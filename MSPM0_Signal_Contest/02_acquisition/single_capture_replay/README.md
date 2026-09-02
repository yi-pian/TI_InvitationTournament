# 单次任意波捕获、三槽存储、ST7789显示与DAC周期复现

## 1. 模块作用

这个模块整理的是已经在 `fuxian/example04` 实板验证成功的完整闭环：

```text
ADC0连续多块DMA + COMP边沿通知
-> 相邻两块拼接
-> 在ADC历史中定位门限边沿
-> 保留触发前后固定窗口
-> 检查尾部是否已经回到稳定基线
-> 裁掉首尾无效直线
-> 保存到3个槽位
-> ST7789自动X/Y显示
-> Arbitrary Wave重采样
-> DAC DMA周期复现
```

它组合调用 `adc_dual_sync`、`trigger_capture`、`arbitrary_wave`、`dac_dma` 和 `st7789`，不重复修改这些模块。

## 2. 复制文件

把以下文件复制到工程 `modules/`：

- 本目录 `signal_single_capture_replay.c/.h`
- `01_bsp/common/signal_status.h`
- `02_acquisition/adc_dual_sync/signal_dual_adc_mspm0g3507.c/.h`
- `02_acquisition/trigger_capture/signal_trigger_capture.c/.h`
- `06_generator/arbitrary_wave/signal_arbitrary_wave.c/.h`
- `06_generator/dac_dma/signal_dac_dma_mspm0g3507.c/.h`
- `12_external_devices/display/st7789` 对应显示核心和平台文件

如果工程本来已有这些依赖，不要重复复制第二份。

## 3. SysConfig配置

本组合模块不新增资源，严格复用上游模块README：

1. 双路同步ADC：Timer事件同时触发ADC0/ADC1，DMA连续多块采集。`example04`中ADC0波形脚为PA25。
2. COMP0：模拟输入PA27，负端使用内部DAC门限约1.65 V；打开输出上升沿和下降沿中断。MSPM0G3507的ISR名称是`GROUP1_IRQHandler`。
3. DAC DMA：Timer事件触发DAC，DMA重复输出波表。
4. ST7789：按ST7789模块README配置SPI、CS、DC和背光。

模块不写死SysConfig生成实例名。Application只需提供`clear_trigger`和`consume_trigger`两个小回调，用本工程的`SIGNAL_COMP_INST`宏清除/读取COMP边沿标志。

## 4. 最小调用顺序

完整初始化代码直接复制 `README_MINIMAL_EXAMPLE.c`。比赛主流程只有：

```c
/* D键 */
(void)SignalSingleCaptureReplay_Arm(&g_capture);

/* COMP ISR清标志后 */
SignalSingleCaptureReplay_NotifyTrigger(&g_capture);

/* 主循环 */
if (SignalSingleCaptureReplay_Service(&g_capture) == SIGNAL_RESULT_OK) {
    /* 捕获成功，刷新页面 */
}

/* 选中槽位后 */
(void)SignalSingleCaptureReplay_ReplaySelected(&g_capture);
```

## 5. ST7789安全绘图

调用`SignalSingleCaptureReplay_DrawSelectedST7789()`。模块按屏幕列抽样，自动把每个Y坐标限制在绘图区内，再用修复后的`TFT_ST7789_DrawLine()`连接相邻列，得到连续曲线。ST7789模块的Bresenham实现必须先保存`e2 = 2 * err`，再用同一个`e2`判断X/Y步进；不能在修改`err`后重新计算第二个判断，否则某些斜率下可能无法正确收敛。组合模块仍检查槽位长度、图框范围和每个端点，避免把异常坐标交给屏幕驱动。

## 6. 参数怎么改

| 参数 | 当前实板值 | 作用 |
|---|---:|---|
| `requested_sample_rate_hz` | 1000000 | ADC捕获采样率 |
| `samples_per_block` | 416 | 固定捕获窗，1MSPS时416us |
| `pretrigger_samples` | 208 | 触发前历史长度 |
| `baseline_samples` | 32 | 首部基线平均点数 |
| `minimum_activity_codes` | 32 | 最小活动门限 |
| `activity_run_samples` | 3 | 连续活动点数，排除孤立毛刺 |
| `quiet_tail_samples` | 8 | 尾部至少保留的安静点数 |
| `edge_margin_samples` | 4 | 裁剪后首尾额外保留点数 |
| `slot_count` | 3 | 波形存储槽位数 |

缓冲区容量必须与这些参数匹配。模块Init会检查容量，不够会返回`SIGNAL_RESULT_INVALID_ARGUMENT`。

## 7. main允许保留的少量逻辑

- COMP硬件标志的读取和清除，因为宏名由当前工程SysConfig生成。
- D键调用`Arm`，1/2/3键调用`SelectSlot`，#键调用`ReplaySelected`。
- 页面文字，例如`WAIT SIGNAL`、`SLOT READY`和`DUR`。
- `GROUP1_IRQHandler`中清中断后调用`NotifyTrigger`。

跨块DMA、触发搜索、完整性判断、基线裁剪、槽位管理、绘图坐标和DAC重采样都不再写在main。

## 8. 验证状态

`MODULE_STATUS_BOARD_VERIFIED`。证据来自2026-08-19的`fuxian/example04`实板结果：PA25连续ADC、PA27内部COMP触发，能够捕获50us到200us单次任意波，裁剪首尾基线，安全显示并周期回放。工程构建使用TI Arm Clang `-Wall -Werror`通过。

## 9. 常见错误

- 一直`WAIT SIGNAL`：检查PA27是否跨过COMP门限、ISR是否为`GROUP1_IRQHandler`。
- 未触发却捕获：武装前必须由`clear_trigger`清除COMP和NVIC遗留标志。
- 波形不完整：提高采样率或增加` samples_per_block/pretrigger_samples`，同时扩大缓冲区。
- 总是拒绝保存：尾部没有`quiet_tail_samples`个安静点，说明捕获窗太短。
- 回放周期错误：必须保留捕获时真实`sample_rate_hz`，模块已经按重采样点数同比调整DAC更新率。
