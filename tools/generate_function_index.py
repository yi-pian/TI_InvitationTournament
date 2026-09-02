#!/usr/bin/env python3
"""Generate the contest-project function reuse index.

The generator is deliberately repository-local and dependency-free.  It scans
function *definitions* in main.c, modules/*.c and inline definitions in
modules/*.h.  Generated SysConfig output and Debug/Release trees are excluded.
"""

from __future__ import annotations

import html
import re
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path


WORKSPACE = Path(__file__).resolve().parents[1]
FUXIAN = WORKSPACE / "fuxian"
OUTPUT = FUXIAN / "工程函数分类复用索引.md"

PROJECTS = [
    "24_A_rebuild",
    "24_C_rebuild",
    "22_X",
    *[f"example{index:02d}" for index in range(1, 9)],
]

CATEGORY_ORDER = [
    "工程入口、调度与中断",
    "ADC、DMA、连续采集与缓冲",
    "DAC、DDS、波形发生与扫频",
    "基础时域与电压测量",
    "频率、相位与同步检波",
    "FFT、频谱、谐波与失真",
    "稳健估计、校准与补偿",
    "触发、捕获、存储与回放",
    "模拟前端、比较器、增益与功耗",
    "TFT、字库、波形与界面绘制",
    "矩阵键盘与交互输入",
    "外部器件与阻塞式总线",
    "数学、状态与内部公共支持",
]


