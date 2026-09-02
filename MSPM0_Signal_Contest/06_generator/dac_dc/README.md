# DAC 固定直流输出

## 你真的需要这个模块吗？

**普通比赛使用不推荐本旧模块。** 固定 DAC 电压/code 直接使用 SysConfig + `DL_DAC12_output12()`，步骤更少、调试更直接。连续波形再选 DAC DMA。见 [TI DriverLib 初学者指南](../../00_docs/TI_DRIVERLIB_BEGINNER_GUIDE.md)。

## 30 秒拼装路线

1. 新 MSPM0G3507 工程不链接旧 DAC DC/BSP callback：`signal_dac_dc.c/.h` 为 `[REFERENCE ONLY]`；不复制。
2. `[GENERATED]` 目标 `.syscfg` 生成 `ti_msp_dl_config.*`；main include `ti_msp_dl_config.h`。
3. SysConfig 对照 P07：DAC0、12 bit、VDDA/VSSA reference、amplifier/output enabled、PA15。
4. `SYSCFG_DL_init()` 后调用 `DL_DAC12_output12(DAC0, code)`。
5. `code` 类型 `uint16_t`，范围 0..4095；真实输出在 PA15，单位 V 需按真实参考测量。
6. Clean → Build；先输出 2048 code，用万用表/示波器确认约半量程。

## 第一次把本模块加入母版工程

### STEP 1～4：文件、CCS、SysConfig 与参数

- `[LINK]` 无模块源，简单直流输出直接使用 SDK DriverLib；`[COPY]` 无；`[GENERATED]` `ti_msp_dl_config.*`；`[REFERENCE ONLY]` P07 和 `dac_dc_minimum`。
- 在母版 `.syscfg` 添加 DAC12，选择 DAC0/PA15、reference、amplifier/output；Pin 可选项以 SysConfig 冲突检查为准，不能凭空换成任意 GPIO。
- 题目给电压时先按 `code=round(Vout/Vref*4095)` 计算并限幅；VREF/负载/输出缓冲会影响真实电压。

### STEP 5～10：main、调用、结果与连接

```c
#include <stdint.h>
#include "ti_msp_dl_config.h"
volatile uint16_t g_dac_code = 2048U;
int main(void)
{
    SYSCFG_DL_init();
    DL_DAC12_output12(DAC0, g_dac_code);
    while (1) { __WFI(); }
}
```

初始化必须在 output12 前；`g_dac_code` 是写入码，不是测得电压。连接：DAC DC→PA15→DUT bias；DAC DC→OPA bias 需确认外部/内部路由；周期波形改用 DDS→DAC DMA→Platform→PA15。

### STEP 11～12：Build 与最小验证

保存 SysConfig → Clean → Build。`DAC0` 未定义=未添加/命名 DAC；无输出=输出 Pin/amplifier/reference 未启用或接错 PA15；电压不准=VREF/负载/码值换算问题。最小验证工程：`09_examples/platform_closure/dac_dc_minimum`。

## 根据题目修改参数

| 题目要求 | 在哪里改 | 影响/同步项 |
|---|---|---|
| 直流电压 | `g_dac_code` 或应用换算 | 0..4095，受 VREF 限制 |
| 输出范围/参考 | `.syscfg` DAC reference | 同步 code↔V 公式并实测 |
| 连续波 | 不继续循环写 DC | 换 DAC DMA/DDS |

## 比赛现场最常改的地方

经常改 code/目标电压；偶尔改 reference/amplifier；通常不要改 DAC instance/PA15 和底层初始化顺序，除非 SysConfig 明确支持且已核对接线。

## 从母版到成功调用：完整例子

上面的 `main.c` 加 P07 DAC 配置就是完整闭环：母版 → SysConfig → generated config → 写 code → PA15 → 仪表验证。

## MSPM0G3507 比赛推荐方式

固定电压/固定 code 输出直接使用 **SysConfig + TI DriverLib**。普通比赛工程不再推荐 `DAC DC → BSP DAC → callback → Platform Adapter`。

### STEP 1：SysConfig 配置

参考 `09_examples/integration_profiles/PROFILE_07_BASIC_IO/profile.syscfg`，确认：

- 实例为 DAC0；
- 12-bit binary；
- 参考源与真实硬件一致（该 Profile 为 VDDA/VSSA）；
- DAC amplifier、output 已 enable；
- 输出 Pin 为 PA15。

修改 `.syscfg` 后重新 Generate，不要编辑生成的 `ti_msp_dl_config.c/.h`。

### STEP 2：include 与初始化

```c
#include "ti_msp_dl_config.h"

SYSCFG_DL_init();
```

`SYSCFG_DL_init()` 已按 SysConfig 初始化并 enable 当前 DAC；不要在 `main.c` 重做静态初始化。

### STEP 3：直接输出 12-bit code

```c
DL_DAC12_output12(DAC0, 2048U);
```

