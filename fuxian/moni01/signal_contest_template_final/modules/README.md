# modules

把模块 README 列出的 `.c/.h/.inc` 和少量公共头文件复制到本目录。

本母版 Include Path 已包含 `${PROJECT_ROOT}/modules`。复制后：

1. 在 CCS Project Explorer 右键工程并 Refresh；
2. 确认新 `.c` 文件没有 Exclude from Build；
3. 在 `main.c` include 模块主头文件；
4. 立即 Build 一次。

不要把正式仓库的 Platform/Adapter 整目录搬进来；只复制当前模块 README 明列的文件。