# basename -> (category, concise module purpose)
MODULE_INFO: dict[str, tuple[str, str]] = {
    "ad9833": ("外部器件与阻塞式总线", "AD9833 外部 DDS 的 SPI 配置、频率/相位/波形控制"),
    "ad9850": ("外部器件与阻塞式总线", "AD9850 并行/串行位操作、调谐字和输出控制"),
    "ad9850_mspm0_platform": ("外部器件与阻塞式总线", "AD9850 到 MSPM0 GPIO/延时接口的适配层"),
    "mspm0_blocking_bus": ("外部器件与阻塞式总线", "MSPM0 阻塞式 SPI/I2C/UART 公共收发辅助"),
    "signal_adc_dma": ("ADC、DMA、连续采集与缓冲", "单路 ADC，由定时器事件触发并通过 DMA 采集"),
    "signal_adc_pingpong_dma": ("ADC、DMA、连续采集与缓冲", "ADC ping-pong 双缓冲的块完成/消费状态管理"),
    "signal_adc_ring_buffer": ("ADC、DMA、连续采集与缓冲", "ADC 样本环形缓冲、溢出检测和 FIFO 读写"),
    "signal_dual_adc_mspm0g3507": ("ADC、DMA、连续采集与缓冲", "MSPM0G3507 双 ADC 同定时器事件同步 DMA 采集"),
    "signal_ac_rms": ("基础时域与电压测量", "去除直流后的交流有效值 AC RMS"),
    "signal_adc_to_voltage": ("基础时域与电压测量", "ADC 原始码到电压的量化换算"),
    "signal_mean": ("基础时域与电压测量", "数组平均值/直流分量计算"),
    "signal_minmax": ("基础时域与电压测量", "最小值、最大值及首次位置搜索"),
    "signal_rms": ("基础时域与电压测量", "包含直流分量的真 RMS 计算"),
    "signal_statistics": ("基础时域与电压测量", "均值、方差、标准差等统计量"),
    "signal_vpp": ("基础时域与电压测量", "峰峰值 Vpp 计算"),
    "signal_dual_adc_phase": ("频率、相位与同步检波", "双路原始 ADC 动态中点、滞回过零及平均相位差"),
    "signal_lock_in": ("频率、相位与同步检波", "已知参考频率下的正交锁相/同步检波"),
    "signal_multi_cycle_average": ("频率、相位与同步检波", "跨多个周期平均周期和频率，抑制单周期抖动"),
    "signal_phase": ("频率、相位与同步检波", "由过零、相关延迟或 FFT bin 计算通道相位差"),
    "signal_timer_capture": ("频率、相位与同步检波", "通用定时器边沿捕获状态和频率/周期计算"),
    "signal_timer_capture_mspm0g3507": ("频率、相位与同步检波", "MSPM0G3507 Capture 外设初始化、IRQ 与测频适配"),
    "signal_zero_cross": ("频率、相位与同步检波", "滞回上升过零检测与频率估计"),
    "signal_zero_cross_interpolation": ("频率、相位与同步检波", "过零点线性插值，提高周期/频率分辨率"),
    "signal_fft": ("FFT、频谱、谐波与失真", "未归一化 radix-2 复数/实数前向 FFT"),
    "signal_fft_magnitude": ("FFT、频谱、谐波与失真", "复频谱到单边 magnitude 的换算"),
    "signal_fft_parabolic_interpolation": ("FFT、频谱、谐波与失真", "峰值 bin 三点抛物线插值"),
    "signal_harmonic": ("FFT、频谱、谐波与失真", "按已知基波定位各次谐波并做邻 bin 聚合"),
    "signal_multi_bin_energy": ("FFT、频谱、谐波与失真", "目标 bin 邻域的多 bin 能量聚合"),
    "signal_peak_detect": ("FFT、频谱、谐波与失真", "指定频谱区间的峰值搜索"),
    "signal_sfdr": ("FFT、频谱、谐波与失真", "无杂散动态范围 SFDR 计算"),
    "signal_snr": ("FFT、频谱、谐波与失真", "频谱目标带与噪声带的 SNR 计算"),
    "signal_thd": ("FFT、频谱、谐波与失真", "基波和谐波能量的总谐波失真 THD 计算"),
    "signal_window": ("FFT、频谱、谐波与失真", "Rect/Hann/Hamming/Blackman 窗生成与加窗"),
    "signal_window_gain_correction": ("FFT、频谱、谐波与失真", "窗函数相干增益/能量增益修正"),
    "signal_adc_gain_offset_calibration": ("稳健估计、校准与补偿", "ADC 两点增益/偏移标定和应用"),
    "signal_channel_delay_calibration": ("稳健估计、校准与补偿", "双通道固定延迟标定和相位补偿"),
    "signal_frequency_response_correction": ("稳健估计、校准与补偿", "频响标定表的增益/相位插值与校正"),
    "signal_hampel": ("稳健估计、校准与补偿", "滑窗中位数+MAD 的 Hampel 离群点替换"),
    "signal_mad": ("稳健估计、校准与补偿", "中位数绝对偏差 MAD 和稳健 sigma"),
    "signal_median_filter": ("稳健估计、校准与补偿", "滑窗中值滤波"),
    "signal_remove_dc": ("稳健估计、校准与补偿", "估计并移除信号直流分量"),
    "signal_robust_peak_to_peak": ("稳健估计、校准与补偿", "分位数峰峰值，降低毛刺影响"),
    "signal_robust_rms": ("稳健估计、校准与补偿", "Winsorize 后的稳健 RMS/AC RMS"),
    "signal_sine_fit_3param": ("稳健估计、校准与补偿", "已知频率三参数正弦最小二乘拟合"),
    "signal_sine_fit_4param": ("稳健估计、校准与补偿", "频率可调四参数正弦拟合"),
    "signal_arbitrary_wave": ("触发、捕获、存储与回放", "任意波样本的线性重采样，供 DAC 回放"),
    "signal_single_capture_replay": ("触发、捕获、存储与回放", "连续触发、基线裁剪、三槽存储、显示和回放编排"),
    "signal_trigger_capture": ("触发、捕获、存储与回放", "触发点搜索和固定长度片段提取"),
    "signal_clipping_detect": ("模拟前端、比较器、增益与功耗", "ADC/电压上下限削顶计数"),
    "signal_comparator": ("模拟前端、比较器、增益与功耗", "比较器迟滞/门限预算和通用配置校验"),
    "signal_comparator_threshold": ("模拟前端、比较器、增益与功耗", "固定门限比较器配置预算"),
    "signal_comparator_zero_cross": ("模拟前端、比较器、增益与功耗", "偏置过零比较器配置预算"),
    "signal_gpamp": ("模拟前端、比较器、增益与功耗", "GPAMP 增益与输出范围预算"),
    "signal_gpamp_buffer": ("模拟前端、比较器、增益与功耗", "GPAMP 单位增益缓冲配置预算"),
    "signal_opa": ("模拟前端、比较器、增益与功耗", "片上 OPA 增益计算与配置校验"),
    "signal_opa_inverting": ("模拟前端、比较器、增益与功耗", "反相 OPA/PGA 参数预算"),
    "signal_opa_noninverting_pga": ("模拟前端、比较器、增益与功耗", "同相 OPA/PGA 参数预算"),
    "signal_opa_to_adc": ("模拟前端、比较器、增益与功耗", "OPA 输出与 ADC 量程/偏置兼容性检查"),
    "signal_slew_rate": ("模拟前端、比较器、增益与功耗", "从波形上升/下降沿计算压摆率"),
    "signal_static_power": ("模拟前端、比较器、增益与功耗", "供电电压、电流到静态功耗的换算"),
    "signal_vca820_gain_control": ("模拟前端、比较器、增益与功耗", "VCA820 控制电压、增益和 DAC 码预算"),
    "signal_tft_ili9341": ("TFT、字库、波形与界面绘制", "ILI9341 SPI 屏驱动、图元和文字输出"),
    "signal_tft_ili9341_mspm0g3507": ("TFT、字库、波形与界面绘制", "ILI9341 到 MSPM0 SPI/GPIO 的平台适配"),
    "signal_tft_st7789": ("TFT、字库、波形与界面绘制", "ST7789 SPI 屏驱动和基础图元"),
    "signal_tft_st7789_font": ("TFT、字库、波形与界面绘制", "ST7789 多尺寸 ASCII 字库、数字和浮点文本"),
    "signal_tft_st7789_mspm0g3507": ("TFT、字库、波形与界面绘制", "ST7789 到 MSPM0 SPI/GPIO 的平台适配"),
    "signal_tft_st7789_text": ("TFT、字库、波形与界面绘制", "ST7789 兼容小字库文字辅助"),
    "signal_tft_waveform_st7789": ("TFT、字库、波形与界面绘制", "ST7789 坐标网格、缩放和波形折线绘制"),
    "signal_matrix_keypad_4x4": ("矩阵键盘与交互输入", "4×4 矩阵键盘行列扫描、消抖、鬼键过滤和符号映射"),
    "signal_dac_dma_mspm0g3507": ("DAC、DDS、波形发生与扫频", "MSPM0G3507 DAC12 定时事件+DMA 循环输出"),
    "signal_dac_wave_table": ("DAC、DDS、波形发生与扫频", "归一化浮点波形到 DAC 原始码的波表转换"),
    "signal_dds": ("DAC、DDS、波形发生与扫频", "相位累加器 DDS 初始化、调频和样本填充"),
    "signal_frequency_sweep": ("DAC、DDS、波形发生与扫频", "线性/对数扫频点生成"),
    "signal_sawtooth": ("DAC、DDS、波形发生与扫频", "锯齿波样本生成"),
    "signal_sine": ("DAC、DDS、波形发生与扫频", "正弦波样本生成"),
    "signal_square": ("DAC、DDS、波形发生与扫频", "方波样本生成"),
    "signal_triangle": ("DAC、DDS、波形发生与扫频", "三角波样本生成"),
    "signal_wave_output_mspm0g3507": ("DAC、DDS、波形发生与扫频", "频率/Vpp/偏置三参数统一波形输出封装"),
    "signal_algorithm_status": ("数学、状态与内部公共支持", "算法状态码公共定义"),
    "signal_complex": ("数学、状态与内部公共支持", "轻量复数类型公共定义"),
    "signal_fft_backend_config": ("数学、状态与内部公共支持", "FFT 后端选择和编译配置"),
    "signal_math": ("数学、状态与内部公共支持", "数学常数与轻量公共接口"),
    "signal_math_backend": ("数学、状态与内部公共支持", "sqrt/atan2 等数学后端适配"),
    "signal_math_backend_config": ("数学、状态与内部公共支持", "数学后端编译配置"),
    "signal_status": ("数学、状态与内部公共支持", "驱动/模块统一状态码"),
    "signal_types": ("数学、状态与内部公共支持", "采样率、波形等公共数据类型"),
}

