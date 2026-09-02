# Calibration Fit

读取带 `x,y` 表头的 CSV，尝试一次/二次/三次多项式、指数、对数、分段线性和全 LUT 插值，输出：

- R²、RMSE、最大绝对误差、最大相对误差；
- 推荐模型及参数；
- `calibration_report.json`；
- 带表内范围检查的 MCU `calibration.h`。

```powershell
python .\MSPM0_Signal_Contest\tools\algorithm\calibration_fit\calibration_fit.py .\data.csv `
  --output-dir .\calibration_output `
  --max-abs-error 0.01 --max-rel-error 0.005
```

有明确误差门限时，工具在通过门限的候选中优先复杂度最低者；没有门限时使用确定性留出点误差和复杂度惩罚推荐。少于 8 点时无法形成独立留出集，报告会明确写 `in_sample_too_few_points_for_holdout`。输出 `PC_VERIFIED` 只表示脚本对给定 CSV 完成数值计算，不表示校准已上板；正式使用前必须用未参与拟合的参考点和真实硬件复验。

不要手抄终端系数，也不要让语言模型心算替代此工具。
