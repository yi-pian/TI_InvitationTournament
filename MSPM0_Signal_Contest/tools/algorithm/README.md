# Algorithm Tools

本目录是算法 Recipe 验证、标定拟合和机器校验工具的唯一正式位置。

这里只放能重复运行并有自测的算法工具：

| 工具 | 用途 | 当前验证 |
|---|---|---|
| `validate_direct_recipes.ps1` | 提取 Direct Copy 代码，GCC 真值测试并做 TI Arm Clang compile | PC 14/14 PASS、TI compile PASS |
| `recipe_validation/recipe_validation.py` | 22 条 Measurement Recipe 的 20 项契约、链接、目标词、API 符号检查 | `PASS`，Board `NOT_RUN` |
| `calibration_fit/calibration_fit.py` | CSV x,y 模型选择、误差指标、生成 `calibration.h` | 线性 fixture `PC_VERIFIED`；GCC run 与 TI compile PASS |

以后新增工具必须记录：输入单位、输出 C 数组格式、设计参数、依赖版本、命令示例和与 C 算法的回归测试。工具输出的 `PC_VERIFIED` 不会自动升级算法、校准或硬件状态。