PROJECT_PURPOSE = {
    "24_A_rebuild": "24_A 四问应用（DDS/VCA、扫频带宽、压摆率、静态功耗）",
    "24_C_rebuild": "24_C 双路动态采样、统计/FFT/谐波、突发检测与显示",
    "22_X": "PLL/Y 通道倍频控制、双 ADC 相位测量和 ILI9341 李萨如显示",
    "example01": "双通道可调信号分析仪",
    "example02": "DAC/DDS 激励下的幅频/相频扫频测试",
    "example03": "双路连续/有限帧数字示波与 XY 显示",
    "example04": "综合波形发生、测量、频谱、稳健处理、捕获与回放平台",
    "example05": "AD9833 激励的阻抗扫频和 RLC 参数识别",
    "example06": "单 ADC 弱信号自动频率/幅值测量和多周期显示",
    "example07": "未知通道频响测量、1 kHz 与谐波自动补偿",
    "example08": "片上 GPAMP/OPA/COMP 双通道模拟前端分析仪",
}

MAIN_EXACT: dict[tuple[str, str], str] = {
    ("24_A_rebuild", "App_RunQuestion2"): "执行 Q2 对数/分段扫频，测量各点 AC RMS，并求增益、截止频率和带宽。",
    ("24_A_rebuild", "App_RunQuestion3"): "执行 Q3 波形采集、稳健 Vpp 和上升/下降沿压摆率测量。",
    ("24_A_rebuild", "App_RunQuestion4"): "执行 Q4：关闭 DDS，采集电源电压/采样电阻压降并计算静态功耗。",
    ("24_A_rebuild", "App_DDSWakeAndSetFrequency"): "退出 AD9850 掉电状态并设置目标输出频率。",
    ("24_A_rebuild", "VCA820_TargetVppToDACCode"): "把目标输出 Vpp 换算为 VCA820 控制 DAC 码。",
    ("24_A_rebuild", "VCA820_SetTargetVpp"): "写 DAC 控制 VCA820，并返回实际采用的 DAC 码。",
    ("24_C_rebuild", "RunFFT"): "对去直流、加 Hann 窗的帧做 CMSIS RFFT，生成幅度谱并估计基波频率。",
    ("24_C_rebuild", "AnalyzeHarmonics"): "围绕基波及多次谐波聚合频谱能量，给出各次幅值/有效性。",
    ("24_C_rebuild", "ClassifyWaveform"): "综合时域统计量、谐波比例和频率特征识别正弦/方波/三角波/脉冲等波形。",
    ("24_C_rebuild", "DetectBurstFromAnalog"): "从连续模拟样本的包络/门限变化中检测突发起止、持续时间和有效性。",
    ("24_C_rebuild", "SelectSampleRate"): "按已测频率选择下一帧采样率，使每周期点数兼顾精度与带宽。",
    ("22_X", "App_SetPLLMultiplier"): "设置 1–9 倍 PLL GPIO 编码，并同步相位算法的 fY/fX。",
    ("22_X", "App_SetYVMultiplier"): "设置 Y 通道量程/增益控制 GPIO。",
    ("22_X", "App_SetWaveMode"): "切换外部波形控制 GPIO 状态。",
    ("22_X", "Lissajous_DrawFrame"): "把同步 X/Y ADC 帧映射为 ILI9341 李萨如轨迹。",
    ("example01", "App_MeasurePhase"): "调用双 ADC 相位算法，以动态中点和多次过零求 Y 相对 X 的相位。",
    ("example01", "App_DrawTrace"): "将双通道时域帧降采样并绘制到 ST7789 波形区。",
    ("example02", "App_RunSweepPoint"): "输出当前扫频点，完成双路 ADC 采集，计算幅值增益/相位并更新曲线。",
    ("example03", "App_DrawWaveforms"): "按当前 CH1/CH2/双通道/XY 模式局部擦除并重绘波形。",
    ("example04", "App_BasicMeasurements"): "完成均值、最小/最大、Vpp、RMS、AC RMS、标准差和削顶等基础指标。",
    ("example04", "App_TimeFrequency"): "综合过零、插值、多周期平均和硬件 Capture 得到多种频率测量结果。",
    ("example04", "App_Spectrum"): "执行去直流、窗、FFT、插值、谐波、THD、SNR、SFDR 等频谱分析。",
    ("example04", "App_RobustMeasurement"): "执行中值/Hampel/MAD、稳健 Vpp 和稳健 RMS，对抗脉冲毛刺。",
    ("example04", "App_SineFitAndLockIn"): "运行三/四参数正弦拟合和锁相检测，精修幅值、频率、相位。",
    ("example04", "App_ServiceCapture"): "消费比较器触发，拼接连续 ADC 块并交给三槽捕获回放模块。",
    ("example05", "App_MeasureFrame"): "采集电流/电压两路 ADC，求 RMS、阻抗模和相位。",
    ("example05", "App_FitSeriesModel"): "根据扫频复阻抗数据拟合串联 R/L/C 参数。",
    ("example05", "App_DrawAnalysis"): "寻找谐振点与 -3 dB 带宽，判断元件类型并显示 RLC、f0、BW、Q。",
    ("example06", "App_Measure"): "单帧完成去直流、RFFT 峰值、过零插值候选、Vpp 和相位信息测量。",
    ("example06", "App_FindDisplayWindow"): "选择与 FFT 目标频率一致的连续过零周期作为显示窗口。",
    ("example06", "App_PrepareWave"): "按键设定的显示周期数对有效窗口重采样，准备 320 点屏幕波形。",
    ("example07", "App_MeasureResponse"): "用双路锁相结果计算未知通道的增益和相位响应。",
    ("example07", "App_RunSweep"): "逐点产生 DDS、采集响应并建立未知通道频响表。",
    ("example07", "App_Run1kCompensation"): "由频响表生成 1 kHz 预补偿，测量并显示残余增益/相位误差。",
    ("example07", "App_RunHarmonics"): "分别测量基波/二次/三次响应，合成预失真波表进行谐波补偿。",
    ("example08", "App_DrawLissajous"): "把 GPAMP→ADC1 与 OPA0→ADC0 同步帧归一化后绘制李萨如轨迹。",
    ("example08", "App_CountComparatorEdgesFromADC"): "按当前比较器门限在 ADC 帧中软件统计上升/下降穿越，补充硬件边沿计数。",
    ("example08", "App_SetComparatorMode"): "切换 COMP0 的 1.65 V 过零或约 2.0 V 门限 DAC 配置。",
}

