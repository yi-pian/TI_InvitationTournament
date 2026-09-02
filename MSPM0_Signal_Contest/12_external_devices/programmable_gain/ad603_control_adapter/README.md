# AD603 类模拟控制 VGA 适配教程

README 类型：`GENERIC_TUTORIAL`  
状态：`DOCUMENTATION_ONLY / DATASHEET_REQUIRED`；没有 AD603 正式 Driver，也没有锁定外围电路。

## 它是什么

这类 VGA/PGA 的增益由一个模拟控制电压决定。MSPM0 不能“发一个增益寄存器”直接完成控制，而要用内部 DAC、外置 DAC 或 PWM+低通产生控制电压，再由真实电路把它变成增益。

```text
target_gain_db → 校准表/官方公式 → control_voltage_v → DAC → 缓冲/钳位 → VGA → 实际 gain
```

## 拿到实物先确认

完整料号、供电、输入/输出共模和幅度、控制电压范围、增益斜率/截距、模式/绑带、带宽、输出负载、外围运放、控制电压滤波与保护。上述内容由芯片 datasheet 和你的原理图共同决定。

## MSPM0 / SysConfig / 接线

- 优先：配置 DAC 输出；若内部 DAC 范围/分辨率不够，再选外置 DAC。
- 备选：Timer PWM + RC 低通，但纹波和响应时间必须实测。
- 控制电压通常要缓冲、限幅；VGA 信号输入/输出不是 GPIO。

## 最小 Bring-Up

1. 输入固定小幅正弦，确保所有增益档都不会把输出推到削顶。
2. 产生三个安全控制电压，逐点测实际增益和带宽。
3. 检查输出偏置、噪声、振荡和削顶。
4. 建立 `gain_db ↔ control_voltage_v` 实测表。
5. 开环稳定后才做 AGC；AGC 还需限制步长和稳定等待时间。

## Generic main 框架

```c
#include <stdbool.h>
#include "ti_msp_dl_config.h"

bool TODO_PLATFORM_SetControlVoltage(float voltage_v);
float TODO_CALIBRATION_GainDbToVoltage(float gain_db);

int main(void)
{
    float control_v;
    SYSCFG_DL_init();
    control_v = TODO_CALIBRATION_GainDbToVoltage(0.0f);
    if (!TODO_PLATFORM_SetControlVoltage(control_v)) {
        __BKPT(0);
    }
    while (1) { __WFI(); }
}
```

## 比赛最常改参数

目标增益、控制电压上下限、校准斜率/表、DAC VREF/code、切换步长、稳定时间和输入过载保护阈值。

## 如果换成另一个模拟控制 VGA

可复用“增益目标 → 控制电压 → 实测校准”的结构；必须重查控制范围、公式、供电、信号摆幅和外围。参考 [模拟电压控制器件 Recipe](../../00_docs/recipes/ANALOG_VOLTAGE_CONTROLLED_DEVICE_RECIPE.md)。若是数字 SPI PGA，则查看 [PGA113 Exact Device Guide](../pga113/README.md)，不要混成同一种接口。

