# External Device README Completeness Audit

审计日期：2026-08-17  
扫描范围：`12_external_devices/**/README.md`  
总数：47 份 README。

## 1. 先分清 README 的职责

不能拿同一套规则检查所有 README。本轮把文档分成六类：

| 类型 | 数量 | 必须承担的职责 |
|---|---:|---|
| `LIBRARY_INDEX` | 1 | 说明整个外部器件库如何进入 |
| `CATEGORY_INDEX` | 11 | 列出该类别现有具体器件/教程 |
| `FUTURE_CATEGORY_INDEX` | 8 | 没有具体料号时说明先确认什么，并链接对应 Recipe |
| `GENERIC_TUTORIAL` | 13 | 教通用原理、功能接线、SysConfig、Bring-Up、Generic main、迁移方法 |
| `EXACT_DEVICE_GUIDE` | 13 | 按完整型号的官方资料给出可操作教程；其中有 Driver 的继续标记编译状态 |
| `MIGRATION_READY` | 1 | 从旧 Arduino/STM32 资料抽取事实、迁移角色和验证步骤；不伪造 MSPM0 Driver |

`LIBRARY_INDEX`、`CATEGORY_INDEX` 和 `FUTURE_CATEGORY_INDEX` 不是器件教程，不强制放 `int main()`。真正承担比赛学习职责的是 26 份 `GENERIC_TUTORIAL` / `EXACT_DEVICE_GUIDE`；`MIGRATION_READY` 另有独立的小白整理教程，不把旧平台代码当作可编译驱动。

## 2. 完整性判定标准

### Family / Generic README

必须有：用途与工作流程、常见功能脚、拿到实物先确认项、MSPM0 所需外设、功能接线、SysConfig、分阶段 Bring-Up、明确写着 `GENERIC TEMPLATE` 或 `TODO_MODEL_SPECIFIC` 的 main、比赛最常改参数、高速升级（适用时）以及“换成另一个同类型号怎么办”。

不允许在型号未知时编造 Pin Number、Supply Voltage、SPI Mode、Timing Number 或 Register Value。

### Exact Device README

必须有：官方 Product/Datasheet，能找到时再给官方 EVM/Application/Example；供电与逻辑兼容、Pin 功能接线、SysConfig、真实 API、Power-Up/Reset、Bring-Up、最小 main、参数换算、比赛参数和常见错误。任何代码/硬件状态必须有证据。

### Future Category README

至少给出该类器件需要确认的关键事实、最小验证方向、对应接口 Recipe 与 Unknown Device Bring-Up Guide。选定完整料号后另建 Exact Device 目录。

## 3. 状态词重新定义

| 状态 | 本库中的准确含义 |
|---|---|
| `DOCUMENTATION_ONLY` | 当前没有可声明为正式、已编译的具体 Driver；README 仍必须是一份完整教程 |
| `DATASHEET_REQUIRED` | 型号相关的电气、Pin、寄存器和时序必须从完整料号官方资料确认；不是省略教程的理由 |
| `GENERIC_TUTORIAL` | 通用教程完整，但模板中的 `TODO_MODEL_SPECIFIC_*` 不能冒充真实 API |
| `EXACT_DEVICE_GUIDE` | 已按明确料号及官方资料整理；不自动代表有 Driver |
| `COMPILE_VERIFIED_DRIVER` | 具体 `.c/.h` Driver 存在，并已有 TI Arm Clang 目标源码编译证据；不等于完整 Application Link 或上板 |
| `BOARD_VERIFIED` | 只有真实器件、真实接线和运行结果才能使用；本轮没有新增任何此状态 |

## 4. 整改前的系统性问题

2026-08-11 初扫 46 份 README：

- 32 份少于 40 行；
- 32 份没有出现 SysConfig 使用说明；
- 33 份没有 main 框架；
- 多个 Family 叶子目录只有 `DATASHEET REQUIRED`、`DOCUMENTATION ONLY` 和数行提醒；
- 类别索引与器件教程混在同一统计里，导致“索引没有 main”与“器件教程不完整”无法区分。

## 5. 从 PLACEHOLDER 升级为 GENERIC_TUTORIAL

以下 13 份原短占位 README 已重建。它们没有伪造完整料号事实，也没有创建假 Driver。

| 目录 | 现在能教会什么 | 状态 |
|---|---|---|
| [AD7606 family](../adc/ad7606_family/README.md) | 同步采样、CONVST/BUSY、串/并行整帧读取、Timer/DMA 升级 | `GENERIC_TUTORIAL` |
| [Generic SPI ADC](../adc/generic_spi_adc/README.md) | blocking 单帧、Ready、raw 解析、连续 DMA 升级 | `GENERIC_TUTORIAL` |
| [CD4051/74HC4051](../analog_switch/cd4051_74hc4051/README.md) | 8:1 复用、功能接线、切换稳定时间 | `GENERIC_TUTORIAL` |
| [Generic SPI DAC](../dac/generic_spi_dac/README.md) | 静态三点、VREF/code、LDAC、波表 DMA | `GENERIC_TUTORIAL` |
| [TLC5615 family](../dac/tlc5615_family/README.md) | 串行 DAC 的安全静态 Bring-Up | `GENERIC_TUTORIAL` |
| [Generic SPI DDS](../dds/generic_spi_dds/README.md) | tuning word、Reset/Update、参考时钟 | `GENERIC_TUTORIAL` |
| [Generic SPI Digital Pot](../digital_pot/generic_spi_digital_pot/README.md) | A/W/B 安全、档位、易失性、校准 | `GENERIC_TUTORIAL` |
| [X9C family](../digital_pot/x9c_family/README.md) | CS/U-D/INC 三线步进与保存 | `GENERIC_TUTORIAL` |
| [SSD1306 0.96 I2C](../display/ssd1306/README.md) | 已于 2026-08-12 按 WH-X096-2864KSWEG01-A4 规格升级为明确型号驱动 | `EXACT_DEVICE_GUIDE / COMPILE_VERIFIED_DRIVER / COPY_READY` |
| [EC11](../input_devices/ec11/README.md) | A/B 状态序列、轮询/中断、去抖 | `GENERIC_TUTORIAL` |
| [Programmable Filter Control](../programmable_filter/control_template/README.md) | 档位表、控制、稳定等待、扫频校准 | `GENERIC_TUTORIAL` |
| [AD603 Control Adapter](../programmable_gain/ad603_control_adapter/README.md) | gain 到控制电压、DAC/校准、AGC 前置验证 | `GENERIC_TUTORIAL` |
| [GPIO Relay](../relay/gpio_relay/README.md) | 驱动级、COM/NO/NC、安全默认与抗干扰 | `GENERIC_TUTORIAL` |