EXCLUDED_FROM_DEBUG = {
    "example05": {
        "signal_dac_dma_mspm0g3507.c",
        "signal_dac_wave_table.c",
        "signal_dds.c",
        "signal_sine.c",
        "signal_square.c",
        "signal_triangle.c",
        "signal_sawtooth.c",
        "signal_wave_output_mspm0g3507.c",
    }
}


@dataclass(frozen=True)
class Definition:
    project: str
    relative_file: str
    line: int
    name: str
    signature: str
    is_static: bool


def mask_non_code(source: str) -> str:
    """Blank comments, strings and preprocessor lines while preserving offsets."""
    output = list(source)
    index = 0
    state = "code"
    while index < len(source):
        if state == "code":
            if source.startswith("//", index):
                output[index] = output[index + 1] = " "
                index += 2
                state = "line_comment"
                continue
            if source.startswith("/*", index):
                output[index] = output[index + 1] = " "
                index += 2
                state = "block_comment"
                continue
            if source[index] == '"':
                output[index] = " "
                index += 1
                state = "string"
                continue
            if source[index] == "'":
                output[index] = " "
                index += 1
                state = "character"
                continue
            index += 1
        elif state == "line_comment":
            if source[index] == "\n":
                state = "code"
            else:
                output[index] = " "
            index += 1
        elif state == "block_comment":
            if source.startswith("*/", index):
                output[index] = output[index + 1] = " "
                index += 2
                state = "code"
            else:
                if source[index] != "\n":
                    output[index] = " "
                index += 1
        else:
            quote = '"' if state == "string" else "'"
            if source[index] == "\\":
                output[index] = " "
                index += 1
                if index < len(source) and source[index] != "\n":
                    output[index] = " "
                    index += 1
                continue
            if source[index] == quote:
                output[index] = " "
                index += 1
                state = "code"
            else:
                if source[index] != "\n":
                    output[index] = " "
                index += 1

    masked = "".join(output)
    result: list[str] = []
    for line in masked.splitlines(keepends=True):
        if line.lstrip().startswith("#"):
            result.append("".join("\n" if char == "\n" else " " for char in line))
        else:
            result.append(line)
    return "".join(result)


