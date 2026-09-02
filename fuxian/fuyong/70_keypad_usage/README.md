# 70_keypad_usage

## 推荐复制函数

`ReadKeypad()` 后依次调用 `HandlePageSwitch()`、`HandleNumberInput()`、`HandleParameterAdjust()`。COPY 区已覆盖这四个完整函数；直接数字输入由 `signal_keypad_number_input` 管理，变量含义固定为 `current_page`、`number_input`、`adjustable_value`。

## 1. 这个工程干什么

用现有 4×4 矩阵键盘模块读取新按键，并展示页切换、数字缓存和参数加减。

## 2. 这个工程包含哪些子功能

| 我要做什么 | COPY 区域 |
|---|---|
| 读取按键 | `KEY_READ` |
| 切换页面 | `PAGE_SWITCH` |
| 数字输入缓存 | `NUMBER_INPUT` |
| 参数步进调整 | `PARAMETER_ADJUST` |

## 3. 输入

矩阵键盘硬件和 `key`；`current_page` 是统一 UI 页变量。

## 4. 输出

`key`、`current_page`、`number_input`、`adjustable_value`。

## 5. 公共数据链

`keypad scan → key → page/input/parameter state`。

## 6. 功能与 COPY 区对应表

所有功能都依赖 `KEY_READ`；后面三个区仅消费 `key`。

## 7. 使用的模块

`signal_matrix_keypad_4x4`；调用依据：restored example04 的 `SysTick_Handler` 与真实 `ReadNewSymbol` API。

## 8. SysConfig / 引脚

复制 restored example04 keypad GPIO 配置，不改行列定义。

## 9. main.c 流程

读取新按键后，依次示范页面、数字、参数处理。

## 10. 每个 COPY 区说明

数字输入支持 `0~9`、`*` 小数点、`D` 删除、`C` 取消和 `#` 确认；确认后可直接读取 `float`。题目代码仍需根据频率、电压或百分数的真实单位检查范围。

## 11. 如何复制到新工程

复制 keypad 模块、`KEY_READ` 和需要的状态块以及原 SysConfig。

## 12. 可调参数

页数、输入长度、步长、参数上下限。

## 13. 常见错误

按键扫描不应被长 FFT/SPI 阻塞；通用模块负责确认与十进制解析，题目代码负责单位、上下限和确认后的硬件更新。

独立复制直接数字输入时，还要复制：`signal_keypad_number_input.c/.h`。

## 14. 本工程没有做什么

不绑定 TFT，不实现完整编辑状态机。

## 15. Build 状态

待统一 SysConfig 生成和 CCS Compile/Link 审计；实板 `NOT_RUN`。
