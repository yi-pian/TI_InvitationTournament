# 模拟电压控制器件接入 Recipe

适用于 AD603 等由控制电压决定增益/衰减/频率的器件。它们没有 SPI/I2C；MCU 通过内部 DAC、外置 DAC 或低通后的 PWM 产生控制电压。

## 1. 先把控制规律写成公式

从数据手册找到：

- 控制端电压允许范围；
- `Vcontrol -> gain/attenuation/frequency` 的传输关系；
- 控制端输入阻抗/偏置电流；
- 允许纹波与响应速度；
- 控制端是否允许超出器件电源轨。

如果目标量为 `y`，先建立 `Vctrl = f^-1(y)`，再由 DAC 满量程换算成 code。不要直接用“经验码值”。

## 2. DAC 控制链

```text
目标增益/参数
    -> 计算 Vctrl
    -> 限幅到安全范围
    -> DAC code
    -> DAC 输出
    -> 可选 RC/运放缓冲
    -> 器件控制端
```

### 【比赛现场直接复制】

```c
static uint16_t volts_to_dac_code(float volts, float dac_full_scale)
{
    if (volts < 0.0f) volts = 0.0f;
    if (volts > dac_full_scale) volts = dac_full_scale;
    return (uint16_t)((volts / dac_full_scale) * 4095.0f + 0.5f);
}

float control_v = requested_control_v;
uint16_t code = volts_to_dac_code(control_v, 3.3f);
/* 用项目中真实 DAC API 输出 code。 */
```

如果控制端需要负电压或高于 3.3 V，必须加模拟电平变换/运放，不能让 MSPM0 引脚硬顶。

## 3. SysConfig

若使用 MSPM0G3507 内部 DAC，在 SysConfig 添加 DAC、选择输出引脚和参考源；应用中调用工程当前生成配置对应的 DriverLib。若使用外置 DAC，则按该 DAC 的 SPI/I2C README 配置。模拟控制器件本身不需要“通信 SysConfig”。

## 4. 校准与验证

先只输出三个控制点：最小、中间、最大。用万用表测实际 Vctrl，再测受控量。最后可做多点标定表或线性修正。数字码正确而实际控制量不对，通常是模拟输出范围、负载、缓冲或器件传输函数问题，不是 SPI 问题。