def extract_definitions(project: str, path: Path, root: Path) -> list[Definition]:
    source = path.read_text(encoding="utf-8", errors="replace")
    masked = mask_non_code(source)
    depth = 0
    parentheses = 0
    statement_start = 0
    definitions: list[Definition] = []

    for index, char in enumerate(masked):
        if depth == 0:
            if char == "(":
                parentheses += 1
            elif char == ")" and parentheses:
                parentheses -= 1
            elif char in ";}" and parentheses == 0:
                statement_start = index + 1
            elif char == "{" and parentheses == 0:
                header = masked[statement_start:index]
                opening = header.find("(")
                if opening >= 0 and "=" not in header[:opening]:
                    match = re.search(r"([A-Za-z_]\w*)\s*$", header[:opening])
                    if (
                        match
                        and match.group(1) not in {"if", "for", "while", "switch"}
                        and not header.lstrip().startswith("typedef")
                    ):
                        name = match.group(1)
                        absolute_name = statement_start + match.start(1)
                        line = masked.count("\n", 0, absolute_name) + 1
                        signature = " ".join(header.split())
                        definitions.append(
                            Definition(
                                project=project,
                                relative_file=path.relative_to(root).as_posix(),
                                line=line,
                                name=name,
                                signature=signature,
                                is_static=bool(re.search(r"\bstatic\b", signature)),
                            )
                        )
                depth = 1
                statement_start = index + 1
        else:
            if char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
                if depth == 0:
                    statement_start = index + 1
    return definitions


def clean_comment_text(text: str) -> str:
    lines = []
    for line in text.splitlines():
        line = re.sub(r"^\s*\*\s?", "", line).strip()
        if line:
            lines.append(line)
    return " ".join(lines)


def extract_briefs(header_path: Path) -> dict[str, str]:
    source = header_path.read_text(encoding="utf-8", errors="replace")
    briefs: dict[str, str] = {}
    # The tempered comment body is important: a plain ``.*?`` may backtrack
    # across several later comments when a file-level @brief is followed by a
    # typedef, incorrectly attaching the file description to a later API.
    pattern = re.compile(
        r"/\*((?:(?!\*/).)*)\*/\s*([^;{}]+\([^;{}]*\)\s*;)", re.S
    )
    for match in pattern.finditer(source):
        comment, declaration = match.groups()
        if declaration.lstrip().startswith("typedef"):
            continue
        names = re.findall(r"([A-Za-z_]\w*)\s*\(", declaration)
        if not names:
            continue
        name = names[-1]
        brief_match = re.search(
            r"@brief\s+(.*?)(?=\n\s*\*?\s*@(?:param|return|note|warning|see|retval)\b|\Z)",
            comment,
            re.S,
        )
        if brief_match:
            brief = clean_comment_text(brief_match.group(1))
        else:
            brief = clean_comment_text(comment)
            brief = re.split(r"\s+@(?:param|return|note|warning|see|retval)\b", brief)[0]
        if brief.startswith("@file"):
            continue
        if brief:
            briefs[name] = brief
    return briefs


def split_identifier(name: str) -> list[str]:
    spaced = re.sub(r"([a-z0-9])([A-Z])", r"\1 \2", name)
    spaced = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1 \2", spaced)
    return [token for token in re.split(r"[_\s]+", spaced) if token]


TOKEN_ZH = {
    "init": "初始化",
    "start": "启动",
    "stop": "停止",
    "process": "处理",
    "generate": "生成",
    "fill": "填充",
    "draw": "绘制",
    "clear": "清除",
    "set": "设置",
    "get": "读取",
    "read": "读取",
    "write": "写入",
    "compute": "计算",
    "calculate": "计算",
    "measure": "测量",
    "analyze": "分析",
    "find": "查找",
    "select": "选择",
    "convert": "换算",
    "apply": "应用",
    "validate": "校验",
    "check": "检查",
    "update": "更新",
    "refresh": "刷新",
    "handle": "处理",
    "queue": "入队",
    "pop": "出队",
    "push": "入队",
    "release": "释放",
    "acquire": "获取",
    "capture": "捕获",
    "replay": "回放",
    "arm": "布防",
    "service": "维护",
    "map": "坐标映射",
    "interpolate": "插值",
    "classify": "分类",
    "detect": "检测",
    "remove": "移除",
    "wrap": "归一化",
    "fit": "拟合",
    "forward": "前向变换",
    "next": "产生下一样本",
    "count": "计数",
    "status": "状态",
    "rate": "采样率",
    "frequency": "频率",
    "phase": "相位",
    "harmonics": "谐波",
    "spectrum": "频谱",
    "waveform": "波形",
    "continuous": "连续模式",
    "snapshot": "一致性快照",
    "finished": "完成标志",
    "module": "模块",
    "config": "配置",
    "key": "按键",
    "text": "文本",
    "integer": "整数",
    "float": "浮点数",
    "screen": "屏幕",
    "page": "页面",
    "static": "静态区域",
    "dynamic": "动态区域",
    "line": "线段",
    "rect": "矩形",
    "pixel": "像素",
    "buffer": "缓冲区",
    "sample": "样本",
    "power": "功耗",
    "gain": "增益",
    "threshold": "门限",
    "bounds": "边界",
}


