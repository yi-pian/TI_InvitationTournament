# ADC Buffer UART Dump（TEST ONLY）

该工程从板载 TMP6131 采一帧，然后用 UART0 输出纯 CSV：

```csv
INDEX,ADC_RAW
0,xxxx
1,xxxx
```

UART 代码只在本例 `main.c` 中，正式 ADC_DMA 模块不依赖 UART。默认配置为 100 kSPS、N=1024、115200 baud、8 data bits、no parity、1 stop bit、no flow control。

## 板卡跳线

断电确认：

```text
J9    1-2   TMP6131 直连 PB24/ADC0.5
J13   ON    温敏/模拟区供电
J21   1-2   PA10/UART0_TX 连接 XDS110 backchannel
J22   1-2   PA11/UART0_RX 连接 XDS110 backchannel
J101  9-10、7-8 保持安装（XDS110 UART 隔离排针）
```

User's Guide 把 Windows 端口名称写为 `XDS110 Class Application/User UART`；COM 编号由电脑分配。

## CCS 运行与 COM 口

1. 导入 `ticlang/adc_buffer_uart_dump_LP_MSPM0G3507_nortos_ticlang.projectspec`，Build、Download，但先不要 Run。
2. Windows 设备管理器 -> `端口 (COM 和 LPT)`，找到 `XDS110 Class Application/User UART (COMx)`。
3. 在 CCS Terminal 或串口终端打开该 COM：115200、8-N-1、无流控。关闭时间戳、本地回显和自动加前缀。
4. Run。发送 1025 行后程序停在 `__BKPT(0)`。
5. Expressions 应看到 `g_uart_dump_complete=true`、`g_uart_dump_pass=true`、`g_dumped_samples=1024`、`g_module_status=MODULE_DONE`。

如果 COM 口打不开，通常是另一个终端仍占用该端口；关闭后重试。不要选择 `XDS110 Class Auxiliary Data Port` 或调试探针端口。

## 保存 CSV

最简单的方法是在 CCS Terminal 输出结束后全选、复制到记事本，确保第一行恰好是 `INDEX,ADC_RAW`，保存为 UTF-8/ASCII 的 `adc_capture.csv`。不要把 CCS 日志、时间戳或断点文字一起复制。

也可在 Windows PowerShell 先打开串口等待，再回到 CCS 按 Run（把 COM7 改成实际端口）：

```powershell
$serial = [System.IO.Ports.SerialPort]::new("COM7", 115200, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$serial.NewLine = "`n"
$serial.ReadTimeout = 30000
$serial.Open()
$rows = for ($i = 0; $i -lt 1025; $i++) { $serial.ReadLine().Trim() }
$serial.Close()
$rows | Set-Content -Encoding ascii adc_capture.csv
```

## 绘图

```powershell
py tools\pc\plot_adc_csv.py adc_capture.csv
```

脚本只使用 Python 标准库，会生成 `adc_capture_plot.svg` 并用默认浏览器打开。只保存、不自动打开：

```powershell
py tools\pc\plot_adc_csv.py adc_capture.csv --output adc_capture.svg --no-show
```

脚本会输出 sample count、raw min、raw max、raw mean，并绘制 ADC Raw vs Sample Index。当前电脑若尚无 `py`/`python` 命令，先安装 Python 3 并勾选加入 PATH；不需要安装 matplotlib。详见 [tools/pc/README.md](../../tools/pc/README.md)。

## 构建记录

当前工程收口已用 MSPM0 SDK 2.11.00.07、SysConfig 1.28.0、TI Arm Clang 5.1.1.LTS 完成 `-O2 -Wall -Werror` 编译和完整链接。早期 1.26.2/4.0.2 记录不再作为当前基线。实板 UART 数据仍需运行后确认。
