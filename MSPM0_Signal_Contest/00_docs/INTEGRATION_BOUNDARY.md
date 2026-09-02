# 独立工作区与主工程的合入边界

## 当前规则

1. 所有算法新文件只写入同级目录 `MSPM0_Signal_Contest/`。
2. `MSPM0_Signal_Contest/` 视为只读参考，不在这里创建、覆盖、移动或删除文件。
3. 硬件接口若与算法契约不同，只在 `HARDWARE_ALGORITHM_CONTRACT.md` 记录 Expected Interface。
4. 未经用户明确要求，不把本目录任何文件复制回主工程。

## 将来手工合入时

- 先比较同名文件，不用覆盖式复制。
- 只合入选定算法模块的 `.c/.h/README/MODULE_CARD`。
- 将 `03_measurement/common/` 加入编译器 include path。
- 用主工程公开 ADC 接口在应用层做 Adapter；不要把算法塞进采集驱动。
- 重新做 TI Arm Clang build；PC_VERIFIED 不能代替 MCU build 或 BOARD_VERIFIED。
