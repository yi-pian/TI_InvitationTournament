# API Check

从选定 README/Example/Recipe/源码中提取 `Signal*` 引用，并与选定权威 `.c/.h` 根目录中的真实符号比对。它用于发现拼写错误、旧 API 残留和文档引用不存在的 API。

算法 Recipe 检查示例：

```powershell
python .\tools\api_check\api_check.py `
  --source-root ..\MSPM0_Signal_Contest `
  --reference-root ..\MSPM0_Signal_Contest\00_docs\measurement_recipes
```

本工具只证明符号在源码中存在，不证明参数顺序正确；写调用前仍须读取当前 `.h`，构建后仍须 final link。现有 `validate_documentation_api_consistency.ps1` 继续负责标记过的 compile-verified snippet 与正式示例一致性，两者不互相替代。

