# MSPM0G3507 Platform Adapter

## MSPM0G3507 比赛推荐方式

本目录不再作为“所有外设必须经过的平台总入口”。锁定 MSPM0G3507 后：

- GPIO、DAC fixed code、UART blocking、单次 ADC、Timer start/stop/read 等简单动作：直接 SysConfig + DriverLib；
- Comparator Capture 与 ILI9341 等包含 ISR/state/协议时序的专用文件：继续保留；
- `signal_mspm0g3507_platform.c/.h` 中通用 ADC/DAC/GPIO/UART/Timer/DMA/Comparator callback binding：兼容旧工程，**新工程通常不推荐**。

选择规则见 [WHEN_TO_USE_DRIVERLIB_OR_MODULE.md](../../../00_docs/WHEN_TO_USE_DRIVERLIB_OR_MODULE.md)。

这里曾作为仓库唯一的通用 MSPM0G3507 DriverLib 落地层，把 BSP callback/descriptor 转换为 DriverLib 调用。它没有被删除，是因为 ADC Timer Trigger、旧最小例子和文档仍有逆向依赖；新代码不应因为这些兼容入口存在就强制使用它。

公开 API 的唯一真相是本目录三个头文件：

- `signal_mspm0g3507_platform.h`：ADC、DAC、GPIO、Button/Keypad、UART、Timer、DMA、Comparator；
- `signal_mspm0g3507_capture_platform.h`：Comparator + Timer Capture ISR/state；
- `signal_mspm0g3507_tft_platform.h`：ILI9341 SPI/GPIO/delay callbacks。

README 不定义另一套接口。修改任一 public `.h` 后，必须重新运行 `tools/build_platform_closure.ps1`。

## Hardware / Platform Binding

| 上层 callback | 正式绑定入口 | 最终 DriverLib/硬件 | 用户自己写？ |
|---|---|---|---:|
| ADC read/enable/disable | `SignalMSPM0G3507_ADC_Bind` | ADC12 software trigger/MEM result | 旧兼容；新工程直接 DriverLib |
| DAC write | `SignalMSPM0G3507_DAC_Bind` | `DL_DAC12_output12` / DAC12 | 旧兼容；新工程直接 DriverLib |
| GPIO read/write/toggle | `SignalMSPM0G3507_GPIO_Bind` | `DL_GPIO_*` | 旧兼容；新工程直接 DriverLib |
| Button/Latching input | `SignalMSPM0G3507_GPIO_ReadActive` | GPIO electrical level → logical state | 否 |
| Keypad row/column/delay | `SignalMSPM0G3507_KeypadDriveRow` 等 | 8 个 GPIO | 否 |
| UART read/write | `SignalMSPM0G3507_UART_Bind` | UART Main | 旧兼容；新工程直接 DriverLib |
| Timer callbacks | `SignalMSPM0G3507_Timer_Bind` | TimerG | 旧兼容；基础动作直接 DriverLib |
| DMA callbacks | `SignalMSPM0G3507_DMA_Bind` | DMA channel；trigger/mode 仍由 SysConfig 定义 | 旧兼容；复杂采集/输出用专用模块 |
| Comparator apply | `SignalMSPM0G3507_Comparator_Bind` | COMP DAC8/hysteresis/polarity | 旧兼容；静态配置优先 SysConfig |
| Capture ISR/state | `SignalMSPM0G3507_Capture_*` | Comparator Event → Timer Capture IRQ | 否 |
| ILI9341 callbacks | `SignalMSPM0G3507_TFT_Bind` | SPI/GPIO/delay | 否 |

OPA/GPAMP 没有伪造实现：现有公共 config 不能完整表达 MSPM0G3507 的离散增益、MUX、引脚和 bias source，仍是 `API_GAP`。

## Copy Into Target Project

按功能只链接需要的唯一源码：

- 通用 ADC/DAC/GPIO/UART/Timer/DMA/Comparator：`signal_mspm0g3507_platform.c`；
- Comparator capture：另加 `signal_mspm0g3507_capture_platform.c`；
- ILI9341：使用 `signal_mspm0g3507_tft_platform.c`。

Include Path 至少加入本目录、相应 BSP 目录、`01_bsp/common`、SDK `source`、CMSIS Core 和目标工程 SysConfig 生成目录。不要把这些源码复制进 Application；projectspec 链接唯一正式位置。