## 6. 明确型号 P0 审查结果

本轮对用户指定的 10 个明确型号重新核对官方 Product Page、Datasheet，并在能找到时加入官方 EVM/Application 资料。README 均已标记为 `EXACT_DEVICE_GUIDE`。

| 器件 | README | Driver 状态 | Board 状态 |
|---|---|---|---|
| ADS112C04 | [README](../adc/ads112c04/README.md) | `COMPILE_VERIFIED_DRIVER` | 未验证 |
| ADS7866 | [README](../adc/ads7866/README.md) | `COMPILE_VERIFIED_DRIVER` | 未验证 |
| ADS7887 | [README](../adc/ads7887/README.md) | `COMPILE_VERIFIED_DRIVER` | 未验证 |
| DAC7811 | [README](../dac/dac7811/README.md) | `COMPILE_VERIFIED_DRIVER` | 未验证 |
| PGA113 | [README](../programmable_gain/pga113/README.md) | `COMPILE_VERIFIED_DRIVER` | 未验证 |
| TPL0401A-10 | [README](../digital_pot/tpl0401a_10/README.md) | `COMPILE_VERIFIED_DRIVER` | 未验证 |
| X9C104 | [README](../digital_pot/x9c104/README.md) | README 直接 GPIO Recipe，无独立 Driver | 未验证 |
| TCA6408A | [README](../gpio_expander/tca6408a/README.md) | `COMPILE_VERIFIED_DRIVER` | 未验证 |
| AD9833 | [README](../dds/ad9833/README.md) | `COMPILE_VERIFIED_DRIVER` | 未验证 |
| AD9850 | [README](../dds/ad9850/README.md) | `COMPILE_VERIFIED_DRIVER`，另有 core PC test | 未验证 |

另外三份此前已详细整理的明确型号文档继续作为 `EXACT_DEVICE_GUIDE`：

- [CD4052B/CD4053B](../analog_switch/cd4052_cd4053/README.md)
- [CD4066B](../analog_switch/cd4066b/README.md)
- [MAX14752](../analog_switch/max14752/README.md)

它们使用 README 中的直接 GPIO 示例，没有独立 `.c/.h`，因此不标 `COMPILE_VERIFIED_DRIVER`。

## 7. P2 Future Category 处理结果

以下 8 个还没有具体料号，不适合伪造 Exact Guide。现已统一为 `FUTURE_CATEGORY_INDEX`，并链接真实接口 Recipe：

| 类别 | 入口 |
|---|---|
| 可编程衰减器 | [attenuator](../attenuator/README.md) |
| 隔离器件 | [isolation](../isolation/README.md) |
| 电平转换 | [level_shifter](../level_shifter/README.md) |
| 外部存储 | [memory](../memory/README.md) |
| 其他候选器件 | [miscellaneous](../miscellaneous/README.md) |
| PLL/时钟合成 | [pll_synthesizer](../pll_synthesizer/README.md) |
| 电源控制 | [power_control](../power_control/README.md) |
| 外部传感器 | [sensor](../sensor/README.md) |

## 8. 最终自动复扫

对 26 份叶子教程检查以下文本/结构证据：

| 检查项 | 通过数 |
|---|---:|
| 合理教程长度（至少 50 行） | 26 / 26 |
| SysConfig | 26 / 26 |
| 功能接线 | 26 / 26 |
| Bring-Up | 26 / 26 |
| `int main` 示例/模板 | 26 / 26 |
| 比赛最常改参数 | 26 / 26 |
| README 类型标记 | 26 / 26 |

最终叶子 README 完整性缺口：**0**。

## 9. 仍然存在、但不能用文档伪装解决的缺口

1. 本轮没有真实器件上板，因此全部不能写 `BOARD_VERIFIED`。
2. 除已升级的 SSD1306 外，其余 `GENERIC_TUTORIAL` 仍需在拿到完整型号后建立 Exact Device Guide/Driver；其 `TODO_MODEL_SPECIFIC_*` 是教学占位，不是 API。
3. 8 个 `FUTURE_CATEGORY_INDEX` 还没有完整料号，只能给类别入口和 Recipe。
4. X9C104、CD4052/CD4053、CD4066B、MAX14752 当前采用直接 GPIO Recipe，没有独立编译单元；这不是 `COMPILE_VERIFIED_DRIVER`。
5. `COMPILE_VERIFIED_DRIVER` 只说明具体 Driver 源码通过目标编译；应用级链接与硬件功能仍要在实际工程中验证。

因此，“README 已完整”与“器件已验证”被严格分开：文档不再是占位，但验证状态也没有被夸大。
