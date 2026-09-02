# Contest Reproduction Workflow

1. **录入原始题面**：放入 `ORIGINAL_PROBLEM.md`，保留编号和单位。
2. **拆解要求**：用 30 项 Problem Analysis，把输入、输出、范围、精度、时间分开。
3. **匹配现有 Recipe**：选择一条主链和一条有明确切换条件的备用链。
4. **选择模块**：从 Integration Module Index 选择真实公开 API，记录验证级别。
5. **计算参数**：得到 Fs、N、bin、时间、DMA、DDS、RAM、误差预算。
6. **复制 Contest Template**：复制到该题 `application/`，不复制模块源码。
7. **组合模块**：只写少量 config/glue/output；接口不适配先登记 issue。
8. **Build**：SysConfig generate → TI Arm Clang compile → final link → `.out/.map`。
9. **PC 模拟**：使用真值向量扫描 min/typ/max 与异常输入，不降低旧阈值。
10. **实板验证**：信号源/示波器/频谱仪/万用表；此前状态一律 PENDING_BOARD。
11. **按误差调整**：先定位时钟/VREF/前端/同步/算法来源，再改集中参数或正式模块。
12. **记录结果**：更新 Results、Known Limitations、Build/usage stats，状态按证据晋级。

## Gate

每一步都必须可追溯到文档或构建产物。若题目要求的能力不存在，先写入 `MISSING_CAPABILITIES.md`；只有它真正阻塞核心解法时才允许新增一个最小正式模块。任何 application 内几千行复制代码都视为架构失败。
