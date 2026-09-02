param([switch]$Force)

$ErrorActionPreference = 'Stop'
$contestRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

$purpose = @{
    'system_clock'='校验时钟树参数并把目标事件率换算成整数 Timer 周期。'
    'gpio'='用回调接口隔离具体 GPIO 实例，提供读、写、翻转操作。'
    'uart'='统一字节流收发接口，隔离 XDS110 UART 或其他串口实现。'
    'timer'='统一 Timer 设置周期、启动、停止和读计数接口。'
    'dma'='描述并校验 DMA 传输，再交给平台适配器执行。'
    'adc'='统一单次 ADC 原始码读取接口和通道/参考配置。'
    'dac'='统一 DAC 原始码输出及电压到码值换算。'
    'vref'='选择标称或实测参考电压，显式传递标定依据。'
    'opa'='描述 OPA 缓冲、同相和反相配置并交给平台适配器。'
    'gpamp'='描述 GPAMP 增益与偏置配置并交给平台适配器。'
    'comparator'='描述比较器阈值、迟滞和极性并交给平台适配器。'
    'adc_basic'='用阻塞读接口采集一段 ADC 原始码，适合最小验证。'
    'adc_timer_trigger'='按安全顺序组合 Timer 与 ADC 的 arm/start/stop。'
    'adc_dual_sync'='把同步双通道交织数据拆成两个独立数组。'
    'adc_continuous'='把连续采集帧交给回调并记录完成/丢帧计数。'
    'adc_pingpong_dma'='管理双缓冲 DMA 的 ready、release 和 overrun 状态。'
    'adc_ring_buffer'='提供单生产者/单消费者 ADC 环形缓冲区。'
    'trigger_capture'='在原始帧中查找带迟滞边沿并提取触发前后数据。'
    'timer_capture'='处理计数器回绕并由捕获时间戳计算平均周期。'
    'adc_to_voltage'='把 ADC 原始码按参考电压、增益和偏置换算为电压。'
    'dc_measure'='计算 ADC 原始均值和直流电压。'
    'mean'='计算 uint16 或 float 样本算术平均值。'
    'minmax'='计算 uint16 或 float 样本最小值和最大值。'
    'vpp'='由原始码极值计算峰峰值，并可换算成电压。'
    'rms'='计算包含直流分量的总 RMS。'
    'ac_rms'='去均值后计算交流 RMS，同时返回直流分量。'
    'frequency_zero_cross'='用带迟滞的整数过零周期平均估计频率。'
    'frequency_interpolation'='用线性过零插值获得亚采样点频率估计。'
    'frequency_timer_capture'='把 Timer 捕获时间戳换算为输入频率。'
    'duty'='按阈值统计高电平样本比例。'
    'phase'='把通道时延换算成 -180° 到 180° 相位。'
    'remove_dc'='计算并移除整帧平均直流分量。'
    'mean_filter'='执行无动态内存的滑动平均滤波。'
    'median_filter'='使用调用者工作区执行奇数窗中值滤波。'
    'fir_filter'='执行因果 FIR 卷积，系数和缓冲区由调用者提供。'
    'rect_window'='应用矩形窗并给出相干增益。'
    'hann_window'='生成/应用 Hann 窗并给出相干增益。'
    'fft'='执行原地 radix-2 单精度复数 FFT/IFFT。'
    'fft_magnitude'='把复数频谱换算为相干增益修正后的幅度谱。'
    'fft_peak'='在指定 bin 范围内查找主峰并换算频率。'
    'harmonic'='按基波整数倍聚合一个或多个频点的谐波幅度。'
    'thd'='由基波和谐波幅度计算 THD 比值与百分比。'
    'correlation'='在限定 lag 范围内搜索归一化互相关峰。'
    'zero_cross_linear_interpolation'='输出线性过零位置或由多周期过零位置估计频率。'
    'multi_cycle_average'='按每周期点数折叠多周期并逐相位平均。'
    'fft_parabolic_interpolation'='用主峰左右三点抛物线插值细化 FFT 频率。'
    'window_gain_correction'='计算任意窗相干增益并修正幅度。'
    'coherent_sampling'='为给定 Fs/N 选择最近的相干采样频点。'
    'multi_bin_energy'='聚合峰值附近多 bin 能量，降低频谱泄漏敏感度。'
    'sine_fit_3param'='在已知频率下最小二乘拟合 sin、cos 和 DC 三参数。'
    'adc_gain_offset_calibration'='由两点标定求增益/偏置并批量校正。'
    'channel_delay_calibration'='用互相关估计双通道固定延时及相位修正。'
    'jacobsen_interpolation'='用主峰左右三个复数 DFT bin 做 Jacobsen 亚 bin 频率插值。'
    'quinn_interpolation'='用 Quinn Second 复频谱估计器细化孤立单音频率。'
    'macleod_interpolation'='用 Macleod 复频谱估计器细化孤立单音频率。'
    'czt'='在单位圆指定窄频带上直接计算用户指定的复频谱点。'
    'frequency_response_correction'='用频率校准表的线性插值修正幅值和相位响应。'
    'dac_dc'='把目标直流电压换算为 DAC 码并通过适配器输出。'
    'dac_wave_table'='定义 DAC 波表并完成归一化波形到码值的安全换算。'
    'dac_dma'='管理调用者提供的 DAC DMA 平台启动/停止接口。'
    'dds'='用 32 位相位累加器和 2^n 波表生成任意配置频率。'
    'sine'='生成可配置偏置、幅度和相位的正弦 DAC 波表。'
    'square'='生成可配置占空比、偏置、幅度和相位的方波表。'
    'triangle'='生成可配置偏置、幅度和相位的三角波表。'
    'sawtooth'='生成上升或下降锯齿波表。'
    'arbitrary_wave'='用线性插值把捕获波形重采样到目标波表长度。'
    'frequency_sweep'='生成线性或对数扫频频点序列。'
    'am_modulation'='用消息序列和调制度生成 AM 数字样本。'
    'opa_buffer'='生成 OPA 电压跟随器配置。'
    'opa_noninverting_pga'='由目标同相增益和基准电阻计算反馈电阻。'
    'opa_inverting'='由目标反相增益和输入电阻计算反馈电阻。'
    'opa_dac_bias'='计算 DAC 偏置加有符号输入增益后的输出范围。'
    'opa_to_adc'='检查 OPA 预期输出是否落在 ADC 安全输入范围。'
    'gpamp_buffer'='生成 GPAMP 单位增益缓冲配置。'
    'gpamp_gain'='生成 GPAMP 目标增益与偏置配置。'
    'comparator_zero_cross'='以虚拟地为阈值生成过零比较器配置。'
    'comparator_threshold'='生成任意阈值、迟滞与输出极性的比较器配置。'
    'oscilloscope'='组合均值、极值、Vpp、总 RMS 和 AC RMS。'
    'frequency_meter'='统一波形插值法与 Timer Capture 法的频率入口。'
    'spectrum_analyzer'='组合窗、FFT、幅度谱、峰值和抛物线插值。'
    'harmonic_thd_analyzer'='组合谐波聚合和 THD 计算。'
    'dual_channel_phase_meter'='组合双通道互相关、时延和相位换算。'
    'dds_generator'='组合正弦波表和 DDS 初始化。'
    'sweep_analyzer'='把扫频点参考/响应幅度换算为增益 dB 并保留相位。'
    'waveform_capture_replay'='把捕获 ADC 波形重采样并缩放为 DAC 回放表。'
    'signal_analyzer'='组合时域统计与线性插值频率测量。'
}