def generic_action(name: str) -> str:
    special = {
        "main": "工程入口：初始化 SysConfig 和所选模块，随后进入应用主循环",
        "SysTick_Handler": "SysTick 中断：提供 1 ms 时基并驱动键盘周期扫描/事件入队",
        "GROUP1_IRQHandler": "GROUP1 外设中断：读取并清除比较器事件，向捕获状态机发布触发",
        "COMP0_IRQHandler": "COMP0 中断：读取比较器上升/下降事件并累计硬件边沿",
        "valid": "检查驱动上下文、回调和初始化状态是否可用",
        "transfer_unlocked": "在外层已持锁时执行一次 SPI/GPIO 数据事务",
        "transfer": "加锁后执行一次 SPI/GPIO 数据事务并解锁",
        "command_data": "向 TFT 发送命令及其可选参数数据",
        "write_spi": "MSPM0 SPI 阻塞发送适配回调",
        "write_word": "拉低片选并向 AD9833 发送一个 16 位控制字",
        "bit_reverse": "执行 FFT 输入/输出索引的位反转重排",
        "SignalDDS_IsPowerOfTwo": "判断波表长度是否为 2 的幂，以便使用相位掩码快速寻址",
        "trim_baseline": "裁去捕获片段首尾的长直流基线，保留中间有效波形",
    }
    if name in special:
        return special[name]
    lowered = name.lower()
    api_rules = [
        ("startcontinuous", "启动连续多块 DMA 采集并发布完整块快照"),
        ("getcontinuoussnapshot", "原子读取连续 DMA 已完成块的序号和索引"),
        ("getcontinuousblocksequence", "读取连续 DMA 的已发布块序号"),
        ("getcontinuouscompletedblock", "读取连续 DMA 最近完成的块索引"),
        ("setconstant", "设置运行参数为固定值并更新模块状态"),
        ("setsamplerate", "按目标采样率重算并写入采样 Timer 周期"),
        ("setupdaterate", "按目标更新率重算并写入 DAC Timer 周期"),
        ("setfrequency", "重算相位步进或调谐字并设置输出频率"),
        ("getconfiguredrate", "返回 Timer 量化后实际采用的采样/更新率"),
        ("getconfiguredfrequency", "返回当前相位步进对应的实际输出频率"),
        ("getmodulematurity", "返回模块成熟度和验证等级元数据"),
        ("getmodulestatus", "返回模块成熟度和验证等级元数据"),
        ("getstatus", "返回模块当前运行状态、错误码和完成信息"),
        ("isfinished", "查询本次 DMA/算法处理是否完成"),
        ("iscontinuous", "查询模块是否处于连续采集模式"),
        ("isarmed", "查询触发捕获状态机是否已布防"),
        ("makeconfig", "根据电压、增益或门限预算生成可供 SysConfig/应用层核对的配置"),
        ("checkrange", "检查模拟输出、偏置和摆幅是否落在 ADC 安全量程内"),
        ("calculategain", "根据电阻/反馈参数计算模拟前端闭环增益"),
        ("normalizedtoraw", "把归一化幅值、偏置和 DAC 位数换算为原始 DAC 码"),
        ("normalizetoraw", "把归一化幅值、偏置和 DAC 位数换算为原始 DAC 码"),
        ("forwardcomplexinplace", "对复数缓冲区执行原地 radix-2 前向 FFT"),
        ("forwardreal", "把实数输入转换为复数频谱并执行前向 FFT"),
        ("resamplelinear", "用线性插值把任意长度样本重采样为目标点数"),
        ("validate", "检查配置、容量、范围和指针是否满足模块前置条件"),
        ("initialize", "初始化模块对象、缓冲区和运行状态"),
        ("init", "初始化模块对象、硬件绑定和运行状态"),
        ("generate", "按配置生成完整输出数组/扫频表/波形表"),
        ("process", "对输入数组执行该模块的核心算法并写入结果结构"),
        ("acquire", "获取一个已完成且可由 CPU 安全读取的数据块"),
        ("release", "释放已消费的数据块，使 DMA 可以再次覆盖"),
        ("start", "启动一次硬件搬运、采集或输出操作"),
        ("stop", "停止当前硬件操作并恢复空闲状态"),
        ("fill", "连续产生样本并填满调用者提供的输出缓冲区"),
        ("next", "按当前相位/状态产生一个下一样本"),
        ("clear", "清除缓存、计数、标志或显示区域"),
        ("count", "返回或更新当前累计数量"),
    ]
    for token, action in api_rules:
        if token in lowered:
            return action
    tokens = split_identifier(name)
    useful = [token for token in tokens if token.lower() not in {"app", "signal", "tft", "st7789", "ili9341", "mspm0", "mspm0g3507"}]
    translated = [TOKEN_ZH.get(token.lower(), token) for token in useful]
    phrase = "、".join(translated) if translated else name
    return f"完成 `{name}` 对应的{phrase}步骤"


