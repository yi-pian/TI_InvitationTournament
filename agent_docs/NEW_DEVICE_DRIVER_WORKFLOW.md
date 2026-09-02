# 陌生器件 Driver 工作流

适用于 `FOREIGN_DRIVER_PORT`：比赛临时出现的新 ADC/DAC/DDS/传感器/PGA/显示控制器。目标是形成可交付的 MSPM0G3507 器件模块，不接管用户母版。

## 四类真相

- Datasheet = Device Truth：寄存器、命令、协议、电气、时序和限制。
- STM32/Arduino/Vendor Code = Implementation Reference：帮助理解流程，不是 MSPM0 API 真相。
- MSPM0 Skill、当前 SDK Header、DriverLib = Platform Truth：GPIO/SPI/I2C/IRQ/DMA/delay 的实际实现依据。
- Repository Card、README、public header = Project Truth：命名、状态、公共契约和目录规则。

必须先把参考代码分成 `device protocol logic` 与 `source platform code`。迁移寄存器/帧/状态机/时序语义，丢弃 STM32 HAL、Arduino Wire/SPI、启动文件、平台 ISR 和工程宏，再用已核实的 MSPM0G3507 平台接口重建传输层。

```text
确认 exact part number
-> 找 exact datasheet/原理图/模块丝印
-> 供电与绝对最大值
-> MCU/module logic level 与电平转换
-> interface 与 Pin
-> SPI/I2C mode、bit order、速率、地址
-> 时序/CS/DRDY/CONV/RESET/ENABLE
-> 上电与 reset sequence
-> Read ID/status（若器件提供）
-> 区分 device protocol 与 source platform code
-> 映射到当前 MSPM0 DriverLib/回调
-> 在隔离/候选目录生成 device_xxx.c/.h/README.md
-> isolated API/compile test（可用时）
-> 停止，交给用户放入母版、配置 SysConfig 和板测
```

验证等级只允许按真实证据递进：`DRAFT -> COMPILE_VERIFIED -> BUILD_VERIFIED -> BOARD_VERIFIED`。创建文件不能自动越级；Compile 不能冒充 Link，Build 不能冒充 Board。

## 最小提交面

优先只完成：`Init`、`Reset`、`ReadID/Status`、最简单输出或单次采样，以及 README/接线/最小例/验证记录。器件没有 ID/status 时明确写“datasheet 未提供”，不要发明寄存器。默认交付面固定为 `device_xxx.c`、`device_xxx.h`、`README.md`。

新驱动先放在任务隔离目录或 `12_external_devices/_candidates/<exact-part>/`，不得直接标为正式推荐模块。没有用户母版、主程序和板测时，Build/Board 如实保持 `NOT_RUN`；文件生成成功不能自动晋级。

禁止看完 Datasheet 一次性生成巨大完整 Driver。每增加一个能力先用已知输入/输出验证；不要在 Driver 内混入 FFT、UI、自动量程或赛题状态机。

## 搜索与复用

先查 `MSPM0_Signal_Contest/12_external_devices/README.md` 和 exact device 目录，区分 `exact driver / generic tutorial / documentation only / datasheet required`。已有 blocking bus、platform callback 或同协议 exact 模式时复用，不复制第二套底层总线。

## README 最低内容

exact part、Datasheet 版本/链接或文件、供电/logic、接线、真实 API、Init 顺序、阻塞/超时、buffer/单位、文件清单、错误排查、验证等级与尚未验证项，并明确列出：

- 所需 SysConfig resource/instance；
- SPI/I2C mode、bit order、address、bitrate；
- GPIO/CS/RESET/ENABLE/DRDY/CONV；
- IRQ/DMA 是否必需以及触发条件；
- clock、delay、上电与通信时序；
- 初始化方法和调用示例。

调用示例只展示模块 API，不修改或交付用户的 `main.c`。默认禁止修改 `main.c`、`app.*`、`.syscfg`、生成文件和现有母版；这些属于用户后续集成流程。
# SysConfig Contract deliverable

In `FOREIGN_DRIVER_PORT`, also deliver `device_xxx.sysconfig_contract.yaml`.
It declares `sysconfig.class`: normally `required` for a real hardware driver,
`none` for a pure software helper, and `conditional` only with explicit
`condition -> required_resources` mappings. A required contract records exact
Datasheet evidence plus `required_resources`, instance requirements, bus mode,
clock/rate, GPIO roles (not guessed physical pins), IRQ/DMA/timer/event/PinMux,
timing, `required_generated_symbols`, dependencies, constraints, and
`user_selectable_fields`. Unresolved material facts remain
`contract_incomplete: true`; they are never guessed. This sidecar is consumed by
`SYSCONFIG_COMPOSE`; it never edits the user project.
