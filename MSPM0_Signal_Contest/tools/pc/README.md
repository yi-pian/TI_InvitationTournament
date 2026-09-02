# ADC CSV PC 调试工具

`plot_adc_csv.py` 只接受两列 CSV：

```csv
INDEX,ADC_RAW
0,1984
1,1986
```

运行：

```powershell
py tools\pc\plot_adc_csv.py adc_capture.csv
```

脚本只用 Python 标准库，默认生成 `adc_capture_plot.svg` 并用浏览器打开，不需要 matplotlib。当前电脑若没有 `py` 或 `python` 命令，请先安装 Python 3 并选择 Add Python to PATH；即使暂时不安装，也可以先用 CCS Graph 查看 `g_adc_buffer`。

可选参数：

- `--output adc.svg`：指定 SVG 输出文件；默认是 `<CSV文件名>_plot.svg`。
- `--no-show`：不打开交互窗口，适合只保存图。

脚本验证 INDEX 从 0 连续增长、ADC_RAW 在 0..4095 内，然后打印 sample count、raw min、raw max、raw mean。它是 PC 调试工具，不是固件 measurement 模块。
