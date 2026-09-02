# 步骤 7：main.c 代码归属与逐段解释

`main.c` 的详细行内注释已标明“复制”或“自写”。按比赛检查顺序可分为：

| main 区域 | 代码来源 | 做什么 |
|---|---|---|
| 头文件、`g_raw_a/b` | 模块接口/本题数组 | 引入已复制模块，保留两路 DMA 数据 |
| `App_MapX/Y`、绘图四函数 | 自写 + ST7789 README API | 将采样数据组合为李萨如轨迹；动态帧采用受限短段 `TFT_ST7789_DrawLine`，端点和长度先验证 |
| `App_SetComparatorMode`、ADC 门限统计 | README 补充/自写 | 切 COMP0.DAC8；因实板异步 COMP IRQ 会卡住，按同一门限统计 ADC 帧边沿 |
| `SysTick_Handler` | 键盘 README + `22_X` 方式 | 5 ms 扫描并直接分发稳定新键值 |
| `App_ProcessKey` | 自写 | 仅修改页面、频率比、COMP 模式及局部刷新 revision |
| 四个 `MakeConfig` 与两组 BUFFER 预算 | GPAMP/OPA/COMP README | 软件预算参数，不直接配置寄存器 |
| `SYSCFG_DL_init` | SysConfig 生成接口 | 真正初始化 GPAMP/OPA/COMP/ADC/DMA/Timer |
| `SignalDualADC_*` | ADC README | 启动 DMA、WFI 等待同步完成 |
| `SignalDualADCPhase_Process` | 相位 README | 计算相位 |

关键逐行关系：`SYSCFG_DL_init()` 必须早于所有外设调用；COMP0 的 NVIC 在本板保持关闭，避免异步 IRQ 卡住显示；每轮先 `Start`，随后在 `IsFinished` 循环 `__WFI()`，DMA 完成后才允许相位/绘图读取数组。显示按 `22_X` 的 revision 机制局部刷新：常态只更新波形内部区域和数字字段，静态框架只在上电或换页时重画。ST7789 模块的 `DrawLine` 已修复 `e2` 快照缺陷，主程序只调用端点在绘图区内且不超过 8 像素的短线段。

已经执行的静态验证：SysConfig CLI 无 error；CCS `gmake -C Debug all` 已完整编译链接通过。链接 map 显示 SRAM 使用 2859/32768 B（含 512 B 栈），不存在链接期内存溢出。未完成上板验收，不能把此结果写成 BOARD_VERIFIED。
