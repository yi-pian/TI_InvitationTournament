# Offline Module Assembly Workflow：四步复制拼装

仅在你已经确定要使用真正模块时执行。GPIO、固定 DAC、单点 ADC、简单 blocking UART/SPI 等直接走 SysConfig + TI DriverLib。

简单算法也不走模块复制流程：Mean/Vpp/RMS/AC RMS/ADC To Voltage/Remove DC/普通 Peak/Multi-Cycle Average 直接打开本仓库的 `00_docs/SIGNAL_ALGORITHM_COOKBOOK.md`，复制对应 Recipe。

## STEP 1：确定模块

你已经自己选好模块，例如：

```text
ADC DMA -> ADC To Voltage Recipe -> VPP Recipe
```

如果还没选好，去看 `MODULE_SELECTION_GUIDE.md`，不要在这里填写表格。

## STEP 2：逐个看 README

对每个真正的 Level B/C 模块只确认五件事：

1. README 要复制哪些 `.c/.h/.inc`；
2. 是否要改 SysConfig/Pin；
3. `main.c` 顶部复制什么；
4. 初始化/处理代码放哪里；
5. 最常改哪些题目参数。

README 已列出的路径、输入输出和依赖无需重新抄表。

## STEP 3：逐个复制并 Build

把 README 明列的文件复制到母版 `modules/`，在 CCS 中 Refresh，确认 `.c` 参与 Build，然后粘贴 README 代码。

```text
复制 ADC DMA -> Build PASS
-> 复制 ADC To Voltage -> Build PASS
-> 复制 VPP -> Build PASS
```

算法通常不需要 SysConfig；硬件模块只有 README 写明需要时才改 `.syscfg`。不要手改生成的 `ti_msp_dl_config.c/.h`。

若接口不清楚才查 `MODULE_INTERFACE_MATRIX.md`；参数位置不清楚才查 `PARAMETER_MODIFY_GUIDE.md`；SysConfig 不清楚才查 `SYSCONFIG_MODIFY_GUIDE.md`。它们是工具书，不是必走流程。

## STEP 4：整体修改与验证

全部模块加入后统一检查：

- Fs、N、FFT N；
- VREF、前端 scale/offset；
- threshold、window、backend；
- Pin、Timer、DMA、Event、IRQ 是否冲突；
- 大 Buffer 是否为 static，以及 SRAM 是否够。

最后执行 `Clean -> Build -> 上板最小验证 -> 完整功能验证`。

比赛工程允许冻结复制件。只需在 `COPIED_MODULES.md` 记录模块、原始路径、日期和本题修改；不需要维持 Linked Source 或仓库根变量。
