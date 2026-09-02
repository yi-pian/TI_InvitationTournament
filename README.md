# MSPM0 信号测量与电赛开发资料库

这是一个面向 TI MSPM0G3507 的嵌入式信号处理与电子设计竞赛开发仓库，包含可复用的底层驱动、采集与测量算法、波形发生模块、显示与外设示例，以及若干完整应用和测试工程。

项目主要使用 C 语言开发，工程文件兼容 TI Code Composer Studio（CCS）和 SysConfig。仓库中的模块既可以单独复用，也可以组合成频率计、示波器、频谱分析仪、双通道相位测量仪、DDS 信号源等应用。

## 目录结构

核心代码位于 [`MSPM0_Signal_Contest`](MSPM0_Signal_Contest)：

| 目录 | 内容 |
| --- | --- |
| `00_docs` | SysConfig、采样率、信号处理、内存和模块集成等说明文档 |
| `01_bsp` | GPIO、ADC、DAC、DMA、定时器、UART、按键、显示等基础驱动 |
| `02_acquisition` | ADC 采集、定时触发、DMA、双通道同步和环形缓冲 |
| `03_measurement` | 频率、周期、占空比、相位、幅值等测量模块 |
| `04_dsp` | FFT、滤波、去直流、谐波和频谱相关算法 |
| `05_precision` | 高精度测量、校准和误差处理模块 |
| `06_generator` | 正弦波、方波、三角波、扫频和 DDS 波形发生 |
| `07_signal_frontend` | 运放、比较器、可编程增益和模拟前端模块 |
| `08_applications` | 可直接参考的完整应用工程 |
| `09_examples` | 外设和平台闭环示例 |
| `10_tests` | 模块测试、集成测试和离线验证脚本 |
| `11_legacy_compatibility` | 历史算法和兼容性实现 |
| `12_external_devices` | 外部显示、传感器、继电器、增益控制等设备接口 |
| `tools` | API 检查、资源检查、文档生成和工程验证脚本 |

此外，`fuxian` 目录保存了若干独立的比赛题目示例和可复用工程，适合按具体题目查阅；根目录的 PDF、DOCX 和 Markdown 文件是芯片、开发板及比赛调试参考资料。

## 推荐入口

如果希望快速了解完整应用，建议从以下目录开始：

- [`08_applications/signal_analyzer`](MSPM0_Signal_Contest/08_applications/signal_analyzer)：信号分析应用
- [`08_applications/frequency_meter`](MSPM0_Signal_Contest/08_applications/frequency_meter)：频率测量应用
- [`08_applications/spectrum_analyzer`](MSPM0_Signal_Contest/08_applications/spectrum_analyzer)：频谱分析应用
- [`08_applications/dual_channel_phase_meter`](MSPM0_Signal_Contest/08_applications/dual_channel_phase_meter)：双通道相位测量应用
- [`08_applications/dds_generator`](MSPM0_Signal_Contest/08_applications/dds_generator)：DDS 波形发生应用
- [`fuxian/ready_project`](fuxian/ready_project)：整理好的独立示例工程

每个模块或应用目录中的 `README.md`、`MODULE_CARD.md` 和示例源文件，应作为该模块的具体使用说明。

## 开发环境

建议准备：

1. TI Code Composer Studio（CCS）
2. 与 CCS 配套的 MSPM0 SDK 和 SysConfig
3. LP-MSPM0G3507 或兼容的 MSPM0G3507 开发板
4. USB 调试器或板载 XDS 调试接口

具体 SDK、编译器和 SysConfig 版本应以本地 CCS 工程能够正常打开和生成代码为准。

## 使用方式

1. 在 CCS 中导入或打开目标应用目录中的 `.project` / `.ccsproject` 工程。
2. 使用 SysConfig 打开对应的 `.syscfg` 文件，确认芯片型号、引脚和外设配置。
3. 根据应用目录中的 README 检查模块路径和外部器件连接。
4. 选择目标工程并执行 Build，再下载到 MSPM0G3507 开发板运行。

模块复用时，优先复制目标模块目录中的 `.c`、`.h` 及其依赖模块，并同步检查 `MODULE_CARD.md` 或模块 README 中的初始化顺序、缓冲区大小和硬件约束。

## 验证工具

`MSPM0_Signal_Contest/tools` 中提供了若干 PowerShell 和 Python 工具，可用于：

- 检查模块 API 和依赖关系
- 检查工程路径、外设资源和内存使用
- 验证 SysConfig 生成结果
- 执行模块集成和回归测试
- 生成模块文档

这些工具通常需要在 Windows PowerShell、Python 以及已安装 TI 工具链的环境中运行。运行前请先阅读对应目录的 README 和脚本参数说明。

## 仓库整理说明

仓库已忽略 CCS 编译输出、临时目录、日志、Python 缓存和本地工具状态。构建产物应在本地生成，不建议提交到 Git；如果需要发布可下载固件，建议通过 GitHub Releases 单独提供。

## 许可与使用

本仓库主要用于学习、实验和电子设计竞赛开发。仓库中引用的 TI、芯片厂商及第三方资料仍归原版权方所有；使用外部资料时请遵守相应许可证和版权要求。
