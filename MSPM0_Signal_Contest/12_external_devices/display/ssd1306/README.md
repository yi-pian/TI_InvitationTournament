# SSD1306 0.96 寸 4-pin I2C OLED

README 类型：`EXACT_DEVICE_GUIDE / COMPILE_VERIFIED_DRIVER / COPY_READY`

验证状态：`DOC_VERIFIED`、`CODE_COMPILE_VERIFIED`、`COPY_READY`；尚未连接实物，不能标 `BOARD_VERIFIED`。

明确模块：深圳市旺泓科技 `WH-X096-2864KSWEG01-A4`，规格书 Rev 1.0（2024-05-08）。本地原始资料位于仓库根目录 `0.96oled/`，其中 STM32F103 软件 I2C 例程只作初始化序列与字库参考，不是 MSPM0 正式源码。

## 1. 已核对规格

| 项目 | 确认值 |
|---|---|
| 显示 | 0.96 寸白色单色 OLED，128×64，1/64 duty |
| 控制器 | SSD1306 |
| 模块接口 | 4-pin I2C：GND、VCC、SCL、SDA；无外露 RESET |
| 模块供电 | 规格图标称 3.3 V；直接接 LP-MSPM0G3507 的 3.3 V |
| I/O 电平 | SCL/SDA 规格上限 3.3 V，禁止上拉到 5 V |
| 地址 | 默认 7-bit `0x3C`；移动板上电阻后为 `0x3D` |
| 上拉 | 模块原理图包含 4.7 kΩ SCL/SDA 上拉至 VCC |
| I2C 速度 | 规格最短周期 2.5 us，即上限 400 kHz；首次建议 100 kHz |
| 运行电流 | 规格给出全亮典型约 25.6 mA、最大 32 mA（内部 DC/DC 条件） |
| framebuffer | 128 × 64 / 8 = 1024 B SRAM |

屏幕规格书的绝对最大值表虽写 VCC 3～5 V，但同一机械图明确此成品模块为 3.3 V，且 SCL/SDA 逻辑不超过 3.3 V；本库固定推荐整板 3.3 V 供电。

## 2. 正式文件和依赖

比赛工程复制：

- `signal_status.h`
- `ssd1306.c`
- `ssd1306.h`
- `ssd1306_font_6x8.inc`
- `ssd1306_mspm0_i2c.c`
- `ssd1306_mspm0_i2c.h`
- `ssd1306_mspm0g3507.c`
- `ssd1306_mspm0g3507.h`
- `mspm0_blocking_bus.c`
- `mspm0_blocking_bus.h`
- `README_MINIMAL_EXAMPLE.c`（复制或粘贴成应用 `main.c`）

核心 `ssd1306.c/.h` 不依赖 TI SDK，负责初始化、显示控制、1024 B framebuffer 刷新、像素、直线和 6×8 ASCII。MSPM0 适配层负责把控制字节 `0x00/0x40` 与命令/数据交给硬件 I2C。

原 STM32 代码没有原样复制：它硬编码 GPIOG12/GPIOD5、软件 I2C 不检查 ACK，且原 `OLED_DrawLine()` 的负向 Y 分支存在明显变量错误。正式核心使用可注入传输函数并修正为标准 Bresenham 直线。

## 3. 接线

推荐使用本库独立复制测试的 I2C profile：I2C Controller，PB2=SCL、PB3=SDA。

| OLED Pin | LP-MSPM0G3507 | SysConfig |
|---|---|---|
| GND | GND | 无 |
| VCC | 3.3 V | 无 |
| SCL | PB2 | I2C SCL |
| SDA | PB3 | I2C SDA |

如果应用已占用 PB2/PB3，可在 SysConfig 换到该 I2C 实例支持的其他 Pin，并以生成的 `ti_msp_dl_config.h` 为准。模块板已有 4.7 kΩ 上拉，组合其他 I2C 板时先计算并联上拉，不要无条件重复焊接。

## 4. SysConfig

1. 添加 `I2C Controller`，实例名设为 `I2C`。
2. `Enable Controller`，首次配置 100 kHz；稳定后可升到 400 kHz。
3. 分配 SCL/SDA（独立 profile 使用 PB2/PB3）。
4. 确认开漏线外部上拉到 3.3 V，运行 `SYSCFG_DL_init()` 后再初始化显示。

不要手改生成的 `ti_msp_dl_config.c/.h`。适配层默认以最多 7 个 payload 字节加 1 个 control byte 分包，适配公共 helper 的 8-byte TX FIFO 边界，因此整屏刷新是阻塞操作。100 kHz 下仅总线净传输约 96 ms，400 kHz 下约 24 ms；测量主循环应降低 UI 刷新率，避免每次采样都刷屏。

## 5. 最小使用

