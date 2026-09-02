# Recipe Validation

检查 `measurement_recipes/*.md` 的 20 项执行契约、`DRAFT` 状态、算法文档相对链接、索引目标词和文档中的 `Signal*` 符号是否存在于正式算法源码。

```powershell
python .\MSPM0_Signal_Contest\tools\algorithm\recipe_validation\recipe_validation.py
```

它只做文档/API 静态检查，不把 Recipe 升级为 `PC_VERIFIED`，也不替代 `validate_direct_recipes.ps1` 的真实编译/真值回归。
