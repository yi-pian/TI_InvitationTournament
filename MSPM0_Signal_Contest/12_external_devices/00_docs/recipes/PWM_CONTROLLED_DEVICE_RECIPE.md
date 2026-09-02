# PWM 控制器件接入 Recipe

适用于可直接接受 PWM 的功率/亮度/速度控制，或经 RC 低通后需要一个慢速模拟控制电压的器件。

## 1. 两种用途不要混淆

- **数字 PWM 控制**：器件读取占空比，高/低电平直接进入控制脚。
- **PWM + RC 模拟电压**：低通后近似 `Vavg = duty × Vlogic`，有纹波和响应时间。

精密 PGA/VGA 控制优先真正 DAC；只有精度、纹波和响应都允许时才使用 PWM+RC。

## 2. SysConfig

1. 添加 Timer，选择周期/计数模式并启用一个 Capture Compare 输出。
2. 选择 PWM 输出引脚。
3. 设定 Timer 时钟、周期值和初始比较值。
4. 先选便于验证的频率，例如 10 kHz；再按器件带宽和 RC 设计调整。
5. 保存后在 `ti_msp_dl_config.h` 中确认 Timer instance、CC index 和 load value 宏。

## 3. 占空比换算

### 【比赛现场直接复制】

```c
static uint32_t duty_permille_to_compare(uint16_t duty_permille,
                                         uint32_t period_counts)
{
    if (duty_permille > 1000U) duty_permille = 1000U;
    return (period_counts * duty_permille) / 1000U;
}
```

将返回值写入 SysConfig 所配置 Timer/CC 的比较寄存器。MSPM0 Timer 的具体 DriverLib 更新函数要和当前定时器类型及 PWM 输出模式一致；不要照搬另一个 Timer 实例的宏。先查看该工程生成的初始化和 SDK 对应 Timer 示例。

## 4. RC 低通估算

截止频率 `fc = 1/(2*pi*R*C)`。为了压低 PWM 纹波，`fc` 通常显著低于 PWM 频率；但 `fc` 越低，控制响应越慢。先确定允许的响应时间和纹波，再选 R/C，不存在一个通用最佳值。

## 5. 验证顺序

1. 示波器看 MCU 管脚频率与占空比；2. 加 RC 后测平均电压与纹波；3. 接入受控器件后再次测量；4. 测 0%、50%、100% 和题目常用点。若负载改变导致电压漂移，增加缓冲或改用 DAC。