完整代码见 `README_MINIMAL_EXAMPLE.c`。关键结构如下；平台入口会自动绑定当前 SysConfig 生成的 `I2C_INST`：

```c
static uint8_t g_framebuffer[SSD1306_FRAMEBUFFER_SIZE];
static ssd1306_t g_display;
static ssd1306_mspm0_i2c_t g_bus;

SSD1306_ClearBuffer(g_framebuffer);
SSD1306_DrawString6x8(g_framebuffer, 0, 0, "MSPM0 OLED", true);
if (SignalSSD1306_MSPM0_Init(&g_display, &g_bus,
        SSD1306_I2C_ADDRESS_DEFAULT, false) == SSD1306_STATUS_OK) {
    (void) SSD1306_Update(&g_display, g_framebuffer);
}
```

所有大数组由应用静态提供，驱动不使用 `malloc`。`SSD1306_Update()` 失败时返回 `SSD1306_STATUS_IO_ERROR`，应用可记录错误并尝试重新初始化，而不要在 ISR 中调用全屏刷新。

## 6. API 速查

| API | 作用 |
|---|---|
| `SSD1306_Init()` | 执行厂商 128×64 初始化并点亮屏幕 |
| `SSD1306_Update()` | 把 1024 B framebuffer 按 8 page 刷到屏幕 |
| `SSD1306_ClearBuffer()` | 只清 RAM，不自动发 I2C |
| `SSD1306_DrawPixel()` | 画/清单点，越界返回 `false` |
| `SSD1306_DrawLine()` | 支持任意方向并裁掉屏外像素 |
| `SSD1306_DrawChar6x8()` / `DrawString6x8()` | 画 ASCII 0x20～0x7E |
| `SSD1306_DrawRect()` / `DrawBitmap()` | 在 framebuffer 中画矩形边框或单色位图 |
| `SignalSSD1306_MSPM0_Init()` | 绑定当前 SysConfig 的 `I2C_INST` 后初始化 |
| `SSD1306_SetContrast()` | 修改 0～255 对比度 |
| `SSD1306_SetInverse()` | 正常/反显 |
| `SSD1306_SetRotation()` | 正常/180°方向 |
| `SSD1306_SetDisplayOn()` | 显示开关；不删除 framebuffer |

## 7. Bring-Up 与验收

1. 断电接线，先量 VCC 和上拉电压均为 3.3 V。
2. 以 100 kHz 扫描 `0x3C`；若无 ACK，再检查地址电阻是否改为 `0x3D`。
3. 跑最小例，应出现 `MSPM0 OLED` 和一条水平线。
4. 依次测试四角点、正反斜线、字符边界、反显、旋转和连续上电 10 次。
5. 逻辑分析仪确认 7-bit 地址、control byte、ACK、page 命令和数据顺序。
6. 测量任务运行时降低显示刷新率，确认 ADC/DMA 时序未被长时间 blocking refresh 干扰。

当前只完成代码与隔离构建；上述真实接线和显示结果必须上板后才能升级 `BOARD_VERIFIED`。

## 8. 常见问题

- 把原例程的 `0x78` 当 7-bit 地址：`0x78` 是含 R/W 位的 8-bit 写地址，DriverLib 应传 `0x3C`。
- SCL/SDA 上拉到 5 V：超过规格逻辑上限，也会危及 MSPM0。
- 把其他 5 V OLED 板的接法直接套过来：本模块原理图虽带 3.3 V 稳压并把上拉接到稳压后的 VCC，但本规格机械图标称 3.3 V；本库接线基线统一使用 3.3 V。
- 地址没 ACK 仍继续发送：正式适配层检查公共总线 helper 的错误/超时返回。
- 在采样 ISR 中刷新屏幕：阻塞 I2C 会造成不可接受的 ISR 延迟。
- 只清 framebuffer 不调用 `SSD1306_Update()`：屏幕不会变化。
- 长期显示固定高亮内容：OLED 可能残影，UI 应适当降低对比度并更新内容。

## 9. 资源与证据

- 应用 framebuffer：1024 B SRAM。
- 字库：95 × 6 = 570 B 只读数据。
- 无动态内存；核心对象只保存配置与初始化状态。
- TI Arm Clang 5.1.1.LTS：`-std=c11 -Wall -Wextra -Werror` 目标源码编译通过。
- 独立 SysConfig generate、compile、full link 已通过；详细 Flash/SRAM 见 `00_docs/COPY_ASSEMBLY_READINESS.md`。
- Board：`NOT_RUN`。

规格依据为本地 `0.96oled/WH-096-4pin-I2C-SSD1306.pdf`。需要机械尺寸、板上地址电阻位置或原理图时直接打开该 PDF；原始 STM32 工程保留在资料文件夹，不再作为正式入口。