function Get-Category([string]$relativePath) {
    if ($relativePath -like '01_bsp/*') { return 'BSP 适配层' }
    if ($relativePath -like '02_acquisition/*') { return '采集层' }
    if ($relativePath -like '03_measurement/*') { return '测量算法层' }
    if ($relativePath -like '04_dsp/*') { return 'DSP 层' }
    if ($relativePath -like '05_precision/*') { return '精密算法层' }
    if ($relativePath -like '06_generator/*') { return '波形生成层' }
    if ($relativePath -like '07_signal_frontend/*') { return '模拟前端配置层' }
    return '应用组合层'
}

function Get-Status([string]$name) {
    # The canonical registry owns current status. This legacy README generator
    # must not downgrade clean reimplementations based on an old name list.
    return 'SEE_MODULE_CARD_AND_VERIFICATION'
}

function Get-Memory([string]$name) {
    switch ($name) {
        'fft' { return '调用者提供 N 个 complex-f32，工作区 8N bytes；模块内动态分配 0。' }
        'spectrum_analyzer' { return '调用者提供 8N bytes FFT 区和约 2N bytes 单边幅度谱；模块内动态分配 0。' }
        'median_filter' { return '调用者另提供 window_size 个 float；模块内动态分配 0。' }
        'adc_pingpong_dma' { return '调用者提供 2×N×2 bytes 原始缓冲区；控制结构为常数大小。' }
        'adc_ring_buffer' { return '调用者提供 capacity×2 bytes；环形控制结构为常数大小。' }
        default { return '模块内动态分配 0；数组/工作区由调用者提供，具体大小由 API 的 count/capacity 决定。' }
    }
}

