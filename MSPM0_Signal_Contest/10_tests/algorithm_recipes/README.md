# Direct Recipe 验证

运行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ..\..\tools\algorithm\validate_direct_recipes.ps1
```

脚本会：

1. 检查 `00_docs/recipes/*.md` 都具备固定九节新手结构；
2. 从 `DIRECT_COPY` 标记中抽取比赛现场代码，而不是测试另一份手抄副本；
3. 用 PC GCC 严格编译并运行 14 项真值检查；
4. 若本机存在 TI Arm Clang，再以 Cortex-M0+ 参数执行源码编译；
5. 把结果写入 `build/direct_recipe_results.json`。

这只能证明 Recipe 代码可编译并通过当前 PC 真值，不代表 ADC、VREF、模拟前端或开发板已经验证。
