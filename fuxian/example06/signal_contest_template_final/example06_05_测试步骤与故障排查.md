# example06-05：编译、烧录与实板测试

## 1. 先处理当前 CCS 链接错误

你提供的日志中，`main.c`、SysConfig 文件和所有模块均已编译成功；失败发生在链接器：

```text
FLASH memory range has already been specified
cannot find file "REGION_ALIAS"
```

原因是工程中残留了旧的 `tmp/syscfg` 生成副本，链接命令同时加入了 `Debug` 和
`tmp/syscfg` 两套 `device_linker`、`ti_msp_dl_config` 文件。

关闭 CCS 后，先把工程根目录的 `tmp` 文件夹改名为 `tmp_stale_backup`（不要直接删除，便于恢复），
再将工程内旧的 `Debug` 文件夹改名为 `Debug_stale_dual_backup`。`Debug` 只是 CCS 的构建产物，
重新 Build 会自动生成；本轮已按此方式保留旧目录备份。

重新打开 CCS 并 Refresh 工程，确认 Project Explorer 中不存在根目录 `tmp/syscfg`，然后执行：

1. Project -> Clean...
2. 右键工程 -> Build Project。
3. 检查最后的链接命令不再出现 `tmp/syscfg/ti_msp_dl_config.o`、`device_linker.lds`。
4. 最后应生成 `Debug/signal_contest_template_final.out`。

SysConfig 的 ADC 唤醒、SPI 休眠保持和 DMA Full Channel 信息是提示，不是错误。

## 2. 硬件连接检查

| 功能 | 引脚 |
|---|---|
| 单 ADC 混合输入 | PA25 / ADC0 通道 2 |
| ST7789 SCLK | PB9 |
| ST7789 MOSI | PB8 |
| ST7789 CS | PB6 |
| ST7789 DC | PB15 |
| ST7789 BLK | PB12 |
| 键盘行 R1~R4 | PB16、PB0、PB7、PB17 |
| 键盘列 C1~C4 | PB18、PB13、PB20、PB4 |

开发板、ST7789、信号源必须共地。ADC 输入必须限制在 0～3.3 V，不能直接输入双极性信号；
建议使用 1.65 V 直流偏置，例如目标信号为 30 kHz、约 100 mVpp，叠加带外干扰和噪声。
不要让叠加后的波形削顶。

## 3. CCS 烧录运行

1. 连接 XDS110，确认工程使用 `targetConfigs/MSPM0G3507.ccxml`。
2. Build 成功后点击 Debug Project。
3. CCS 下载 `Debug/signal_contest_template_final.out` 后运行（Resume）。
4. 若只想烧录不调试，在 CCS 中使用 Flash/Load 选项下载该 `.out`，然后复位运行。

本工程没有配置 UART，因此不要以串口输出作为主要验收手段，主要观察 ST7789 屏幕；调试时可在
`App_Measure()` 返回处查看 `measurement`，在 Expressions 中查看 `g_display_periods` 和
`g_raw[]`。

## 4. 按比赛小步骤验收

### 步骤一：单 ADC/DMA

先不给信号或输入稳定电平运行，确认程序不会停在 `App_Fail()`，DMA 能持续完成。设置断点到
`while (!SignalADC_IsFinished())` 后单步，完成后检查 `g_raw[]` 有 1024 个样本。

### 步骤二：ST7789 和 8×16 字库

运行后屏幕应显示 `WEAK SIGNAL`、`F:`、`VPP:`、`N:` 和波形框。若屏幕全黑，依次检查供电、
共地、CS/DC/BLK、SPI1 引脚和屏幕方向；若只有背光没有图像，重点检查 CS、DC 和 MOSI。

### 步骤三：矩阵键盘

分别按 `1`、`2`、`3`、`4`、`5`，屏幕 `N:` 应对应改变，波形横向应显示约 1～5 个目标周期。
其他按键不改变 `N:`。键盘失效时检查行输出、列上拉和 PB 引脚是否接反。

### 步骤四：频率与峰峰值

输入一个已知频率（例如 30 kHz）的目标正弦波，屏幕应显示：

- `F` 接近信号源频率；当前 1024 点、500 kS/s 的频率分辨率约为 488 Hz。
- `A` 接近目标正弦的峰值电压。
- `VPP` 为目标正弦的峰峰值；改变信号源幅度时应同步变化。

### 步骤五：10 倍带外干扰和自动量程

目标信号保持在 10～100 kHz，例如 30 kHz；加入 10 倍幅度的带外单频干扰，例如 200 kHz，
再逐渐增加噪声。屏幕的 `F/VPP` 仍应跟随目标频率，波形应自动调整到绘图区约 80% 高度。

若 10 倍干扰也位于 10～100 kHz，且没有参考通道或先验频率，仅凭一个混合输入不能保证识别
哪个峰是目标；这是输入信息不足，不是屏幕或键盘故障。

## 5. 常见故障定位

| 现象 | 优先检查 |
|---|---|
| 链接出现 FLASH/SRAM already specified | 清理根目录旧 `tmp` 和整个 `Debug` 后重新 Generate/Build |
| 程序停在 `App_Fail()` | 查看 `SignalADC_Init` 或 ST7789 初始化返回值 |
| 屏幕黑屏 | 电源、共地、CS/DC/BLK、SPI1 接线 |
| F 始终不变或无效 | ADC 输入偏置、PA25 接线、信号是否在 10～100 kHz |
| 波形削顶 | 降低信号源幅度，保证 ADC 输入 0～3.3 V |
| 按键无效 | 行列接线、PB 引脚、列上拉、矩阵方向 |
