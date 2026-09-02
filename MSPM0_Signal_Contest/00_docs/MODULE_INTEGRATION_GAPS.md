# Module Integration Gaps

这里只列截至 2026-08-11 仍不能仅靠正式 README/adapter 完成 MSPM0G3507 手工拼装的模块。已闭环项不放在这里；当前 94 模块入口见 [MODULE_CATALOG.md](MODULE_CATALOG.md)，旧 callback 的 93 模块闭环快照见 [MODULE_COMPOSABILITY_AUDIT.md](MODULE_COMPOSABILITY_AUDIT.md)。

## 当前 Gap

### P0 — BSP OPA

- Module：`01_bsp/opa`
- 状态：`API_GAP`
- 缺什么：公开 API 缺 MSPM0G3507 `DL_OPA_Config` 必需的 PSEL/NSEL/MSEL、离散 gain、output pin、chopping、GBW 等选择。
- 为什么阻塞：任意 `resistor_feedback_ohm/resistor_input_ohm/bias_voltage_v` 不能唯一映射到片上 OPA 的 -1/-3/-7/-15/-31 或 +2/+4/+8/+16/+32 与真实 MUX；薄 adapter 会掩盖错误硬件拓扑。
- 解决方案：先迁移正式 BSP API，使其显式表达 hardware topology 与离散配置；再为 buffer/inverting/non-inverting 各建立 SysConfig Profile、adapter 和 full-link/board test。不要在 Application 私写 callback。

### P0 — BSP GPAMP

- Module：`01_bsp/gpamp`
- 状态：`API_GAP`
- 缺什么：公开 API 只有 `requested_gain/bias_voltage_v`，缺 `DL_GPAMP_Config` 的 PSEL/NSEL、output、RRI、chopping mode/frequency。
- 为什么阻塞：MSPM0G3507 GPAMP DriverLib 并没有一个可由任意 requested gain 直接设置的通用连续 gain 参数，无法诚实转换。
- 解决方案：按真实 GPAMP 能力重定义/迁移正式 config；完成具体 pin 的 SysConfig、adapter、minimum full link 后再升级 READY。

### P0 — GPAMP Buffer / GPAMP Gain

- Module：`07_signal_frontend/gpamp_buffer`、`07_signal_frontend/gpamp_gain`
- 状态：`API_GAP`
- 缺什么：继承 BSP GPAMP 的真实硬件字段与 adapter。
- 为什么阻塞：模块只能生成抽象 request，不能走到 `DL_GPAMP_init()`、MUX 与物理引脚。
- 解决方案：先解决 BSP GPAMP；上层模块只复用正式 BSP，不另写一套。

### P0 — OPA Buffer / OPA Inverting / OPA Non-inverting PGA

- Module：`07_signal_frontend/opa_buffer`、`07_signal_frontend/opa_inverting`、`07_signal_frontend/opa_noninverting_pga`
- 状态：`API_GAP`
- 缺什么：继承 BSP OPA 的 topology/MUX/discrete gain adapter 与 SysConfig Profile。
- 为什么阻塞：软件电阻计算结果不等于 MSPM0G3507 片上离散 OPA 配置，无法保证引脚、极性或增益正确。
- 解决方案：BSP API 迁移后，让这些模块明确选择可支持的离散 topology；不支持的任意电阻网络继续只作为外部模拟电路计算，不假装可配置片上 OPA。

## 不属于 Gap 的相邻模块

- `OPA DAC Bias` 与 `OPA To ADC` 是纯软件电压/范围预算，能够独立完成输入→API→输出，所以为 READY；它们不代表 OPA 已配置。
- Comparator 已有 DAC8 threshold、0/10/20/30 mV hysteresis 与 polarity 的正式 adapter，不再是 Platform Gap。
- ADC Continuous 的 frame callback 是业务 consumer，不是硬件 glue；README 已给最小签名。

## 修复原则

1. 先修改唯一正式 BSP API，再修改上层 frontend；
2. 不在 Application 复制 DriverLib glue；
3. 必须有明确 SysConfig Profile 和 generated macro；
4. 必须完成 SysConfig、全部 compile、final link；
5. 上板前只能写 BUILD_VERIFIED，不能写 BOARD_VERIFIED。