## Public API Map

下表只列名字和职责，参数顺序以头文件为准。

| 功能 | 当前真实 public API |
|---|---|
| ADC Basic | `SignalMSPM0G3507_ADC_Bind`、`SignalMSPM0G3507_ADC_Read`、`SignalMSPM0G3507_ADC_Enable`、`SignalMSPM0G3507_ADC_Disable` |
| DAC DC | `SignalMSPM0G3507_DAC_Bind`、`SignalMSPM0G3507_DAC_Write` |
| GPIO | `SignalMSPM0G3507_GPIO_Bind`、`SignalMSPM0G3507_GPIO_Write`、`SignalMSPM0G3507_GPIO_Read`、`SignalMSPM0G3507_GPIO_Toggle`、`SignalMSPM0G3507_GPIO_ReadActive` |
| Keypad | `SignalMSPM0G3507_KeypadDriveRow`、`SignalMSPM0G3507_KeypadReadColumn`、`SignalMSPM0G3507_DelayUs` |
| UART | `SignalMSPM0G3507_UART_Bind`、`SignalMSPM0G3507_UART_Write`、`SignalMSPM0G3507_UART_Read` |
| Timer | `SignalMSPM0G3507_Timer_Bind`、`SignalMSPM0G3507_TimerSetPeriod`、`SignalMSPM0G3507_TimerStart`、`SignalMSPM0G3507_TimerStop`、`SignalMSPM0G3507_TimerRead` |
| DMA | `SignalMSPM0G3507_DMA_Bind`、`SignalMSPM0G3507_DMA_Configure`、`SignalMSPM0G3507_DMA_Start`、`SignalMSPM0G3507_DMA_Stop` |
| Comparator | `SignalMSPM0G3507_Comparator_Bind`、`SignalMSPM0G3507_Comparator_Apply` |
| Capture | `SignalMSPM0G3507_Capture_Init`、`SignalMSPM0G3507_Capture_Start`、`SignalMSPM0G3507_Capture_Stop`、`SignalMSPM0G3507_Capture_IsFinished`、`SignalMSPM0G3507_Capture_GetCount`、`SignalMSPM0G3507_Capture_Copy` |
| TFT | `SignalMSPM0G3507_TFT_Bind`、`SignalMSPM0G3507_TFT_Write`、`SignalMSPM0G3507_TFT_SetCS`、`SignalMSPM0G3507_TFT_SetDC`、`SignalMSPM0G3507_TFT_SetReset`、`SignalMSPM0G3507_TFT_SetBacklight`、`SignalMSPM0G3507_TFT_DelayMs` |

## Capture Adapter 的时钟与回绕契约

`signal_mspm0g3507_capture_platform.c` 读取向下计数 Capture 寄存器后，只转换为当前 Timer 周期内的递增模数时间戳：

```text
timestamp = counter_modulus - 1 - raw_capture
```

结构体里的 `overflow_count` 目前只用于到期结束采集，没有拼入时间戳。因此它不是 32/64-bit 扩展计数器，不能恢复相邻输入边沿之间的多次 Timer 回绕。使用方必须保证：

```text
counter_modulus = 当前工程生成的 SIGNAL_CAPTURE_INST_LOAD_VALUE + 1
输入周期 < counter_modulus / Timer真实计数频率
```

配置方法是在 CCS 中双击 `.syscfg`，通过 SysConfig 图形界面的 `SYSCTL` 与 `SIGNAL_CAPTURE` 页面选择 Clock Source、Divider/Prescaler 和 Period；不要直接编辑 `.syscfg` 文本或生成的 `ti_msp_dl_config.*`。保存后从图形页的 Calculated Clock 和生成的 LOAD 核对应用参数。

测 10 Hz 的推荐起点是板载 LFXT 32.768 kHz → LFCLK/2 → Capture Timer 16384 Hz、Period 2 s；应用传 `counter_modulus=LOAD+1`，频率计算层传 `timer_hz=16384`。`timeout_overflows` 仅设置无足够边沿时的最长等待，例如值 2 对应约 4 s 超时。

若需要一套配置同时覆盖 10 Hz 和很高频率，不要只增大 `timeout_overflows`。应在应用层切换慢/快两套 SysConfig 对应工程配置，或另行实现能处理 ZERO/Capture 同时到达竞争条件的扩展时间戳；当前 Adapter 不提供该能力。

