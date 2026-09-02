# Build、Debug 与验证规则

只有实施/构建/Debug 时读取。

## 修改前

确认目标 `.projectspec/.syscfg/.project/.cproject/CMakeLists/uvprojx`、device、SDK、compiler、SysConfig、Application，以及新母版冻结复制或旧 Application linked-source 路线。两条路线不得混用。

## 修改后

```text
SysConfig generate
-> 检查 warning/error 和生成 header
-> compile 全部 translation units
-> final link
-> 读 map/size
-> 运行相关 PC/Copy Assembly/Integration tests
-> 有板时分层验证
```

每加入一个模块 Build 一次，不等所有模块堆完。运行 `MSPM0_Signal_Contest/tools/` 脚本前先读参数、输出目录和目标范围；不得改 build/generated 产物让测试“通过”。

## Debug 顺序

1. Include path 和目标 `.c` 是否参与编译。
2. API/宏/参数类型是否与当前 `.h` 相同。
3. SysConfig instance/Pin/IRQ/DMA/Event 与初始化顺序。
4. undefined/duplicate symbol 和错误 source list。
5. DMA request/width/increment/repeat/完成中断。
6. Timer clock/period/Event/Capture 方向与 overflow。
7. ISR 名、清 flag、priority、唯一 owner。
8. Backend/条件编译/CMSIS/IQMath include/library。
9. RAM、stack、大数组和 buffer 生命周期。
10. 供电、共地、连线、logic、器件模式与测试方法。

一次只修与真实首个根因直接相关的问题；禁止空 stub、禁用 `-Werror`、排除失败源码或大重构掩盖错误。

## 状态词

- `PC_VERIFIED`：PC 真值/单元测试真实通过。
- `COPY_READY`：隔离母版复制件完成 SysConfig/compile/full link。
- `BUILD_VERIFIED`：目标完整工程已 final link。
- `BOARD_VERIFIED`：有真实开发板、条件和结果记录。
- `CONTEST_VERIFIED`：有真实赛题复现与指标证据。
- `NOT_RUN`：当前未执行。

单 `.c` 语法检查、旧日志、别的工程、静态阅读或“看起来能编译”都不能写 `BUILD_VERIFIED`；Build 也不能外推 Board/Contest。