def classify_main(definition: Definition) -> str:
    name = definition.name.lower()
    if name == "main" or name.endswith("irqhandler") or name == "systick_handler":
        return "工程入口、调度与中断"
    if any(token in name for token in ("key", "input", "queue")):
        return "矩阵键盘与交互输入"
    if any(token in name for token in ("ad9833", "ad9850")):
        return "外部器件与阻塞式总线"
    if any(token in name for token in (
        "draw", "map", "text", "screen", "field", "row", "grid", "color",
        "trace", "lissajous", "restoreline", "eraseprevious", "preparewave",
        "displaywindow", "curve", "label",
    )):
        return "TFT、字库、波形与界面绘制"
    if any(token in name for token in ("spectrum", "fft", "harmonic", "dirichlet", "hann", "classifywaveform")):
        return "FFT、频谱、谐波与失真"
    if any(token in name for token in ("calibrat", "robust", "fit", "compensation", "correction", "removedc")):
        return "稳健估计、校准与补偿"
    if any(token in name for token in ("capture", "replay", "burst", "trigger")):
        return "触发、捕获、存储与回放"
    if any(token in name for token in ("dds", "dac", "waveform", "sweep")):
        return "DAC、DDS、波形发生与扫频"
    if any(token in name for token in ("comparator", "vca", "power", "slew", "gain")):
        return "模拟前端、比较器、增益与功耗"
    if any(token in name for token in ("adc", "acquire", "frame", "sample_rate")):
        return "ADC、DMA、连续采集与缓冲"
    if any(token in name for token in ("phase", "frequency", "period", "crossing", "lockin")):
        return "频率、相位与同步检波"
    if any(token in name for token in ("measure", "statistics", "peaktopeak", "voltage", "rms", "findbounds")):
        return "基础时域与电压测量"
    return "工程入口、调度与中断"


def module_info(definition: Definition) -> tuple[str, str]:
    if definition.relative_file == "main.c":
        return classify_main(definition), PROJECT_PURPOSE[definition.project]
    stem = Path(definition.relative_file).stem
    return MODULE_INFO.get(
        stem,
        ("数学、状态与内部公共支持", f"{stem} 文件提供的内部/公共支持"),
    )


def describe(definition: Definition, briefs: dict[tuple[str, str], str]) -> str:
    exact = MAIN_EXACT.get((definition.project, definition.name))
    if exact:
        return exact
    brief = briefs.get((Path(definition.relative_file).stem, definition.name))
    if (
        brief
        and re.search(r"[\u4e00-\u9fff]", brief)
        and not brief.startswith("执行该模块公开的功能")
    ):
        return brief.rstrip("。.") + "。"
    _, purpose = module_info(definition)
    action = generic_action(definition.name).rstrip("。.")
    if definition.relative_file == "main.c":
        return f"应用层（{PROJECT_PURPOSE[definition.project]}）：{action}。"
    visibility = "内部辅助" if definition.is_static else "公开接口"
    return f"{visibility}（{purpose}）：{action}。"


def scan() -> tuple[list[Definition], dict[tuple[str, str], str]]:
    definitions: list[Definition] = []
    briefs: dict[tuple[str, str], str] = {}
    for project in PROJECTS:
        root = FUXIAN / project / "signal_contest_template_final"
        paths = [root / "main.c"]
        paths.extend(sorted((root / "modules").glob("*.c")))
        paths.extend(sorted((root / "modules").glob("*.h")))
        for path in paths:
            if not path.exists():
                continue
            definitions.extend(extract_definitions(project, path, root))
        for header in sorted((root / "modules").glob("*.h")):
            stem = header.stem
            for name, brief in extract_briefs(header).items():
                briefs.setdefault((stem, name), brief)
    return definitions, briefs


def group_key(definition: Definition) -> tuple[str, str, str]:
    if definition.relative_file == "main.c":
        return definition.project, definition.relative_file, definition.name
    return Path(definition.relative_file).name, definition.signature, definition.name


def location_link(definition: Definition) -> str:
    target = (
        f"{definition.project}/signal_contest_template_final/"
        f"{definition.relative_file}#L{definition.line}"
    )
    label = f"{definition.project}:{definition.line}"
    excluded = (
        Path(definition.relative_file).name
        in EXCLUDED_FROM_DEBUG.get(definition.project, set())
    )
    suffix = "〔构建排除〕" if excluded else ""
    return f"[{label}]({target}){suffix}"


def render_locations(definitions: list[Definition]) -> str:
    by_file: dict[str, list[Definition]] = defaultdict(list)
    for definition in definitions:
        by_file[definition.relative_file].append(definition)
    parts = []
    for relative_file, items in sorted(by_file.items()):
        links = "、".join(location_link(item) for item in sorted(items, key=lambda item: PROJECTS.index(item.project)))
        parts.append(f"`{relative_file}`：{links}")
    return "<br>".join(parts)