## This platform is used by

| 模块/功能 | 模块 README | 真实 Example |
|---|---|---|
| DAC DC / BSP DAC | [DAC DC](../../../06_generator/dac_dc/README.md) / [DAC](../../../01_bsp/dac/README.md) | [dac_dc_minimum](../../../09_examples/platform_closure/dac_dc_minimum/README.md) |
| ADC Basic / BSP ADC | [ADC Basic](../../../02_acquisition/adc_basic/README.md) / [ADC](../../../01_bsp/adc/README.md) | [adc_basic_minimum](../../../09_examples/platform_closure/adc_basic_minimum/README.md) |
| ADC Timer Trigger | [README](../../../02_acquisition/adc_timer_trigger/README.md) | [adc_timer_trigger_minimum](../../../09_examples/platform_closure/adc_timer_trigger_minimum/README.md) |
| ADC Continuous callback | [README](../../../02_acquisition/adc_continuous/README.md) | [adc_continuous_minimum](../../../09_examples/platform_closure/adc_continuous_minimum/README.md) |
| GPIO | [README](../../../01_bsp/gpio/README.md) | [gpio_minimum](../../../09_examples/platform_closure/gpio_minimum/README.md) |
| UART | [README](../../../01_bsp/uart/README.md) | [uart_minimum](../../../09_examples/platform_closure/uart_minimum/README.md) |
| Comparator + Timer Capture | [Comparator](../../../01_bsp/comparator/README.md) / [Timer Capture](../../../02_acquisition/timer_capture/README.md) | [timer_capture_minimum](../../../09_examples/platform_closure/timer_capture_minimum/README.md) |
| TFT ILI9341 | [README](../../../01_bsp/tft_ili9341/README.md) | [TFT example](../../../09_examples/tft_ili9341_lp_mspm0g3507/README.md) |

ADC DMA、DAC DMA 和 Dual ADC 使用各自已经存在的正式硬件实现/Platform Adapter，见 [Platform Closure Examples](../../../09_examples/platform_closure/README.md)、[DAC DMA Platform](../dac_dma_platform_adapter/README.md) 和 [Dual ADC Platform](../dual_adc_platform_adapter/README.md)。

## SysConfig Profiles

| 功能 | 参考配置 |
|---|---|
| 软件 ADC、DAC DC、UART、GPIO | `PROFILE_07_BASIC_IO` |
| ADC DMA、ADC Timer Trigger | `PROFILE_01_ADC_CAPTURE` |
| Dual ADC | `PROFILE_02_DUAL_ADC` |
| DAC DMA | `PROFILE_03_DAC_GENERATOR` |
| Comparator Capture | `PROFILE_05_FREQUENCY` |
| TFT | `09_examples/tft_ili9341_lp_mspm0g3507/tft_ili9341.syscfg` |

## 【COMPILE-VERIFIED EXAMPLE】DAC DC direct DriverLib

下面代码来自真实 `dac_dc_minimum/main.c`，并由文档一致性脚本逐字符核对。

<!-- COMPILE_VERIFIED_EXAMPLE: 09_examples/platform_closure/dac_dc_minimum/main.c -->
```c
#include <stdint.h>

#include "ti_msp_dl_config.h"

volatile uint16_t g_dac_code = 2048U;

int main(void)
{
    SYSCFG_DL_init();
    DL_DAC12_output12(DAC0, g_dac_code);
    while (1) __WFI();
}
```

其余完整调用不要从 README 重新抄一套，直接打开上表中的真实 `main.c`。

## Documentation Code Rule

- `【COMPILE-VERIFIED EXAMPLE】`：必须带 `COMPILE_VERIFIED_EXAMPLE` source marker，代码与真实 `.c` 一致并进入 full-link 回归；
- `【ILLUSTRATIVE SNIPPET】`：只解释局部概念，不保证独立编译；
- 未标记的 API 名称表不是完整程序，参数顺序仍以 `.h` 为准。

## Validation

`tools/build_platform_closure.ps1` 先运行 `tools/validate_documentation_api_consistency.ps1`，再构建 Platform Minimal Examples。当前只允许写 `BUILD_VERIFIED`；没有开发板实测。