- `DAC0`：MSPM0G3507 的 DAC12 instance；换工程时以生成头和 `.syscfg` 为准。
- `code`：12-bit 无符号码，范围 `0..4095`。
- `0U`：接近零量程；`2048U`：约半量程；`4095U`：接近满量程。
- 题目要求改变输出时，直接修改 code。

理想换算为 `code ≈ voltage_v / reference_voltage_v × 4095`。真实输出还受参考电压、DAC 误差、放大器设置和负载影响；先用 `2048U` 配合万用表/示波器验证约半量程。

### STEP 4：Build 与验证

可编译最小例子：[dac_dc_minimum/main.c](../../09_examples/platform_closure/dac_dc_minimum/main.c)。仓库回归命令：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/build_platform_closure.ps1
```

Build PASS 只证明 SysConfig/compile/link；当前没有开发板观测证据，状态不是 `BOARD_VERIFIED`。

## 旧 DAC DC 模块（DEPRECATED FOR NEW APPLICATIONS）

本目录的 `signal_dac_dc.c/.h` 与 `01_bsp/dac` 暂不删除，原因是要保持旧工程/source list 兼容。它们原来的完整链为：

```text
SignalDACDC_SetVoltage
  → SignalDAC_VoltageToRaw
  → SignalDAC_WriteRaw
  → write callback
  → MSPM0G3507 Platform Adapter
  → DL_DAC12_output12
```

这条链对“写一个固定 code”增加了 struct、callback、源文件和 include path，封装收益为负，因此新项目不要采用。`SignalDAC_VoltageToRaw()` 中的纯电压换算有一定 helper 价值，但当前仍与旧 DAC BSP 同目录；本轮不再创建第二份换算源码。

旧公开 API 仍以 [`signal_dac_dc.h`](signal_dac_dc.h) 和 [`signal_dac.h`](../../01_bsp/dac/signal_dac.h) 为准，只用于维护旧引用。

## Hardware / Platform Binding

新工程：**Not Applicable**，固定输出直接由 `ti_msp_dl_config.h` 中的 DAC0 与 DriverLib 绑定。旧兼容 Platform 位于 `08_applications/common/mspm0g3507`，但 `dac_dc_minimum` 已不再链接它。

## 1. 模块作用

兼容旧工程的“目标电压→code→callback write”链；新 MSPM0G3507 工程不推荐。

## 2. 输入

旧 API 输入 `signal_dac_t` descriptor、目标 `voltage_v` 与结果指针。

## 3. 输出

旧 API 成功时写硬件并返回实际 12-bit `written_raw`；新推荐方式直接写 code。

## 4. 依赖

旧源码依赖 `01_bsp/dac` 与 `signal_status.h`；新 direct 例子不依赖这些模块。

## 5. SysConfig 设置

见本文顶部 STEP 1：DAC0、12-bit、参考、amplifier/output 与 PA15。

## 6. 初始化方法

新工程只调用 `SYSCFG_DL_init()`；旧 descriptor/Bind 初始化仅为兼容。

## 7. 调用方法

新工程调用 `DL_DAC12_output12(DAC0, code)`。旧 `SignalDACDC_SetVoltage()` 不再作为比赛推荐入口。

## 8. 参数修改方法

固定输出只改 `code`；若按电压计算，必须同步真实 reference voltage。

## 9. 与其他模块如何连接

固定 DC 不连接模块链。连续波连接 Wave Table/DDS → DAC DMA。

## 10. 最小示例

以本文下方 compile-verified direct example 为准。

## 11. 常见错误

把 12-bit code 写出 `0..4095`、误认参考电压、漏开 DAC output/PA15、编辑生成文件、把 Build PASS 当实板 PASS。

## 12. RAM 占用

Direct example 的 map 为 514 B（含 512 B stack）；应用变量只增加一个 16-bit code。

## 13. Flash 占用

Direct example 当前为 1480 B；旧 wrapper 版本为 2448 B。

## 14. CPU 计算量估计

固定 code 仅一次 DriverLib 写；没有持续计算。连续波不要用 CPU 循环写，改用 DAC DMA。

## 15. 当前验证状态

Direct example 为 `BUILD_VERIFIED`（SysConfig/compile/final link PASS）；Board 为 `NOT_RUN`。

## 16. 以后实板验证步骤

先写 `2048U`，用万用表/示波器测 PA15 对 GND 的半量程附近电压，再测试 0/满量程附近 code、负载和参考误差。

## 【COMPILE-VERIFIED EXAMPLE】

下面代码与真实源码由 `tools/validate_documentation_api_consistency.ps1` 逐字符同步检查。

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

## 什么时候仍应使用模块

- 固定 DC：直接 DriverLib。
- 连续周期波：Wave Table/DDS → DAC DMA。
- 任意波回放：捕获/重采样 → DAC DMA。

连续输出包含 Timer/Event/DMA/DAC 协作，继续使用正式复杂模块，不要在循环里反复调用固定 DC 写函数。
