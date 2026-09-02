# Verification Truth

## 状态总表

| 层级 | 状态 | 证据范围 |
|---|---|---|
| PC_VERIFIED | `NOT_RUN` | 完整 CCS 固件没有 PC 执行；各算法模块的独立 PC 状态不能自动升级整个案例 |
| COPY_READY | `COPY_READY` | 本案例知识文件完整并由案例检查器验证；不表示旧工程适合整套复制 |
| SYSCONFIG | `SYSCONFIG_VERIFIED` | 2026-08-13 使用工程声明的 SysConfig 1.28.0、SDK 2.11.0.07 在临时目录 CLI 验证，返回 0，生成 6 个文件 |
| COMPILE | `COMPILE_VERIFIED` | 最终源对应的 object 和依赖文件存在 |
| BUILD | `BUILD_VERIFIED` | `.out/.map/main_template.o` 时间均为 2026-08-12 12:03:40，`main_template.c` 为 12:03:39 |
| BOARD | `BOARD_VERIFIED`（限定范围、用户证明） | 开发会话中完成 DDS/DAC、键盘/TFT 和示波器交叉验证；最终用户声明 A 题已完成。没有自动化原始采样日志 |
| CONTEST | `NOT_RUN` | 没有正式竞赛运行记录，也没有一键三参数/60 秒完整验收记录 |

## 证据指纹

| 证据 | SHA-256 |
|---|---|
| `fuxian/24A题.png` | `62358c2de9b6f8e24bb7b3ef128488c004d2d5cc708ff9c16261fbdae6edd0e5` |
| `main_template.c` | `777d2a357e9f5189adde3604a67386bab978c68c7c2f0a7a02f2856e77e47b08` |
| `profile.syscfg` | `0c52919236352c9de4ea854965b488743cc51b7865c5e446f6360ab39782ea17` |
| 最终 `.map` | `da17e9e2984ac05cc3e46ebdcdb7be6b93e23deb492284a0e4dcf77c5631aa17` |
| 最终 `.out` | `10d413192f75973be359b8ba933c27f09d92d365f76a0cea3d16e1843f0865a1` |
| `24A_Q2_Q4_BEGINNER_GUIDE.md` | `0bfbc5509d85af9b2fb696fe5cddd5ca1cb095383fedba0ab88b972bf7e04e9a` |

指纹的作用是防止以后把修改后的工程误说成这个历史案例。指纹不证明电气性能。

## 最终 Build/Map 事实

Map 的 Memory Configuration：

| 区域 | 总量 | 已用 | 剩余 |
|---|---:|---:|---:|
| FLASH | 131,072 B | 36,832 B (`0x8fe0`) | 94,240 B (`0x17020`) |
| SRAM | 32,768 B | 26,900 B (`0x6914`) | 5,868 B (`0x16ec`) |
| Stack section | — | 512 B (`0x200`) | 包含在布局中，不能把全部 free SRAM 当栈余量 |

两个最大 BSS：

- `g_wave_capture`: 12,288 B (`0x3000`)
- `g_wave_voltage`: 12,288 B (`0x3000`)

最终 Map 中存在 `SignalACRMS_Process` 与 `SignalRobustPeakToPeak_Process`，证明 Q2/Q3 算法被链接，而不是只把源文件放在目录里。

## SysConfig 验证事实

独立验证读取的是历史工程 `profile.syscfg`，输出写入临时目录并删除，没有覆盖成功工程生成文件。

确认内容：

- MSPM0G3507，LQFP-64，ticlang。
- MSPM0 SDK 2.11.0.07，SysConfig 1.28.0。
- `SIGNAL_ADC` / ADC0、`POWER_ADC` / ADC1、DAC0、SPI1、TIMG0、DMA_CH0、GPIO 和 UART0。
- 结果：`status=ok`、`returncode=0`、`generated_files=6`。
- SysConfig 输出 4 条 `info`，没有 warning/error；包括 ADC Power-down 提示、SPI retention 提示和 DMA Full Channel 提示。

## 板测证据边界

开发记录包含以下可追溯事实：

- AD9850 最初无输出，用户发现接线错误，纠正后输出正常并可改频率。
- DAC0 PA15 在打开 DAC amplifier 后，code 2048 实测约 1.647 V。
- Q3 在名义 3.555556 MSPS 时，MCU 约 14.81 us 与示波器约 24.9 us 不符；切到 2 MSPS 后用户确认准确。
- Q2 的 Vpp 路线曾将示波器约 970 kHz 的截止点显示到约 1.3 MHz；最终构建源码改用三帧 AC RMS。
- 用户最后声明“现在A题已经全部完成了”。

没有保存示波器截图、ADC 原始数组、每个 Mode 的最终数值或全自动测试日志。因此不能把限定范围的用户证明扩写成计量校准报告。

## 不能升级的状态

- 有 `.out` 不等于 Board Verified；本例的 Board 状态来自用户开发会话，而不是 `.out`。
- 用户说完成不等于 Contest Verified；原题一键自动流程仍缺文件证据。
- 当前正式库 Card 的验证等级不能由这个历史案例反向覆盖。
- 新项目复制这些文件后，其 Compile/Build/Board 状态全部重新从 `NOT_RUN` 开始。