$moduleDirs = Get-ChildItem -Path $contestRoot -Directory -Recurse | Where-Object {
    $_.FullName -notmatch '\\09_examples\\|\\10_tests\\|\\tools\\|\\01_bsp\\common$' -and
    (Get-ChildItem -Path $_.FullName -Filter 'signal_*.c' -File -ErrorAction SilentlyContinue) -and
    (Get-ChildItem -Path $_.FullName -Filter 'signal_*.h' -File -ErrorAction SilentlyContinue)
}

$written = 0
foreach ($dir in $moduleDirs) {
    $name = $dir.Name
    $relative = $dir.FullName.Substring($contestRoot.Length + 1).Replace('\','/')
    if ($relative -eq '02_acquisition/adc_dma') { continue }
    $header = Get-ChildItem $dir.FullName -Filter 'signal_*.h' -File | Select-Object -First 1
    $headerText = Get-Content -Raw -LiteralPath $header.FullName
    $apiNames = [regex]::Matches($headerText, '\b(Signal[A-Za-z0-9_]+)\s*\(') |
        ForEach-Object { $_.Groups[1].Value } | Select-Object -Unique
    $apiText = if ($apiNames) { ($apiNames | ForEach-Object { '`' + $_ + '`' }) -join '、' } else { '见头文件' }
    $includeNames = [regex]::Matches($headerText, '#include\s+"([^"]+)"') |
        ForEach-Object { $_.Groups[1].Value } | Where-Object { $_ -ne $header.Name } |
        Select-Object -Unique
    $dependencyText = if ($includeNames) { ($includeNames | ForEach-Object { '`' + $_ + '`' }) -join '、' } else { '仅 C 标准库' }
    $modulePurpose = if ($purpose.ContainsKey($name)) { $purpose[$name] } else { "提供 $name 的可组合接口。" }
    $status = Get-Status $name
    $category = Get-Category $relative
    $memory = Get-Memory $name
    $hardwareText = if ($category -match 'BSP|采集|前端') {
        '通用代码不绑定 SysConfig 实例名；接到 MSPM0 时由独立平台适配器使用生成宏。当前状态不代表对应外设已实板验证。'
    } else {
        '无寄存器、引脚或 SysConfig 依赖，可在 PC 上独立测试。'
    }
    $testText = if ($status -eq 'MODULE_STATUS_DRAFT') {
        '只验证接口可编译和返回 NOT_SUPPORTED；在公式、误差与内存测试完成前不得用于比赛结果。'
    } else {
        '纳入 `10_tests/pc` 全库构建；关键数值路径还应按题目范围补充向量和误差上限。'
    }

    $validationText = if ($category -match 'BSP|采集|前端') {
        'Hardware validation: PENDING。在 SysConfig 中按目标引脚/实例完成平台适配，用已知输入验证启停、边界和连续重启，记录变量与实测条件后才可升级 BOARD_VERIFIED。'
    } else {
        'Hardware validation: PENDING。先用 PC 已知向量和误差上限验证，再接入已验证的采集/发生链，覆盖题目最小值、典型值和最大值后才可升级。'
    }

    $readme = @"
# $name

## 1. 模块作用

$modulePurpose

## 2. 输入

输入由公开头文件中的指针、长度、配置结构或平台回调给出；所有单位写在字段名中。

## 3. 输出

输出写入调用者提供的结果/缓冲区，并返回 ``signal_result_t``；不通过隐藏全局变量传递数据。

## 4. 依赖

$dependencyText。

## 5. SysConfig 设置

$hardwareText

## 6. 初始化方法

模块不做隐式全局初始化。包含 ``$($header.Name)``，由调用者准备配置、缓冲区或平台回调；如头文件提供 Init/Configure，先调用它。

## 7. 调用方法

$apiText。

## 8. 参数修改方法

只修改调用者配置结构、count/capacity 和采样率等函数参数；不要为某个 Demo 改底层 ``.c``。

## 9. 与其他模块如何连接

通过 ``signal_types.h`` 的数组+长度+采样率语义或本模块公开结构连接；先检查返回码再消费输出。

## 10. 最小示例

~~~c
#include "$($header.Name)"

/* 按头文件准备输入/输出，调用上述主 API，并检查 signal_result_t。 */
~~~

$testText

## 11. 常见错误

空指针、零长度、capacity 小于 count、单位混用、把配置采样率当物理实测值，以及复用仍在使用的工作区。

## 12. RAM 占用

$memory

## 13. Flash 占用

无固定常量：取决于编译优化、是否链入数学库和死代码删除。已纳入整库链接检查；比赛应用以 CCS 生成的 `.map` 为最终数据。

## 14. CPU 计算量估计

函数为同步确定性处理；硬件回调的中断上下文只做最小状态更新，重计算放在主循环。

## 15. 当前验证状态

``$status``。该状态只表示现有证据等级，不等于完整比赛场景已经验证。

## 16. 以后实板验证步骤

$validationText

不使用时，从工程移除本目录 `.c` 及上层引用；若有平台外设适配，再从 SysConfig 删除对应实例。
"@

    $card = @"
# MODULE CARD: $name

| 项目 | 内容 |
|---|---|
| 目录 | ``$relative`` |
| 层级 | $category |
| 作用 | $modulePurpose |
| 输入/输出 | 公开结构或调用者缓冲区；无隐藏数据通道 |
| 依赖 | $dependencyText |
| RAM | $memory |
| 状态 | ``$status`` |
| 独立测试 | $testText |
| 硬件声明 | $hardwareText |
| 移除 | 删除本目录源文件及上层引用；平台实例按需从 SysConfig 移除 |
"@

    $readmePath = Join-Path $dir.FullName 'README.md'
    $cardPath = Join-Path $dir.FullName 'MODULE_CARD.md'
    if ($Force -or -not (Test-Path $readmePath)) {
        Set-Content -LiteralPath $readmePath -Value $readme -Encoding utf8
        $written++
    }
    if ($Force -or -not (Test-Path $cardPath)) {
        Set-Content -LiteralPath $cardPath -Value $card -Encoding utf8
        $written++
    }
}

Write-Output "module_dirs=$($moduleDirs.Count) files_written=$written"

