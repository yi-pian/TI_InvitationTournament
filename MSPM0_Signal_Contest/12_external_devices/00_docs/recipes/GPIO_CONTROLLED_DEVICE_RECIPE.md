# GPIO 控制器件接入 Recipe

适用于 CD4052/CD4053/CD4066B/MAX14752、继电器使能脚、硬件模式选择脚等。它们没有通信寄存器；正确做法是 SysConfig 配 GPIO，再直接调用 DriverLib，不为几行代码制造 `.c/.h` 驱动。

## 1. 先确认四件事

1. 控制输入高低阈值是否兼容 MSPM0 的 3.3 V GPIO；
2. 控制脚是否有反相含义，如 `/EN`、`INH`；
3. 上电默认状态是否安全；
4. 被切换的模拟电压是否位于模拟开关电源轨之间。

GPIO 能控制器件，不代表模拟信号范围也适合。尤其高压模拟开关，模拟端与控制端必须分别按手册检查。

## 2. SysConfig

1. 每个选择/使能脚添加为 GPIO Output。
2. 先设置安全初始值：一般先禁用，再设置地址，最后启用。
3. 给 SysConfig 实例和 pin 起可读名字，如 `MUX_S0`、`MUX_S1`、`MUX_EN`。
4. 保存后只使用 `ti_msp_dl_config.h` 生成的 `..._PORT` 和 `..._PIN` 宏。

## 3. 二进制地址控制

### 【比赛现场直接复制】

```c
static void mux_select(uint8_t channel)
{
    /* 先禁用，避免地址切换瞬间接到错误通道。 */
    DL_GPIO_setPins(GPIO_MUX_PORT, GPIO_MUX_INH_PIN);

    if ((channel & 0x01U) != 0U) {
        DL_GPIO_setPins(GPIO_MUX_PORT, GPIO_MUX_S0_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_MUX_PORT, GPIO_MUX_S0_PIN);
    }

    if ((channel & 0x02U) != 0U) {
        DL_GPIO_setPins(GPIO_MUX_PORT, GPIO_MUX_S1_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_MUX_PORT, GPIO_MUX_S1_PIN);
    }

    DL_GPIO_clearPins(GPIO_MUX_PORT, GPIO_MUX_INH_PIN);
}
```

三位地址器件再增加 `S2`。若 EN 高有效，就把禁用/启用电平反过来。

## 4. 单个开关控制

```c
DL_GPIO_setPins(GPIO_SW_PORT, GPIO_SW1_CTRL_PIN);    /* 接通 */
DL_GPIO_clearPins(GPIO_SW_PORT, GPIO_SW1_CTRL_PIN); /* 断开 */
```

CD4066B 是高电平接通；其他器件必须查真值表。

## 5. 验证

不要一开始就接敏感模拟链。先用万用表或小信号源验证每个通道的导通/断开，再测导通电阻、带宽、串扰和切换瞬态是否满足题目。

