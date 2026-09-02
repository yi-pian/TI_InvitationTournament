# oscilloscope

> **LEGACY REFERENCE**：本目录是早期纯分析 glue，不是当前推荐的新 Application 起点；新工程请参考 `signal_meter` 并链接独立算法仓库的正式算法。

## 本 Application 的实现层级

| 功能 | 类型 | 当前实现 |
|---|---|---|
| 硬件采集 | B Complex Hardware Module | 本目录未包含 |
| 均值/极值/RMS | C Algorithm Module | 早期调用示例；不要把旧算法目录加入新工程 |
| 完整组合 | E Application Reference | 仅供理解薄 glue，不代表当前 BUILD_VERIFIED 工程 |

## 1. 模块作用

组合均值、极值、Vpp、总 RMS 和 AC RMS。

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 `signal_result_t`；不通过隐藏全局变量传递数据。

## 4. 依赖

`signal_status.h`。

## 5. SysConfig 设置

无寄存器、引脚或 SysConfig 依赖，可在 PC 上独立测试。

## 6. 初始化方法

模块不做隐式全局初始化。包含 `signal_oscilloscope.h`，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

`SignalOscilloscope_Analyze`、`SignalOscilloscope_GetModuleStatus`。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 `.c`。

## 9. 与其他模块如何连接

通过 `signal_types.h` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "signal_oscilloscope.h"

/* 按头文件准备输入/输出，调用上述主 API，并检查 signal_result_t。 */
~~~

纳入 `10_tests/pc` 全库构建；关键数值路径还应按题目范围补充向量和误差上限。

## 11. 常见错误

空指针、零长度、capacity 小于 count、单位混用、把配置采样率当物理实测值，以及复用仍在使用的工作区。

## 12. RAM 占用

模块内动态分配 0；数组/工作区由调用者提供，具体大小由 API 的 count/capacity 决定。

## 13. Flash 占用

无固定常量：取决于编译优化、是否链入数学库和死代码删除。已纳入整库链接检查；比赛应用以 CCS 生成的 .map 为最终数据。

## 14. CPU 计算量估计

函数为同步确定性处理；硬件回调的中断上下文只做最小状态更新，重计算放在主循环。

## 15. 当前验证状态

`MODULE_STATUS_BUILD_VERIFIED`。该状态只表示现有证据等级，不等于完整比赛场景已经验证。

## 16. 以后实板验证步骤

Hardware validation: PENDING。先用 PC 已知向量和误差上限验证，再接入已验证的采集/发生链，覆盖题目最小值、典型值和最大值后才可升级。

不使用时，从工程移除本目录 .c 及上层引用；若有平台外设适配，再从 SysConfig 删除对应实例。