def render() -> tuple[str, int, int]:
    definitions, briefs = scan()
    groups: dict[tuple[str, str, str], list[Definition]] = defaultdict(list)
    for definition in definitions:
        groups[group_key(definition)].append(definition)

    rows_by_category: dict[str, list[tuple[str, str, str, str, str]]] = defaultdict(list)
    for grouped in groups.values():
        representative = grouped[0]
        category, module_purpose = module_info(representative)
        visibility = "内部" if representative.is_static else "公开"
        if representative.name == "main":
            visibility = "入口"
        elif representative.name.endswith("IRQHandler") or representative.name == "SysTick_Handler":
            visibility = "ISR"
        signature = html.escape(representative.signature, quote=False)
        function_cell = f"`{representative.name}`<br><sub>{visibility} · `{signature}`</sub>"
        description = describe(representative, briefs).replace("|", "\\|")
        module_cell = module_purpose.replace("|", "\\|")
        locations = render_locations(grouped)
        rows_by_category[category].append(
            (Path(representative.relative_file).name, representative.name, function_cell, description, locations)
        )

    lines = [
        "# 工程函数分类复用索引",
        "",
        "> 范围：`24_A_rebuild`、`24_C_rebuild`、`22_X`、`example01`～`example08`。",
        "> 位置均为当前源码的函数**定义起始行**；点击“工程:行号”可跳到对应文件。",
        "",
        "## 使用说明",
        "",
        "- 收录 `main.c`、`modules/*.c` 以及 `modules/*.h` 中真正带函数体的 `static inline` 定义。纯头文件声明、宏、`Debug/Release` 生成副本、TI DriverLib/CMSIS 外部库函数不重复收录。",
        "- 同一冻结模块在多个工程中重复出现时合并为一条函数说明，但逐个保留工程行号；应用层 `main.c` 函数不跨工程合并。",
        "- `公开` 表示可跨文件调用；`内部` 表示 `static`，复用时通常应连同所属 `.c` 整体复制；`ISR` 的名字和向量绑定必须与本工程 SysConfig 一致。",
        "- `〔构建排除〕` 只出现在 `example05` 的旧片内 DAC/DDS 演示源：文件仍在目录中，但 `.cproject` 的 Debug 配置不编译它们。",
        "- 算法函数可优先复制 `.c/.h`；ADC/DMA、DAC、TFT、键盘、Capture、OPA/COMP 等硬件函数必须同时迁移对应 SysConfig 实例、DMA 通道、事件和引脚，不能只抄函数体。",
        "",
        "## 覆盖统计",
        "",
        f"- 定义位置总数：**{len(definitions)}**",
        f"- 合并后的函数条目：**{len(groups)}**",
        f"- 工程数：**{len(PROJECTS)}**",
        "",
        "| 工程 | 定义数 | 入口/应用层 | 模块定义 |",
        "|---|---:|---:|---:|",
    ]
    for project in PROJECTS:
        project_defs = [item for item in definitions if item.project == project]
        main_defs = [item for item in project_defs if item.relative_file == "main.c"]
        lines.append(
            f"| `{project}` | {len(project_defs)} | {len(main_defs)} | {len(project_defs) - len(main_defs)} |"
        )

    lines.extend(
        [
            "",
            "## 功能导航",
            "",
        ]
    )
    for index, category in enumerate(CATEGORY_ORDER, 1):
        lines.append(f"{index}. [{category}](#category-{index})")

    for category_index, category in enumerate(CATEGORY_ORDER, 1):
        rows = rows_by_category.get(category, [])
        lines.extend(
            [
                "",
                f"<a id=\"category-{category_index}\"></a>",
                "",
                f"## {category}",
                "",
                "| 所属文件/函数 | 具体功能 | 定义位置 |",
                "|---|---|---|",
            ]
        )
        for file_name, name, function_cell, description, locations in sorted(rows, key=lambda row: (row[0].lower(), row[1].lower())):
            lines.append(f"| `{file_name}`<br>{function_cell} | {description} | {locations} |")

    lines.extend(
        [
            "",
            "## 赛场复用建议",
            "",
            "1. 先在本索引按功能找公开入口，再点击行号查看它依赖的结构体、宏和初始化顺序。",
            "2. 纯算法模块（均值、RMS、FFT、相位、拟合、稳健估计）通常复制同名 `.c/.h` 和公共状态/数学头即可。",
            "3. 硬件模块先复制源文件，再在目标工程 SysConfig 中复现实例名；随后核对生成的 `ti_msp_dl_config.h`，最后改 `signal_config.h` 的 Fs、N、VREF、DMA 通道等参数。",
            "4. `static` 函数是实现细节。若只想复用某一内部函数，先确认它没有依赖同文件静态状态；比赛时更稳妥的做法是复制整个已验证模块。",
            "5. 行号随源码修改会变化；重新运行 `python tools/generate_function_index.py` 可刷新本索引。",
            "",
        ]
    )
    return "\n".join(lines), len(definitions), len(groups)


def main() -> None:
    content, definitions, groups = render()
    OUTPUT.write_text(content, encoding="utf-8", newline="\n")
    print(f"wrote {OUTPUT}")
    print(f"definitions={definitions} grouped_entries={groups}")


if __name__ == "__main__":
    main()
