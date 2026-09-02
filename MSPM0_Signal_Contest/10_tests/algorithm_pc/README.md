# 纯算法 PC 测试

这些测试不连接开发板，使用带真值的合成数据验证标准 C 算法。Windows 环境使用：

```powershell
cd 10_tests/algorithm_pc
gmake clean
gmake test
```

编译选项固定包含：

```text
-std=c11 -O2 -Wall -Wextra -Werror -pedantic
```

每项输出 Measured、Expected、Absolute Error、Relative Error、PASS/FAIL。只有目标能严格编译且全部比较 PASS，对应纯算法才标为 `PC_VERIFIED`；PC 测试不等于开发板、TI Arm Clang 或模拟链路已验证。

## 测试目标

| 可执行文件 | 内容 | 当前结果 |
|---|---|---:|
| test_first_batch | 转换、均值/统计、Vpp、RMS/AC RMS、RemoveDC、Clipping | 33 PASS |
| test_second_batch | 有/无 DC 过零、线性插值、多周期频率、异常输入 | 21 PASS |
| test_third_batch | MovingAverage、Median、MAD、Hampel、FIR、IIR | 40 PASS |
| test_fourth_batch | 四种窗、FFT、Magnitude、峰值、两种抛物线、增益修正 | 37 PASS |
| test_fifth_batch | MultiBin、BASIC/COMP THD、SNR、SFDR | 23 PASS |
| test_sixth_batch | ZeroCross/FFT/Correlation Phase、Correlation、Autocorrelation | 20 PASS |
| test_seventh_batch | 两种校准、RobustVPP/RMS、SineFit3/4、LockIn | 34 PASS |
| test_signal_vectors | 10 类合成信号发生器 | 26 PASS |
| 合计 | 完整回归 | 234 PASS / 0 FAIL |

## 基础正弦真值

| 参数 | 真值 |
|---|---:|
| sample_rate_hz | 100000 Hz |
| frequency_hz | 1000 Hz |
| amplitude_peak | 0.5 V |
| DC | 1.65 V |
| Vpp | 1.0 V |
| AC RMS | 0.3535533906 V |

## 标准测试信号库

`signal_test_vectors.c/.h` 提供固定参数、确定性噪声的：

- clean_sine
- sine_with_dc
- noisy_sine
- sine_with_harmonics
- square_wave
- triangle_wave
- impulse_noise
- two_tone
- burst
- clipped_sine

噪声用固定 32-bit LCG seed，保证每次回归可重复；它不是高质量随机源，也不进入 MCU 正式算法。

## 测试边界

测试的容差写在各 `test_*.c`。例如 Hann off-bin 线性抛物线案例真值 1037 Hz、测得约 1032.1666 Hz，明确展示该方法仍有约 4.833 Hz 的窗相关偏差。不要因为测试 PASS 就把容差内偏差说成零。

`build/` 是生成目录。运行 `gmake clean` 只删除列出的测试可执行文件，不删除源码。
