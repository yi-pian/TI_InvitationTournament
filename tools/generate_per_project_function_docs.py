#!/usr/bin/env python3
"""Generate one detailed module/function/parameter manual for every contest project.

The source of truth is the current project tree.  The script documents every
function definition found by generate_function_index.py and separately records
module APIs and SDK/CMSIS/C-library functions called directly from main.c.
"""

from __future__ import annotations

import re
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_function_index as indexer  # noqa: E402


OUTPUT_NAME = "工程模块与函数参数详解.md"
NAVIGATION = indexer.FUXIAN / "各工程模块与函数参数详解导航.md"


PROJECT_FLOW = {
    "24_A_rebuild": "SysConfig 初始化后建立 AD9850、VCA820、ADC/DMA、键盘和 ST7789；按题号执行幅值控制、扫频带宽、压摆率或静态功耗测量，并把结果刷新到屏幕。",
    "24_C_rebuild": "双 ADC 连续采集帧经过动态采样率选择、统计量、CMSIS FFT、谐波/波形识别与突发检测，键盘切页，ST7789 显示时域和分析结果。",
    "22_X": "GPIO 控制 PLL 倍频、Y 通道增益和波形模式；双 ADC 同步采集后计算相位，并在 ILI9341 上绘制数值与李萨如轨迹。",
    "example01": "键盘设定量程/显示参数，双 ADC 同步采集，经均值、Vpp、频率和相位测量后在 ST7789 上显示双通道波形。",
    "example02": "片内 DAC/DDS 逐点产生扫频激励，双 ADC 同步采集输入/输出，计算增益和相位并绘制幅频、相频曲线。",
    "example03": "双 ADC 连续 DMA 取帧，按 CH1、CH2、双踪或 XY 模式进行缩放、局部擦除和 ST7789 重绘。",
    "example04": "综合调用采集、DDS、基础测量、FFT、稳健估计、拟合、锁相、比较器触发和三槽回放模块，形成可切页的竞赛功能平台。",
    "example05": "AD9833 产生扫频信号，双 ADC 测电压/电流幅相，计算复阻抗并拟合串联 R/L/C、谐振频率、带宽和 Q 值。",
    "example06": "单 ADC 自动量程采集，结合 RFFT、插值过零和多周期窗口得到弱信号频率、幅值，并在 ST7789 上稳定显示。",
    "example07": "DDS 产生扫频及谐波激励，双路锁相测未知通道频响，建立校正表后执行 1 kHz 幅相补偿和谐波预失真。",
    "example08": "片上 GPAMP/OPA/COMP 构成双通道模拟前端，双 ADC 同步采集并做幅相、边沿和李萨如分析，键盘切换比较器门限。",
}


@dataclass(frozen=True)
class Parameter:
    declaration: str
    type_text: str
    name: str


@dataclass(frozen=True)
class HeaderDoc:
    brief: str
    params: dict[str, tuple[str, str]]  # name -> (optional direction, text)
    returns: str


@dataclass(frozen=True)
class Call:
    line: int
    arguments: str


@dataclass(frozen=True)
class ExternalSpec:
    purpose: str
    parameters: tuple[tuple[str, str, str], ...]
    returns: str


def ext(purpose: str, parameters: list[tuple[str, str, str]], returns: str) -> ExternalSpec:
    return ExternalSpec(purpose, tuple(parameters), returns)


EXTERNAL_SPECS = {
    "SYSCFG_DL_init": ext("执行 SysConfig 生成的整板初始化，配置时钟、引脚和外设实例。", [], "无返回值。"),
    "SysTick_Config": ext("设置 Cortex-M SysTick 重装值并开启 SysTick 中断。", [("ticks", "[in]", "每次中断对应的内核时钟 tick 数；通常传 SystemCoreClock/1000 得到 1 ms。")], "0 表示配置成功，非 0 表示重装值超出 24 位范围。"),
    "__WFI": ext("执行 Wait For Interrupt，使内核休眠到中断到来。", [], "无返回值。"),
    "__NOP": ext("执行一个空操作指令，常用于极短延时或时序占位。", [], "无返回值。"),
    "DL_Common_delayCycles": ext("按 CPU 周期进行忙等待。", [("cycles", "[in]", "要等待的内核周期数；实际时间还取决于 CPU 时钟。")], "无返回值。"),
    "DL_ADC12_enableConversions": ext("允许指定 ADC12 实例开始转换。", [("adc", "[in]", "ADC 外设基地址，例如 SIGNAL_ADC_INST。")], "无返回值。"),
    "DL_ADC12_startConversion": ext("向指定 ADC12 实例发出软件启动转换命令。", [("adc", "[in]", "ADC 外设基地址。")], "无返回值。"),
    "DL_ADC12_getMemResult": ext("读取 ADC12 某个 MEM 转换结果寄存器。", [("adc", "[in]", "ADC 外设基地址。"), ("memory", "[in]", "结果存储器索引，如 DL_ADC12_MEM_IDX_0。")], "返回该 MEM 中的 ADC 原始码。"),
    "DL_ADC12_getRawInterruptStatus": ext("读取 ADC12 原始中断状态。", [("adc", "[in]", "ADC 外设基地址。"), ("mask", "[in]", "要查询的 ADC 中断位掩码。")], "返回与掩码相交的原始中断状态位。"),
    "DL_ADC12_clearInterruptStatus": ext("清除 ADC12 指定中断标志。", [("adc", "[in]", "ADC 外设基地址。"), ("mask", "[in]", "要清除的中断位掩码。")], "无返回值。"),
    "DL_DAC12_output12": ext("把 12 位数字码写到片内 DAC12 输出。", [("dac", "[in]", "DAC 外设基地址。"), ("code", "[in]", "0～4095 的 12 位输出码。")], "无返回值。"),
    "DL_TimerG_setCoreHaltBehavior": ext("设置调试器暂停内核时 TimerG 的运行/冻结行为。", [("timer", "[in]", "TimerG 外设基地址。"), ("behavior", "[in]", "调试暂停时的计数行为枚举。")], "无返回值。"),
    "DL_GPIO_setPins": ext("把 GPIO 端口中掩码指定的引脚置高。", [("port", "[in]", "GPIO 端口基地址。"), ("pins", "[in]", "要置高的引脚位掩码，可按位或组合。")], "无返回值。"),
    "DL_GPIO_clearPins": ext("把 GPIO 端口中掩码指定的引脚置低。", [("port", "[in]", "GPIO 端口基地址。"), ("pins", "[in]", "要置低的引脚位掩码。")], "无返回值。"),
    "DL_DMA_enableInterrupt": ext("开启 DMA 控制器指定通道的完成/错误中断。", [("dma", "[in]", "DMA 控制器基地址。"), ("channel", "[in]", "DMA 通道编号或通道枚举。")], "无返回值。"),
    "DL_COMP_getPendingInterrupt": ext("读取比较器已仲裁的待处理中断来源。", [("comp", "[in]", "COMP 外设基地址。")], "返回待处理中断枚举；无事件时返回相应的 NONE 值。"),
    "DL_COMP_getRawInterruptStatus": ext("读取比较器原始中断状态。", [("comp", "[in]", "COMP 外设基地址。"), ("mask", "[in]", "要查询的比较器中断位掩码。")], "返回与掩码相交的原始状态位。"),
    "DL_COMP_clearInterruptStatus": ext("清除比较器指定中断状态。", [("comp", "[in]", "COMP 外设基地址。"), ("mask", "[in]", "要清除的上升/下降沿等中断位。")], "无返回值。"),
    "DL_COMP_enableInterrupt": ext("使能比较器指定事件中断。", [("comp", "[in]", "COMP 外设基地址。"), ("mask", "[in]", "要使能的事件位掩码。")], "无返回值。"),
    "DL_COMP_setDACCode0": ext("设置比较器内部参考 DAC 的 CODE0 门限。", [("comp", "[in]", "COMP 外设基地址。"), ("code", "[in]", "内部参考 DAC 码；对应电压需结合 VDDA 和参考配置换算。")], "无返回值。"),
    "DL_COMP_setOutputInterruptEdge": ext("选择比较器输出在哪种边沿产生事件/中断。", [("comp", "[in]", "COMP 外设基地址。"), ("edge", "[in]", "上升、下降或双边沿配置枚举。")], "无返回值。"),
    "NVIC_ClearPendingIRQ": ext("清除 NVIC 中指定 IRQ 的挂起状态。", [("irqn", "[in]", "中断号枚举。")], "无返回值。"),
    "NVIC_EnableIRQ": ext("在 NVIC 中使能指定 IRQ。", [("irqn", "[in]", "中断号枚举。")], "无返回值。"),
    "NVIC_DisableIRQ": ext("在 NVIC 中屏蔽指定 IRQ。", [("irqn", "[in]", "中断号枚举。")], "无返回值。"),
    "arm_mean_f32": ext("计算 float32 数组均值。", [("source", "[in]", "输入浮点样本数组。"), ("count", "[in]", "数组元素数。"), ("result", "[out]", "均值输出地址。")], "无返回值；结果写入 result。"),
    "arm_min_f32": ext("在 float32 数组中查找最小值及其首次出现的位置。", [("source", "[in]", "输入浮点样本数组。"), ("count", "[in]", "数组元素数，必须大于 0。"), ("result", "[out]", "最小值输出地址。"), ("index", "[out]", "最小值首次出现的数组下标输出地址。")], "无返回值；结果和下标写入输出地址。"),
    "arm_max_f32": ext("在 float32 数组中查找最大值及其首次出现的位置。", [("source", "[in]", "输入浮点样本数组。"), ("count", "[in]", "数组元素数，必须大于 0。"), ("result", "[out]", "最大值输出地址。"), ("index", "[out]", "最大值首次出现的数组下标输出地址。")], "无返回值；结果和下标写入输出地址。"),
    "arm_offset_f32": ext("给 float32 数组每个元素加同一偏置。", [("source", "[in]", "输入数组。"), ("offset", "[in]", "逐元素相加的常量。"), ("destination", "[out]", "输出数组，可按 CMSIS 约束与输入重叠。"), ("count", "[in]", "处理元素数。")], "无返回值。"),
    "arm_rms_f32": ext("计算 float32 数组的均方根。", [("source", "[in]", "输入浮点样本数组。"), ("count", "[in]", "样本数。"), ("result", "[out]", "RMS 输出地址。")], "无返回值；结果写入 result。"),
    "arm_cfft_q15": ext("执行 Q15 复数 FFT/IFFT。", [("instance", "[in]", "与 FFT 点数匹配的 CMSIS CFFT 描述符。"), ("data", "[in/out]", "交错实部/虚部 Q15 缓冲区，原地覆盖。"), ("ifft_flag", "[in]", "0 为 FFT，1 为 IFFT。"), ("bit_reverse", "[in]", "1 表示输出执行位反转重排。")], "无返回值。"),
    "arm_cmplx_mag_q15": ext("计算交错 Q15 复数数组的幅值。", [("source", "[in]", "交错实部/虚部 Q15 复数输入数组。"), ("destination", "[out]", "Q15 幅值输出数组。"), ("count", "[in]", "复数样本数，不是交错标量数。")], "无返回值；每个复数样本对应一个非负幅值。"),
    "arm_rfft_fast_init_f32": ext("初始化 float32 快速实数 FFT 实例。", [("instance", "[out]", "待初始化的 RFFT 实例。"), ("fft_length", "[in]", "支持的实数 FFT 点数。")], "返回 arm_status，ARM_MATH_SUCCESS 表示成功。"),
    "arm_rfft_fast_f32": ext("执行 float32 快速实数 FFT/IFFT。", [("instance", "[in]", "已初始化的 RFFT 实例。"), ("input", "[in/out]", "时域输入；实现可能使用其作为工作区。"), ("output", "[out]", "CMSIS 打包格式的频域输出。"), ("ifft_flag", "[in]", "0 为正变换，1 为逆变换。")], "无返回值。"),
    "arm_sqrt_f32": ext("计算单精度平方根并报告负数域错误。", [("input", "[in]", "被开方数。"), ("result", "[out]", "平方根输出地址。")], "返回 arm_status；非负输入通常为 ARM_MATH_SUCCESS。"),
    "arm_sin_f32": ext("计算弧度制单精度正弦。", [("angle", "[in]", "弧度制角度。")], "返回 sin(angle)。"),
    "arm_cos_f32": ext("计算弧度制单精度余弦。", [("angle", "[in]", "弧度制角度。")], "返回 cos(angle)。"),
    "sinf": ext("计算弧度制单精度正弦。", [("angle", "[in]", "弧度制角度。")], "返回单精度正弦值。"),
    "cosf": ext("计算弧度制单精度余弦。", [("angle", "[in]", "弧度制角度。")], "返回单精度余弦值。"),
    "sqrtf": ext("计算单精度平方根。", [("value", "[in]", "非负被开方数；负数会产生 NaN/域错误。")], "返回平方根。"),
    "fabsf": ext("取单精度浮点绝对值。", [("value", "[in]", "输入浮点数。")], "返回非负绝对值。"),
    "atan2f": ext("由 y、x 计算带象限的反正切。", [("y", "[in]", "纵向分量/虚部。"), ("x", "[in]", "横向分量/实部。")], "返回 [-π, π] 范围的弧度角。"),
    "floorf": ext("向负无穷方向取整。", [("value", "[in]", "输入浮点数。")], "返回不大于输入的最大整数值（仍为 float）。"),
    "ceilf": ext("向正无穷方向取整。", [("value", "[in]", "输入浮点数。")], "返回不小于输入的最小整数值（仍为 float）。"),
}


FUNCTION_PURPOSE_EXACT = {
    "App_RemoveDC": "调用 CMSIS-DSP 求一帧均值，并用原地 offset 运算从每个样本减去该直流分量；均值有限时返回 true。",
    "EstimateHannPeakAmplitude": "把目标频率换算为分数 bin，用 Hann 窗理论响应补偿最近整数 bin 的幅度衰减，得到峰值电压估计。",
    "App_BackgroundColor": "根据像素是否落在示波网格主轴、分格线或普通背景上，返回擦线时应恢复的 RGB565 颜色。",
    "App_AbsMilli": "取浮点值绝对值、乘 1000 并四舍五入为无符号整数，供界面显示毫单位数值。",
    "AD9850_MaxU32": "返回两个无符号 32 位数中的较大值，用于保证时序下限。",
    "AD9850_CyclesToUsCeil": "把参考时钟周期数向上取整换算为微秒，避免 GPIO 脉冲短于 AD9850 时序要求。",
    "AD9850_WriteLine": "通过平台回调设置 AD9850 的指定控制线电平。",
    "AD9850_DelayEdge": "按已计算的最小边沿间隔延时，满足 AD9850 串行写入时序。",
    "AD9850_Pulse": "在指定控制线上产生一次低/高（或高/低）脉冲。",
    "AD9850_WriteBit": "按 LSB-first 协议向 AD9850 DATA/W_CLK 串行写入一位。",
    "AD9850_WriteByteLsbFirst": "按最低位先发的顺序向 AD9850 连续写入一个字节。",
    "AD9850_WriteFrame": "向 AD9850 写入 32 位频率字和 8 位控制字，并用 FQ_UD 锁存。",
    "AD9850_ComputeTuningWord": "根据输出频率和参考时钟计算并舍入 32 位 DDS 调谐字。",
    "AD9850_Reset": "按器件要求产生 RESET/W_CLK/FQ_UD 序列，使 AD9850 回到已知状态。",
    "AD9850_SetOutput": "组合频率调谐字、相位/控制位并更新 AD9850 输出。",
    "AD9850_SetPowerDown": "设置或清除 AD9850 控制字中的掉电位。",
    "AD9850_MSPM0_GetPin": "把平台抽象的 AD9850 信号枚举映射到 MSPM0 GPIO 端口和引脚掩码。",
    "AD9850_MSPM0_WriteLine": "用 DriverLib 把 AD9850 某根控制线置高或置低。",
    "AD9850_MSPM0_DelayUs": "把微秒延时换算为 CPU 周期并执行忙等待。",
    "AD9833_SetOutput": "把目标频率、相位和波形模式写入 AD9833 频率/相位/控制寄存器。",
    "AD9833_SetDualOutput": "分别设置两路 AD9833 的频率与相位，用公共基准形成指定相位差。",
    "keypad_ghost_possible": "检查当前行列按下组合是否可能形成矩阵键盘鬼键；发现多行多列同时有效时拒绝上报。",
    "SignalMatrixKeypad4x4_Scan": "逐行驱动 4×4 矩阵键盘并读取四列，执行消抖、按下/释放状态更新和事件入队。",
    "SignalMatrixKeypad4x4_ReadNewSymbol": "从键盘事件队列读取一次新按下事件并映射为字符；没有新事件时返回 false。",
    "mspm0g3507_keypad_drive_row": "把指定键盘行拉到有效电平，同时释放其余行。",
    "mspm0g3507_keypad_read_column": "读取指定键盘列 GPIO 电平并转换为按下/未按下状态。",
    "mspm0g3507_keypad_delay_us": "为键盘行切换和消抖提供微秒级忙等待。",
    "SignalRobustP2P_Swap": "交换两个浮点样本，供选择算法原地分区。",
    "SignalRobustP2P_Select": "用快速选择在工作数组中取得第 k 小元素，无需完整排序。",
    "SignalRobustP2P_Quantile": "按分位点计算相邻次序统计量并线性插值。",
    "crossing": "在相邻采样点间对指定电平做线性插值，返回更精细的交越位置。",
    "SignalStaticPower_Calculate": "由供电电压和电流计算静态功耗，并按输出指针返回电流与功率。",
    "cs": "通过总线回调设置 TFT 片选电平。",
    "set_dc": "设置 ST7789 的 D/C 引脚，选择命令阶段或数据阶段。",
    "set_bl": "设置 ST7789 背光控制引脚。",
    "set_reset": "设置 ST7789 硬件复位引脚。",
    "delay_ms": "调用平台延时回调等待指定毫秒数。",
    "command_data_unlocked": "在已经取得总线锁的前提下发送一条 TFT 命令及其可选参数。",
    "command_data": "取得总线锁后发送一条 TFT 命令及其可选参数，再释放总线。",
    "tft_is_valid": "检查 ILI9341 上下文及必需的 SPI 写入、D/C 控制回调是否有效。",
    "tft_lock": "若配置了总线加锁回调则取得锁，防止一条 TFT 事务被其他代码打断。",
    "tft_unlock": "若配置了总线解锁回调则释放锁。",
    "tft_transaction_unlocked": "在外层已持锁时设置 D/C 和片选，通过写回调发送一个命令或数据缓冲区。",
    "tft_transaction": "取得总线锁、执行一次 ILI9341 命令/数据事务并释放锁。",
    "tft_command_data_unlocked": "在已持锁时先发送命令字节，再按需发送参数数据。",
    "tft_command_data": "取得总线锁后发送 ILI9341 命令及参数，再释放锁。",
    "TFT_ST7789_WriteCommand": "通过 SPI 向 ST7789 发送一个命令字节。",
    "TFT_ST7789_WriteData": "通过 SPI 向 ST7789 发送数据缓冲区。",
    "set_rotation_raw": "写 ST7789 MADCTL 寄存器并更新旋转后的逻辑宽高。",
    "TFT_ST7789_SetRotation": "切换屏幕旋转方向并同步坐标系宽度、高度和偏移。",
    "TFT_ST7789_SetBacklight": "通过平台回调开启或关闭屏幕背光。",
    "TFT_ST7789_GetWidth": "返回当前旋转方向下的逻辑屏幕宽度，单位像素。",
    "TFT_ST7789_GetHeight": "返回当前旋转方向下的逻辑屏幕高度，单位像素。",
    "TFT_ST7789_SetAddressWindow": "设置 ST7789 后续显存写入的矩形地址窗口。",
    "draw_pixels": "在已设置的地址窗口中批量发送 RGB565 像素。",
    "TFT_ST7789_DrawPixel": "裁剪坐标后设置 1×1 地址窗口并写入一个 RGB565 像素。",
    "TFT_ST7789_DrawLine": "使用整数增量算法在两点之间绘制 RGB565 线段。",
    "TFT_ST7789_DrawRect": "绘制矩形的四条边，不填充内部。",
    "TFT_ST7789_FillRect": "裁剪目标区域后用同一 RGB565 颜色填充矩形。",
    "TFT_ST7789_FillScreen": "以指定 RGB565 颜色填充整块屏幕。",
    "TFT_ST7789_DrawRGB565": "把调用者提供的 RGB565 像素块写到指定矩形区域。",
    "TFT_ST7789_DrawMonoBitmap": "按单色位图的前景/背景位在屏幕上绘制图形，并支持透明背景。",
    "TFT_ST7789_GetFont": "按字体枚举返回内置字模描述符。",
    "TFT_ST7789_GetFontMetrics": "读取指定字体的字符宽度和高度。",
    "TFT_ST7789_DrawChar": "从字库取出一个字符位图，并按前景色、背景色和透明设置绘制。",
    "TFT_ST7789_DrawString": "逐字符绘制 NUL 结尾字符串，并按配置处理换行和透明背景。",
    "TFT_ST7789_AppendUint32": "把无符号整数转换为十进制字符并追加到文本缓冲区。",
    "TFT_ST7789_DrawInt32": "把有符号 32 位整数格式化后用指定字体绘制。",
    "TFT_ST7789_DrawFloat": "按指定小数位格式化浮点数后用指定字体绘制。",
    "SignalDualADC_SetDMATransferMode": "为双路 ADC 的 DMA 通道设置单次或循环搬运模式。",
    "SignalDualADC_ConfigureDMABlock": "把某个连续采集块的 A/B 目标地址和传输长度写入两路 DMA。",
    "SignalDualADC_GetChannelA": "返回本次双 ADC 采集所绑定的 A 通道缓冲区首地址。",
    "SignalDualADC_GetChannelB": "返回本次双 ADC 采集所绑定的 B 通道缓冲区首地址。",
    "DMA_IRQHandler": "处理双 ADC DMA 完成事件；同步两通道完成状态并发布可读取的完整块。",
    "DAC12_IRQHandler": "处理 DAC DMA/转换完成事件并更新循环输出状态。",
    "DataRange": "遍历输入样本，计算显示缩放所需的最小值和最大值。",
    "ExpandFlatRange": "当波形几乎为常数时扩展上下界，避免显示映射除以零。",
    "PlotX": "把样本序号按总点数线性映射到波形绘图区 X 像素坐标。",
    "SignalTFTWaveformST7789_MapY": "把样本值按量程上下界映射并裁剪为绘图区 Y 像素坐标。",
    "SignalTFTWaveformST7789_GetEnvelopeColumn": "汇总某个屏幕列对应的多个样本，得到该列的最小/最大包络。",
    "DrawDecorations": "绘制波形区域的边框、中线、刻度或网格装饰。",
    "DrawDecimated": "样本多于屏幕宽度时按抽取点绘制折线。",
    "DrawEnvelope": "每个屏幕列绘制样本最小/最大包络，保留高频尖峰。",
    "SignalTFTWaveformST7789_Draw": "计算量程并在抽取折线与列包络模式间选择，完成整幅波形绘制。",
    "SignalTimerCapture_Delta": "考虑计数器回绕，计算两个捕获时间戳之间的 tick 差。",
    "SignalTimerCapture_MeanPeriod": "对多个相邻捕获间隔求平均周期 tick。",
    "SignalTimerCapture_MSPM0_GetResult": "从硬件 Capture 状态生成周期、频率和有效性结果快照。",
    "SIGNAL_CAPTURE_INST_IRQHandler": "读取并清除 Timer Capture 中断，把边沿时间戳交给测频状态机。",
    "SignalDualADCPhase_FindBounds": "遍历 ADC 原始码，返回最小值和最大值供动态中点/迟滞计算。",
    "SignalDualADCPhase_FindRisingCrossings": "以动态中点和迟滞搜索上升过零点，并记录交越样本索引。",
    "SignalDualADCPhase_AveragePeriod": "对同一通道相邻上升过零间隔求平均周期样本数。",
    "SignalDualADCPhase_AverageWrapped": "对已按周期回绕的多组相位延迟做圆周意义上的平均。",
    "SignalPhase_WrapDegrees": "把任意角度规范到模块约定的一个 360° 区间。",
    "SignalPhase_SetResult": "统一写入相位结果结构体中的角度、延迟、频率和有效状态。",
    "SignalMathBackend_SqrtF": "通过当前选择的数学后端计算单精度平方根。",
    "SignalMathBackend_Atan2F": "通过当前选择的数学后端计算带象限单精度反正切。",
    "SignalMath_ClampF32": "把浮点输入限制在给定最小值和最大值之间。",
    "SignalMath_IsPowerOfTwo": "判断无符号整数是否为非零的 2 次幂。",
    "SignalFFT_IsPowerOfTwo": "检查 FFT 点数是否为至少 2 的 2 次幂。",
    "SignalFFT_Swap": "交换两个复数元素，供 FFT 位反转/蝶形重排。",
    "SignalFFT_ForwardComplexReference": "使用项目自带 radix-2 蝶形实现执行未归一化复数前向 FFT。",
    "SignalFFT_GetQ15Instance": "按 FFT 点数选择对应的 CMSIS Q15 CFFT 实例描述符。",
    "SignalFFT_ForwardComplexCMSISQ15": "把浮点复数缩放为 Q15，调用 CMSIS CFFT，再换回浮点频谱。",
    "SignalFFT_GetQ31Instance": "按 FFT 点数选择对应的 CMSIS Q31 CFFT 实例描述符。",
    "SignalFFT_ForwardComplexCMSISQ31": "把浮点复数缩放为 Q31，调用 CMSIS CFFT，再换回浮点频谱。",
    "SignalFFT_GetF32Instance": "按 FFT 点数选择对应的 CMSIS float32 CFFT 实例描述符。",
    "SignalFFT_ForwardComplexCMSISF32": "把数据整理为 CMSIS 交错格式并执行 float32 复数前向 FFT。",
    "SignalMAD_Swap": "交换两个浮点样本，供 MAD 快速选择算法分区。",
    "SignalMAD_SelectKth": "用原地快速选择取得第 k 小样本。",
    "SignalMAD_Median": "根据奇偶样本数调用选择算法计算中位数。",
    "SignalMedianFilter_InsertionSort": "对小滑窗工作数组执行原地插入排序。",
    "SignalRobustRMS_Clamp": "把样本限制在 Winsorize 的上下分位边界内。",
    "SignalSineFit3_Solve": "对 3×4 增广矩阵执行带主元的高斯消元，解出三参数正弦拟合系数。",
    "SignalSineFit4_Evaluate": "在给定候选频率下完成三参数线性拟合并返回残差，供四参数频率搜索。",
    "configuration_valid": "检查捕获/回放配置中的缓冲区、槽数、长度和回调是否完整。",
    "slot_pointer": "返回指定捕获槽的可写样本区首地址。",
    "const_slot_pointer": "返回指定捕获槽的只读样本区首地址。",
    "roll_search_window": "滚动连续捕获搜索窗口，保留跨块触发所需的尾部样本。",
    "SignalSNR_IsExcluded": "判断频谱 bin 是否位于直流、信号带或其他排除区，从而不计入噪声。",
    "GlyphIndex": "把可显示 ASCII 字符映射为小字库中的字模索引。",
    "SignalWindow_Coefficient": "按窗口类型和样本位置计算 Rect/Hann/Hamming/Blackman 系数。",
    "i2c_has_error": "检查 I2C 控制器状态是否出现 NACK、仲裁丢失或总线错误。",
    "i2c_wait_idle": "阻塞等待 I2C 控制器空闲，并在超时或错误时退出。",
    "i2c_wait_raw": "阻塞等待指定 I2C 原始中断标志，并处理超时/错误。",
    "MSPM0_EXT_SPI_Transfer8": "使用阻塞轮询完成 SPI 字节数组发送、接收或全双工交换。",
    "MSPM0_EXT_SPI_Write16MSB": "按高字节在前的顺序通过 SPI 写出一个 16 位字。",
    "MSPM0_EXT_I2C_Write": "向 7 位地址 I2C 从机阻塞写入字节序列。",
    "MSPM0_EXT_I2C_Read": "从 7 位地址 I2C 从机阻塞读取指定字节数。",
    "MSPM0_EXT_I2C_WriteRead": "执行不释放总线的 I2C 写后重复起始读事务。",
    "signal_frc_wrap_degrees": "把频响相位角规范到插值和补偿采用的 360° 区间。",
    "SignalGPAMP_Apply": "校验 GPAMP 配置并把计算结果写入预算/状态对象；真正寄存器初始化仍由 SysConfig 完成。",
    "SignalOPA_Apply": "校验 OPA 配置并生成增益/量程结果；真正寄存器初始化仍由 SysConfig 完成。",
}


SEMANTIC_OBJECTS = [
    ("continuoussnapshot", "连续 DMA 块序号与完成块的一致性快照"),
    ("continuousblocksequence", "连续 DMA 已发布块序号"),
    ("continuouscompletedblock", "连续 DMA 最近完成块索引"),
    ("addresswindow", "TFT 显存写入窗口"),
    ("measurementstate", "测量状态文本"),
    ("gainrestoredvoltage", "经前端增益还原后的电压"),
    ("waveformvoltage", "采集波形的电压数组"),
    ("samplerate", "采样率"),
    ("updaterate", "更新率"),
    ("frequency", "频率"),
    ("phase", "相位"),
    ("harmonic", "谐波指标"),
    ("spectrum", "频谱"),
    ("statistics", "统计量"),
    ("peaktopeak", "峰峰值"),
    ("burstgateedge", "突发门控边沿"),
    ("dirichletkernel", "Dirichlet 核频谱泄漏项"),
    ("hannresponse", "Hann 窗频率响应"),
    ("displaywindow", "周期显示窗口"),
    ("lissajous", "李萨如轨迹"),
    ("comparator", "比较器门限/模式"),
    ("capturetrigger", "捕获触发事件"),
    ("capture", "触发捕获"),
    ("replay", "已存波形回放"),
    ("calibrat", "标定参数"),
    ("waveform", "波形输出/类型"),
    ("channel", "通道数据"),
    ("grid", "波形网格"),
    ("page", "显示页面"),
    ("screen", "屏幕内容"),
    ("staticui", "静态界面"),
    ("staticlayout", "静态界面布局"),
    ("static", "无需频繁变化的界面元素"),
    ("dynamic", "动态测量界面"),
    ("text", "文本"),
    ("float", "浮点数"),
    ("integer", "整数"),
    ("int", "整数"),
    ("value", "测量值"),
    ("key", "键盘事件"),
    ("input", "用户输入"),
    ("bounds", "上下界"),
    ("range", "数值范围"),
    ("line", "线段/曲线"),
    ("point", "数据点"),
    ("wave", "波形"),
    ("status", "状态"),
    ("power", "静态功耗"),
    ("rms", "有效值"),
    ("adc", "ADC 数据/配置"),
    ("dac", "DAC 数据/配置"),
    ("dds", "DDS 输出"),
    ("ratio", "幅值比/频率比"),
    ("progress", "运行进度"),
    ("response", "频率响应"),
    ("frame", "采样帧"),
]


def function_description(
    definition: indexer.Definition,
    briefs: dict[tuple[str, str], str],
) -> str:
    exact = FUNCTION_PURPOSE_EXACT.get(definition.name)
    if exact:
        return exact
    description = indexer.describe(definition, briefs)
    if "对应的" not in description:
        return description
    lowered = definition.name.lower().replace("_", "")
    object_text = next((text for token, text in SEMANTIC_OBJECTS if token in lowered), "模块专用状态和数据")
    verb_rules = [
        ("convert", "换算"), ("calculate", "计算"), ("compute", "计算"),
        ("measure", "测量"), ("analyze", "分析"), ("classify", "识别"),
        ("detect", "检测"), ("find", "查找"), ("select", "选择"),
        ("draw", "绘制"), ("render", "渲染"), ("refresh", "刷新"),
        ("prepare", "准备"), ("restore", "恢复"), ("erase", "擦除"),
        ("set", "设置"), ("get", "读取"), ("read", "读取"), ("write", "写入"),
        ("apply", "应用"), ("handle", "处理"), ("service", "维护"),
        ("queue", "入队"), ("consume", "消费"), ("append", "追加"),
        ("parse", "解析"), ("begin", "开始编辑"), ("commit", "提交"),
        ("cancel", "取消"), ("arm", "布防"), ("replay", "回放"),
        ("capture", "采集"), ("map", "映射"), ("clamp", "限幅"),
        ("delay", "延时"), ("interpolate", "插值"), ("wrap", "角度回绕"),
        ("name", "返回名称文本"), ("fail", "进入故障处理并显示错误"),
    ]
    verb = next((text for token, text in verb_rules if token in lowered), "处理")
    layer = "文件内部辅助函数" if definition.is_static else "模块公开接口"
    return f"作为{layer}，{verb}{object_text}；具体输入、输出和量纲见下方参数及返回值说明。"


PARAMETER_MEANINGS = {
    "config": "模块配置结构体，包含初始化所需的采样率、时钟、容量或硬件参数；调用期间必须保持可读。",
    "context": "驱动/算法上下文或回调用户数据，用于保存实例状态并避免依赖全局变量。",
    "tft": "TFT 驱动上下文，保存总线回调、屏幕尺寸、旋转方向和当前状态。",
    "keypad": "4×4 矩阵键盘对象，保存 GPIO 回调、消抖计数和按键状态。",
    "dds": "DDS 状态对象，保存波表、相位累加器、相位步进和更新率。",
    "table": "波表、校准表或频响表对象；具体元素语义由所属模块决定。",
    "ring": "ADC 环形缓冲对象，包含存储区、读写索引、计数和溢出状态。",
    "capture": "触发捕获/回放状态对象，保存布防、槽位和有效样本信息。",
    "device": "外部器件驱动对象，保存 GPIO/SPI 回调、时钟和器件状态。",
    "platform": "与 MSPM0 硬件相关的平台回调集合，例如写引脚和微秒延时函数。",
    "module": "模块状态或模块元数据对象。",
    "calibration": "标定参数/标定表对象，保存增益、偏移、延迟或频响修正数据。",
    "comparator": "比较器配置/预算对象。",
    "gpamp": "GPAMP 配置/预算对象。",
    "opa": "OPA 配置/预算对象。",
    "stats": "统计结果结构体，用于返回均值、方差、标准差等成组结果。",
    "result": "计算结果输出地址；成功返回后由函数写入。",
    "output": "输出缓冲区或输出结果地址，其容量必须满足相邻 count/capacity 参数。",
    "destination": "目标缓冲区，函数把转换或复制结果写到这里。",
    "source": "源数据缓冲区，只在函数执行期间读取。",
    "data": "待处理的数据/总线字节缓冲区；是否原地修改由参数方向和函数说明决定。",
    "buffer": "调用者提供的工作或输出缓冲区，生命周期必须覆盖本次操作。",
    "buffer_a": "A 通道/第一路工作缓冲区。",
    "buffer_b": "B 通道/第二路工作缓冲区。",
    "channel_a": "同步采集 A 通道样本缓冲区；硬件采集函数会把 ADC 码写入其中。",
    "channel_b": "同步采集 B 通道样本缓冲区；硬件采集函数会把 ADC 码写入其中。",
    "samples": "时域样本数组；样本的单位由模块决定（ADC 码、伏特或归一化值）。",
    "input_samples": "只读输入样本数组。",
    "output_samples": "输出样本数组。",
    "raw_codes": "ADC 原始码数组，未完成参考电压、偏置或前端增益换算。",
    "raw": "原始码/原始数据缓冲区。",
    "voltage_v": "以伏特为单位的电压值或电压数组。",
    "centered_voltage": "已去除直流中点的电压样本。",
    "output_centered_v": "去直流后的输出电压数组，单位为伏特。",
    "sample_rate_hz": "采样率，单位 Hz；必须与实际定时器触发率一致。",
    "actual_rate_hz": "输出参数：由定时器整数分频后实际实现的速率，单位 Hz。",
    "configured_trigger_rate_hz": "已配置的实际 ADC 触发率，单位 Hz。",
    "update_rate_hz": "DAC/DDS 更新率，单位 Hz。",
    "frequency_hz": "频率，单位 Hz。",
    "output_frequency_hz": "目标或已实现的输出频率，单位 Hz。",
    "target_frequency_hz": "待搜索、拟合或校正的目标频率，单位 Hz。",
    "measured_frequency_hz": "实测频率，单位 Hz。",
    "actual_frequency_hz": "量化后实际得到的频率，单位 Hz。",
    "reference_clock_hz": "器件/计数器参考时钟，单位 Hz。",
    "mclk_hz": "AD9833 主时钟频率，单位 Hz，用于换算 28 位频率字。",
    "fundamental_hz": "已知或估计的基波频率，单位 Hz。",
    "frequencies_hz": "频率点数组，单位 Hz。",
    "output_hz": "输出频率数组或频率结果，单位 Hz。",
    "count": "本次参与处理的元素/样本数量。",
    "length": "数据、文本或波表的有效长度。",
    "sample_count": "本次采集或处理的样本数。",
    "samples_per_block": "连续 DMA 每个块包含的样本数。",
    "block_count": "连续 DMA 使用的块数量。",
    "frame_count": "要累计或平均的采集帧数。",
    "capacity": "目标数组/队列的可用元素容量，用于防止越界写入。",
    "workspace_count": "工作区可容纳的元素数。",
    "spectrum_capacity": "复频谱输出缓冲区容量。",
    "magnitude_capacity": "幅度谱输出缓冲区容量。",
    "crossing_capacity": "过零位置数组可容纳的最大元素数。",
    "event_capacity": "事件数组可容纳的最大事件数。",
    "position_capacity": "位置数组可容纳的最大元素数。",
    "sample": "单个样本值。",
    "value": "要设置、换算或限制的输入值；量纲见函数名和相邻参数。",
    "minimum": "下界/最小值输出或搜索范围最小值。",
    "maximum": "上界/最大值输出或搜索范围最大值。",
    "low": "低电平、下限或区间较小端点。",
    "high": "高电平、上限或区间较大端点。",
    "threshold": "判决门限；单位与输入数据一致。",
    "threshold_v": "电压判决门限，单位 V。",
    "hysteresis": "迟滞量；单位与被比较的数据一致。",
    "hysteresis_v": "比较器/过零迟滞电压，单位 V。",
    "scale_minimum": "显示或归一化映射的输入最小值。",
    "scale_maximum": "显示或归一化映射的输入最大值。",
    "amplitude_fraction": "相对于 DAC 满量程的峰值幅度比例，通常限定在 0～1。",
    "offset_fraction": "相对于 DAC 满量程的直流偏置比例，通常限定在 0～1。",
    "duty_fraction": "一个周期内高电平所占比例，0～1 对应 0%～100%。",
    "symmetry_fraction": "三角/锯齿等波形上升段占周期的比例，范围 0～1。",
    "phase_cycles": "以周期为单位的初相位，1.0 表示 360°。",
    "phase_deg": "相位角，单位度。",
    "phase_degrees": "相位角，单位度。",
    "measured_phase_deg": "实测相位角，单位度。",
    "measured_phase_b_minus_a_deg": "实测 B 相对 A 的相位差，单位度。",
    "expected_phase_b_minus_a_deg": "标定时预期的 B-A 相位差，单位度。",
    "corrected_phase_b_minus_a_deg": "完成固定通道延迟补偿后的 B-A 相位差，单位度。",
    "phase_code": "外部 DDS 相位寄存器编码。",
    "phase_code_12bit": "AD9833 12 位相位字，低 12 位对应 0～360°。",
    "phase_a_code_12bit": "AD9833 A 通道/基准相位的 12 位相位字。",
    "phase_difference_code_12bit": "两路 AD9833 输出之间的 12 位相位差编码。",
    "tuning_word": "DDS 频率调谐字；位宽和换算关系由器件决定。",
    "initial_phase": "DDS 相位累加器的初始相位码。",
    "timer_count": "定时器当前/目标计数值，单位为 timer tick。",
    "counter_modulus": "自由运行计数器回绕模数，用于正确计算跨回绕时间差。",
    "delta_ticks": "两个捕获边沿之间的计数差，单位 timer tick。",
    "mean_ticks": "多周期平均后的周期计数，单位 timer tick。",
    "timestamps": "硬件捕获时间戳数组，元素单位为 timer tick。",
    "timestamp_count": "时间戳数组中的有效元素数。",
    "milliseconds": "延时时间，单位 ms。",
    "microseconds": "延时时间，单位 µs。",
    "delay_us": "忙等待时间，单位 µs。",
    "cycles": "CPU/外设时钟周期数。",
    "x": "横坐标、第一操作数或 X 通道值；显示函数中单位为像素。",
    "y": "纵坐标、第二操作数或 Y 通道值；显示函数中单位为像素。",
    "x0": "线段/矩形起点横坐标，单位像素。",
    "y0": "线段/矩形起点纵坐标，单位像素。",
    "x1": "线段/矩形终点横坐标，单位像素。",
    "y1": "线段/矩形终点纵坐标，单位像素。",
    "width": "图形、位图或显示区域宽度，通常单位为像素。",
    "height": "图形、位图或显示区域高度，通常单位为像素。",
    "plot_y": "波形绘图区顶部 Y 坐标，单位像素。",
    "plot_height": "波形绘图区高度，单位像素。",
    "screen_y": "屏幕 Y 坐标，单位像素。",
    "color": "RGB565 颜色值。",
    "foreground": "文字/前景的 RGB565 颜色。",
    "background": "背景的 RGB565 颜色。",
    "fg": "前景 RGB565 颜色。",
    "bg": "背景 RGB565 颜色。",
    "transparent_background": "是否跳过背景像素：true 为透明文字，false 会填背景色。",
    "transparent": "是否启用透明背景绘制。",
    "rotation": "屏幕旋转枚举，决定宽高和坐标方向。",
    "font": "字体描述符，包含字模尺寸、字符范围和位图地址。",
    "text": "以 NUL 结尾的待显示/处理字符串。",
    "label": "界面字段标签字符串。",
    "unit": "数值后显示的单位字符串。",
    "character": "待绘制或转换的字符。",
    "symbol": "矩阵键盘映射得到的符号字符。",
    "key": "按键值或键盘符号。",
    "key_index": "4×4 键盘的线性按键索引，通常为 0～15。",
    "row": "键盘/字模/表格的行索引。",
    "column": "键盘/字模/表格的列索引。",
    "pixels": "RGB565 像素数组。",
    "bitmap": "单色或彩色位图数据。",
    "bitmap_size": "位图缓冲区字节数，用于边界检查。",
    "bytes_per_row": "位图每行占用的字节数。",
    "precision": "浮点显示的小数位数。",
    "decimal_places": "十进制小数位数。",
    "minimum_digits": "整数显示的最少位数，不足时补前导字符。",
    "wrap": "文字到达屏幕边缘后是否自动换行。",
    "command": "发送给 TFT/外部器件的命令字节或命令码。",
    "data_mode": "总线当前发送的是命令还是数据的标志。",
    "transfer_mode": "阻塞式总线的发送/接收工作模式。",
    "active": "启用/激活标志。",
    "on": "开关状态；true 为开启，false 为关闭。",
    "repeat": "是否重复/循环输出或采集。",
    "power_down": "外部 DDS 的掉电控制标志。",
    "invert_output": "是否反相比对器/逻辑输出。",
    "address_7bit": "I2C 从机 7 位地址，不包含读写位。",
    "port": "GPIO 端口基地址。",
    "pin": "GPIO 引脚编号或位掩码。",
    "cs_port": "片选 CS 所在 GPIO 端口。",
    "cs_pin": "片选 CS 引脚位掩码。",
    "fsync_port": "AD9833 FSYNC 所在 GPIO 端口。",
    "fsync_pin": "AD9833 FSYNC 引脚位掩码。",
    "tx": "待发送字节缓冲区。",
    "rx": "接收字节缓冲区。",
    "tx_count": "要发送的字节数。",
    "rx_count": "要接收的字节数。",
    "vpp_v": "峰峰值电压，单位 V。",
    "target_vpp": "期望输出峰峰值，单位由应用约定，当前工程按 V 使用。",
    "target_vpp_v": "期望输出峰峰值，单位 V。",
    "offset_v": "直流偏置电压，单位 V。",
    "bias_voltage_v": "虚地/偏置电压，单位 V。",
    "virtual_ground_v": "模拟前端虚地电压，单位 V。",
    "supply_voltage_v": "供电电压，单位 V。",
    "input_voltage_v": "输入电压，单位 V。",
    "output_voltage_v": "输出电压，单位 V。",
    "offset_voltage_v": "换算时需要扣除或加入的直流偏置，单位 V。",
    "low_voltage_v": "低端标定/下限电压，单位 V。",
    "high_voltage_v": "高端标定/上限电压，单位 V。",
    "measured_low_v": "ADC 在低端标定点测得的电压，单位 V。",
    "true_low_v": "低端标定点的标准真实电压，单位 V。",
    "measured_high_v": "ADC 在高端标定点测得的电压，单位 V。",
    "true_high_v": "高端标定点的标准真实电压，单位 V。",
    "input_scale": "把 ADC 引脚电压还原到被测端电压的比例系数。",
    "dac_bits": "DAC 有效位数，用于计算满量程码。",
    "dac_code": "DAC 原始输出码。",
    "adc_bits": "ADC 有效位数，用于计算满量程码。",
    "gain": "线性电压增益。",
    "requested_gain": "期望的模拟前端闭环增益。",
    "measured_gain_linear": "实测线性幅值增益，不是 dB。",
    "feedback_resistor_ohm": "反馈电阻阻值，单位 Ω。",
    "input_resistor_ohm": "输入电阻阻值，单位 Ω。",
    "resistor_to_ground_ohm": "同相网络接地电阻，单位 Ω。",
    "shunt_resistance_ohm": "电流采样电阻阻值，单位 Ω。",
    "resistance": "拟合/计算得到的电阻，单位 Ω。",
    "inductance": "拟合/计算得到的电感，单位 H。",
    "capacitance": "拟合/计算得到的电容，单位 F。",
    "current_ma": "电流，单位 mA。",
    "power_mw": "功率，单位 mW。",
    "magnitude": "幅度或频谱 magnitude 数组/标量。",
    "spectrum": "复数或实数打包频谱缓冲区。",
    "spectrum_a": "A 通道频谱。",
    "spectrum_b": "B 通道频谱。",
    "fft_size": "FFT 点数，通常必须是 2 的幂。",
    "bin_count": "参与搜索/统计的频谱 bin 数量。",
    "bin_index": "频谱 bin 索引。",
    "center_bin": "目标频率对应的中心 bin。",
    "radius_bins": "中心 bin 两侧需要聚合/排除的半径。",
    "bin_offset": "相对整数频谱 bin 的小数偏移。",
    "peak_index": "检测到的峰值索引输出。",
    "search_start": "搜索区间起始索引。",
    "start_index": "处理区间起始索引（包含）。",
    "end_index": "处理区间结束索引；是否包含由函数说明决定。",
    "window_size": "滑动中值/Hampel 等算法的窗口长度，通常要求奇数。",
    "workspace": "调用者提供的临时工作区，避免算法内部动态分配。",
    "quantile": "分位点，0～1 对应最小值到最大值。",
    "harmonics": "谐波结果数组或需要分析的谐波阶数。",
    "order": "谐波阶次或算法阶数。",
    "crossings": "检测到的过零位置数组。",
    "crossing_count": "有效过零点数量。",
    "crossing_positions_samples": "以样本索引表示的插值过零位置数组。",
    "crossing_a_samples": "A 通道过零位置，单位为样本。",
    "crossing_b_samples": "B 通道过零位置，单位为样本。",
    "period_samples": "一个周期对应的样本数，可为插值后的浮点值。",
    "lag_b_relative_to_a_samples": "B 相对 A 的时间延迟，单位为样本；正负号遵循函数说明。",
    "previous_gate_samples": "上一门限/基线区间累计的样本数。",
    "current_gate_samples": "当前门限/基线区间累计的样本数。",
    "pretrigger_count": "触发点之前要保留的样本数。",
    "trigger_index": "触发点在输入样本数组中的索引。",
    "window_start_sample": "选定显示窗口的起始样本索引。",
    "slot_index": "捕获存储槽编号。",
    "event": "单个触发/键盘/捕获事件对象。",
    "events": "事件数组或队列存储区。",
    "event_count": "事件数组中的有效事件数。",
    "sequence": "连续 DMA 发布序号，用于判断是否出现新块或丢块。",
    "completed_block": "最近完成且可安全读取的 DMA 块索引。",
    "block_index": "DMA/存储块索引。",
    "next_destination": "DMA 下一次搬运应写入的目标块地址。",
    "storage": "捕获槽/环形缓存使用的底层存储区。",
    "target": "目标对象、目标值或待写入的目标缓冲区。",
    "mode": "工作模式枚举，决定采集、显示或算法分支。",
    "type": "波形/窗口/事件等类型枚举。",
    "waveform": "波形类型或波形样本对象。",
    "range_policy": "越界时采用报错、限幅等处理策略。",
    "threshold_mode": "比较器门限模式枚举。",
    "interpolation": "是否启用插值或所用插值配置。",
    "shape_parameter": "波形形状参数，例如占空比或对称度；范围由波形类型决定。",
    "scale": "数值缩放系数。",
    "multiplier": "倍频、增益或显示倍率。",
    "coefficient": "窗函数/拟合/滤波计算中的系数。",
    "real_sum": "相关/拟合的实部累加结果地址。",
    "imaginary_sum": "相关/拟合的虚部累加结果地址。",
    "angle": "角度输入；通常采用弧度，具体见函数说明。",
    "matrix": "3×4 增广矩阵，函数会原地执行高斯消元并写回解算过程。",
    "fit": "正弦拟合结果结构体。",
    "plot": "波形绘图配置/状态对象。",
    "burst": "突发检测结果结构体。",
    "info": "模块信息或计算结果摘要结构体。",
    "bounds": "上下界/安全范围结果结构体。",
    "budget": "模拟前端量程、摆幅或门限预算结果结构体。",
    "response": "频响测量或校正结果结构体。",
}


OUTPUT_POINTER_NAMES = {
    "result", "output", "destination", "output_samples", "output_centered_v",
    "actual_rate_hz", "actual_frequency_hz", "ac_rms_v", "mean_voltage_v",
    "minimum", "maximum", "peak_index", "sequence", "completed_block",
    "crossing_count", "event_count", "mean_ticks", "power_mw", "current_ma",
    "dac_code", "output_hz", "screen_y", "position", "info", "bounds", "budget",
}


def split_top_level(text: str) -> list[str]:
    if not text.strip() or text.strip() == "void":
        return []
    parts: list[str] = []
    start = 0
    round_depth = square_depth = 0
    for position, character in enumerate(text):
        if character == "(":
            round_depth += 1
        elif character == ")":
            round_depth -= 1
        elif character == "[":
            square_depth += 1
        elif character == "]":
            square_depth -= 1
        elif character == "," and round_depth == 0 and square_depth == 0:
            parts.append(text[start:position].strip())
            start = position + 1
    parts.append(text[start:].strip())
    return [part for part in parts if part]


def parse_signature(definition: indexer.Definition) -> tuple[str, list[Parameter]]:
    signature = definition.signature
    opening = signature.find("(")
    closing = signature.rfind(")")
    if opening < 0 or closing < opening:
        raise ValueError(f"invalid signature: {signature}")
    prefix = signature[:opening]
    name_position = prefix.rfind(definition.name)
    return_type = prefix[:name_position]
    return_type = re.sub(r"\b(?:static|inline|extern|__STATIC_INLINE)\b", " ", return_type)
    return_type = " ".join(return_type.split())
    parameters: list[Parameter] = []
    for declaration in split_top_level(signature[opening + 1 : closing]):
        function_pointer = re.search(r"\(\s*\*\s*([A-Za-z_]\w*)\s*\)", declaration)
        if function_pointer:
            name = function_pointer.group(1)
        else:
            array_name = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)+$", declaration)
            if array_name:
                name = array_name.group(1)
            else:
                name_match = re.search(r"([A-Za-z_]\w*)\s*$", declaration)
                if not name_match:
                    raise ValueError(f"cannot parse parameter {declaration!r} in {signature}")
                name = name_match.group(1)
        type_text = declaration
        if function_pointer:
            type_text = declaration.replace(name, "", 1)
        else:
            type_text = re.sub(rf"\b{re.escape(name)}\b(?=\s*(?:\[|$))", "", declaration, count=1)
        type_text = " ".join(type_text.split())
        parameters.append(Parameter(declaration, type_text, name))
    return return_type, parameters


def clean_doc_lines(comment: str) -> list[str]:
    lines: list[str] = []
    for raw in comment.splitlines():
        line = re.sub(r"^\s*\*\s?", "", raw).strip()
        if line:
            lines.append(line)
    return lines


def extract_header_docs(path: Path) -> dict[str, HeaderDoc]:
    source = path.read_text(encoding="utf-8", errors="replace")
    pattern = re.compile(r"/\*((?:(?!\*/).)*)\*/\s*([^;{}]+\([^;{}]*\)\s*;)", re.S)
    result: dict[str, HeaderDoc] = {}
    for match in pattern.finditer(source):
        comment, declaration = match.groups()
        if declaration.lstrip().startswith("typedef"):
            continue
        names = re.findall(r"([A-Za-z_]\w*)\s*\(", declaration)
        if not names:
            continue
        name = names[-1]
        brief_parts: list[str] = []
        return_parts: list[str] = []
        params: dict[str, tuple[str, str]] = {}
        current_kind = ""
        current_name = ""
        current_direction = ""
        accum: list[str] = []

        def flush() -> None:
            nonlocal accum
            text = " ".join(accum).strip()
            if current_kind == "brief" and text:
                brief_parts.append(text)
            elif current_kind == "return" and text:
                return_parts.append(text)
            elif current_kind == "param" and current_name and text:
                params[current_name] = (current_direction, text)
            accum = []

        for line in clean_doc_lines(comment):
            tag = re.match(r"@(brief|return|retval|note|warning|see)\b\s*(.*)", line)
            param_tag = re.match(r"@param(?:\[([^\]]+)\])?\s+([A-Za-z_]\w*)\s*(.*)", line)
            if param_tag:
                flush()
                current_kind = "param"
                current_direction = param_tag.group(1) or ""
                current_name = param_tag.group(2)
                accum = [param_tag.group(3)]
            elif tag:
                flush()
                current_kind = "return" if tag.group(1) in {"return", "retval"} else tag.group(1)
                current_name = ""
                current_direction = ""
                accum = [tag.group(2)]
            elif current_kind in {"brief", "return", "param"}:
                accum.append(line)
            elif not line.startswith("@"):
                current_kind = "brief"
                accum = [line]
        flush()
        brief = " ".join(brief_parts).strip()
        if brief.startswith("@file"):
            brief = ""
        result[name] = HeaderDoc(brief, params, " ".join(return_parts).strip())
    return result


def gather_header_docs() -> dict[tuple[str, str], HeaderDoc]:
    gathered: dict[tuple[str, str], HeaderDoc] = {}
    for project in indexer.PROJECTS:
        root = indexer.FUXIAN / project / "signal_contest_template_final"
        for path in sorted((root / "modules").glob("*.h")):
            for name, document in extract_header_docs(path).items():
                key = (path.stem, name)
                previous = gathered.get(key)
                score = len(document.params) * 10 + len(document.brief) + len(document.returns)
                previous_score = -1 if previous is None else len(previous.params) * 10 + len(previous.brief) + len(previous.returns)
                if score > previous_score:
                    gathered[key] = document
    return gathered


def infer_direction(parameter: Parameter, header_direction: str) -> str:
    if header_direction:
        normalized = header_direction.lower().replace(" ", "")
        if "out" in normalized and "in" in normalized:
            return "[in/out]"
        if "out" in normalized:
            return "[out]"
        return "[in]"
    declaration = parameter.declaration
    if "(*" in declaration or "* " in declaration or "*" in declaration or "[" in declaration:
        if "const" in declaration:
            return "[in]"
        if parameter.name in OUTPUT_POINTER_NAMES:
            return "[out]"
        return "[in/out]"
    return "[in]"


def infer_parameter_meaning(parameter: Parameter, definition: indexer.Definition) -> str:
    name = parameter.name
    if name in PARAMETER_MEANINGS:
        meaning = PARAMETER_MEANINGS[name]
        if name in {"x", "y"} and "tft" not in definition.relative_file.lower() and "draw" not in definition.name.lower() and "map" not in definition.name.lower():
            return "数值/样本坐标操作数；具体量纲由该算法的调用上下文决定。"
        return meaning
    lowered = name.lower()
    if lowered.endswith("_hz"):
        return "频率或速率，单位 Hz。"
    if lowered.endswith("_v"):
        return "电压，单位 V。"
    if lowered.endswith("_mv"):
        return "电压，单位 mV。"
    if lowered.endswith("_ma"):
        return "电流，单位 mA。"
    if lowered.endswith("_mw"):
        return "功率，单位 mW。"
    if lowered.endswith("_ohm"):
        return "电阻，单位 Ω。"
    if lowered.endswith(("_deg", "_degrees")):
        return "角度，单位度。"
    if lowered.endswith("_index") or lowered in {"index", "position"}:
        return "数组、波表、像素或状态表中的索引位置。"
    if lowered.endswith("_count") or lowered in {"count", "length", "size"}:
        return "有效元素、样本、字节或事件的数量。"
    if lowered.endswith("_capacity"):
        return "对应缓冲区最多可容纳的元素数，用于边界检查。"
    if lowered.endswith("_fraction"):
        return "归一化比例，通常取 0～1；具体边界由函数校验。"
    if lowered.endswith("_code"):
        return "硬件寄存器、ADC/DAC 或 DDS 使用的数字编码。"
    if lowered.endswith("_samples"):
        return "以样本为单位的数组、位置或时间间隔。"
    if lowered.endswith("_ticks"):
        return "以定时器 tick 为单位的计数值。"
    if "*" in parameter.declaration or "[" in parameter.declaration:
        return f"`{name}` 数据对象/缓冲区；结合类型 `{parameter.type_text}` 由函数读取或写回。"
    return f"控制 `{definition.name}` 的 `{name}` 输入量；类型为 `{parameter.type_text}`，有效范围由函数中的参数校验决定。"


def parameter_documentation(
    definition: indexer.Definition,
    parameter: Parameter,
    header_docs: dict[tuple[str, str], HeaderDoc],
) -> tuple[str, str, str]:
    document = header_docs.get((Path(definition.relative_file).stem, definition.name))
    source_direction = ""
    if document and parameter.name in document.params:
        source_direction, source_text = document.params[parameter.name]
        if re.search(r"[\u4e00-\u9fff]", source_text):
            if not source_direction:
                if "输入/输出" in source_text or "输入输出" in source_text or "原地" in source_text:
                    source_direction = "in,out"
                elif source_text.startswith(("输出", "写回", "返回")):
                    source_direction = "out"
                elif source_text.startswith("输入") or "只读" in source_text:
                    source_direction = "in"
            return infer_direction(parameter, source_direction), source_text.rstrip("。. ") + "。", "源码 `@param`"
    return infer_direction(parameter, source_direction), infer_parameter_meaning(parameter, definition), "签名/实现语义"


def return_documentation(
    definition: indexer.Definition,
    return_type: str,
    header_docs: dict[tuple[str, str], HeaderDoc],
) -> tuple[str, str]:
    document = header_docs.get((Path(definition.relative_file).stem, definition.name))
    if document and document.returns and re.search(r"[\u4e00-\u9fff]", document.returns):
        return document.returns.rstrip("。. ") + "。", "源码 `@return`"
    name = definition.name.lower()
    if return_type == "void":
        return "无返回值；结果通过硬件状态、模块对象或输出参数产生。", "签名/实现语义"
    if definition.name == "main":
        return "嵌入式主循环正常情况下不会返回；保留的 `int` 返回类型用于满足 C 入口约定。", "签名/实现语义"
    if return_type in {"signal_result_t", "signal_algorithm_status_t", "tft_st7789_status_t", "tft_ili9341_status_t", "ad9850_status_t", "arm_status"}:
        return "返回状态码：成功值表示操作完成，其他值用于区分空指针、范围、容量、硬件或数值错误。", "类型/实现语义"
    if return_type == "signal_status_t":
        return "返回模块当前运行状态枚举，例如未初始化、空闲、忙、完成或错误。", "类型/实现语义"
    if return_type == "signal_module_status_t":
        return "返回模块成熟度/验证等级元数据，供应用判断示例、冻结或硬件验证状态。", "类型/实现语义"
    if return_type == "bool":
        return f"返回布尔判定：`true` 表示 `{definition.name}` 所检查的条件成立，`false` 表示不成立。", "签名/函数名语义"
    if "*" in return_type:
        return "返回内部对象/缓冲区指针；调用者不得越界访问，若接口允许失败还需先检查是否为 `NULL`。", "类型/实现语义"
    if return_type == "signal_statistics_t":
        return "按值返回统计结果结构体。", "类型语义"
    if return_type == "burst_result_t":
        return "按值返回突发检测结果，包括有效性和起止/持续信息。", "类型语义"
    if return_type == "waveform_type_t":
        return "返回识别得到的波形类型枚举。", "类型语义"
    if return_type == "int" and ("poweroftwo" in name or name.startswith("is") or "excluded" in name):
        return "返回非零表示条件成立，0 表示条件不成立。", "实现语义"
    if return_type == "int" and "write" in name:
        return "返回总线回调状态；0 通常表示发送成功，非 0 表示失败。", "实现语义"
    if return_type == "size_t":
        return "返回当前缓冲区元素数或已追加的字符数；具体含义由函数作用决定。", "类型/函数名语义"
    if return_type.startswith("uint") or return_type.startswith("int"):
        if "width" in name or "height" in name:
            return "返回当前旋转方向下的屏幕宽度/高度，单位像素。", "函数名语义"
        if "rate" in name or "frequency" in name:
            return "返回量化后实际速率/频率，单位 Hz。", "函数名语义"
        if "count" in name:
            return "返回当前有效元素、样本或事件数量。", "函数名语义"
        if "index" in name or "slot" in name or "block" in name:
            return "返回相应数组、槽位或 DMA 块索引。", "函数名语义"
        return f"返回 `{definition.name}` 计算得到的整数结果；位宽和符号由 `{return_type}` 决定。", "类型/函数名语义"
    if return_type == "float":
        return f"返回 `{definition.name}` 计算得到的单精度结果；单位由函数用途和输入参数决定。", "类型/函数名语义"
    return f"返回 `{return_type}` 类型结果，语义见函数作用说明。", "类型/实现语义"


def find_calls(path: Path, targets: set[str]) -> dict[str, list[Call]]:
    source = path.read_text(encoding="utf-8", errors="replace")
    masked = indexer.mask_non_code(source)
    calls: dict[str, list[Call]] = defaultdict(list)
    if not targets:
        return calls
    target_pattern = "|".join(re.escape(name) for name in sorted(targets, key=len, reverse=True))
    for match in re.finditer(rf"\b({target_pattern})\s*\(", masked):
        opening = masked.find("(", match.start())
        depth = 1
        position = opening + 1
        while position < len(masked) and depth:
            if masked[position] == "(":
                depth += 1
            elif masked[position] == ")":
                depth -= 1
            position += 1
        if depth:
            continue
        closing = position - 1
        arguments = " ".join(source[opening + 1 : closing].split())
        line = source.count("\n", 0, match.start()) + 1
        calls[match.group(1)].append(Call(line, arguments))
    return calls


def parse_sysconfig(path: Path) -> list[dict[str, str]]:
    source = path.read_text(encoding="utf-8", errors="replace")
    module_vars: dict[str, str] = {}
    for variable, module_path in re.findall(
        r"const\s+([A-Za-z_]\w*)\s*=\s*scripting\.addModule\(\"([^\"]+)\"", source
    ):
        module_vars[variable] = module_path.rsplit("/", 1)[-1]
    instances: dict[str, str] = {}
    instance_parents: set[str] = set()
    for variable, parent in re.findall(
        r"const\s+([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*)\.addInstance\(\s*\)", source
    ):
        instances[variable] = module_vars.get(parent, parent)
        instance_parents.add(parent)
    for variable, module_name in module_vars.items():
        if variable not in instances and variable not in instance_parents:
            instances[variable] = module_name

    selected_properties = (
        "$name", "peripheral.$assign", "timerPeriod", "timerMode", "targetBitRate",
        "adcMem0chansel", "trigSrc", "configureDMA", "enabledInterrupts", "channelEnable",
        "profile", "setDACCode0", "sampClkSrc", "clockPrescale", "DMA_CHANNEL.$name",
        "DMA_CHANNEL.peripheral.$assign",
    )
    rows: list[dict[str, str]] = []
    for variable, module_name in instances.items():
        assignments = re.findall(
            rf"^\s*{re.escape(variable)}((?:\.[A-Za-z_$][\w$]*|\[[0-9]+\])*)\s*=\s*([^;]+);",
            source,
            re.M,
        )
        properties = {key.lstrip("."): " ".join(value.split()) for key, value in assignments}
        logical = properties.get("$name", variable).strip('"')
        physical = properties.get("peripheral.$assign", "").strip('"')
        pin_values = []
        for key, value in properties.items():
            if key.endswith(".$assign") and key not in {"peripheral.$assign", "DMA_CHANNEL.peripheral.$assign"}:
                pin_values.append(f"{key}={value.strip(chr(34))}")
        associated_pin_indexes = sorted(
            {
                int(match.group(1))
                for key in properties
                if (match := re.match(r"associatedPins\[([0-9]+)\]\.", key))
            }
        )
        for pin_index in associated_pin_indexes:
            prefix = f"associatedPins[{pin_index}]."
            pin_name = properties.get(prefix + "$name", f"pin{pin_index}").strip('"')
            assigned_port = properties.get(prefix + "assignedPort", "").strip('"')
            assigned_pin = properties.get(prefix + "assignedPin", "").strip('"')
            direction = properties.get(prefix + "direction", "").strip('"')
            resistor = properties.get(prefix + "internalResistor", "").strip('"')
            location = f"{assigned_port}{assigned_pin}" if assigned_port or assigned_pin else "未固定"
            extras = "/".join(item for item in (direction, resistor) if item)
            pin_values.append(f"{pin_name}={location}" + (f"({extras})" if extras else ""))
        summary = []
        for wanted in selected_properties:
            if wanted in properties and wanted not in {"$name", "peripheral.$assign"}:
                summary.append(f"{wanted}={properties[wanted]}")
        if pin_values:
            summary.append("引脚：" + "，".join(pin_values))
        rows.append(
            {
                "module": module_name,
                "variable": variable,
                "logical": logical,
                "physical": physical or "—",
                "summary": "；".join(summary) if summary else "使用 SysConfig/SDK 默认值或仅参与全局配置",
            }
        )
    return sorted(rows, key=lambda row: (row["module"], row["logical"]))


def source_link(relative_file: str, line: int) -> str:
    return f"[{relative_file}:L{line}]({relative_file}#L{line})"


def escape_table(text: str) -> str:
    return text.replace("|", "\\|").replace("\n", " ")


def visibility(definition: indexer.Definition) -> str:
    if definition.name == "main":
        return "工程入口"
    if definition.name.endswith("IRQHandler") or definition.name == "SysTick_Handler":
        return "ISR（由硬件/内核调用）"
    if definition.is_static:
        return "文件内部 `static`"
    if definition.relative_file.endswith(".h"):
        return "头文件内联接口"
    return "公开接口"


def render_external_calls(main_path: Path) -> list[str]:
    calls = find_calls(main_path, set(EXTERNAL_SPECS))
    lines = [
        "## 5. main.c 直接调用的 SDK / CMSIS-DSP / C 数学接口",
        "",
        "这里只记录工程源码直接调用的外部库函数；其函数体属于 TI SDK、CMSIS 或 C 运行库，不计入本工程函数定义总数。实参按当前源码原样列出，迁移时要同时核对目标 SDK 版本的原型。",
        "",
    ]
    if not calls:
        lines.extend(["本工程 `main.c` 未发现本手册所列外部接口的直接调用。", ""])
        return lines
    for name in sorted(calls):
        spec = EXTERNAL_SPECS[name]
        lines.extend([f"### `{name}`", "", f"- 作用：{spec.purpose}"])
        if spec.parameters:
            lines.extend(["- 形参与意义：", "", "| 形参 | 方向 | 意义 |", "|---|---|---|"])
            for parameter_name, direction, meaning in spec.parameters:
                lines.append(f"| `{parameter_name}` | {direction} | {escape_table(meaning)} |")
        else:
            lines.append("- 参数：无。")
        lines.append(f"- 返回：{spec.returns}")
        links = []
        for call in calls[name]:
            args = escape_table(call.arguments)
            links.append(f"{source_link('main.c', call.line)}：`{name}({args})`")
        lines.extend(["- 当前工程调用：" + "；".join(links), ""])
    return lines


def render_function(
    definition: indexer.Definition,
    briefs: dict[tuple[str, str], str],
    header_docs: dict[tuple[str, str], HeaderDoc],
) -> tuple[list[str], int]:
    return_type, parameters = parse_signature(definition)
    return_text, return_basis = return_documentation(definition, return_type, header_docs)
    excluded = Path(definition.relative_file).name in indexer.EXCLUDED_FROM_DEBUG.get(definition.project, set())
    category, module_purpose = indexer.module_info(definition)
    anchor = re.sub(r"[^A-Za-z0-9_-]", "-", f"fn-{definition.relative_file}-{definition.name}")
    lines = [
        f'<a id="{anchor}"></a>',
        "",
        f"### `{definition.name}`",
        "",
        f"- 定义位置：{source_link(definition.relative_file, definition.line)}" + ("；**Debug 构建排除**" if excluded else ""),
        f"- 属性：{visibility(definition)}；功能分类：{category}。",
        f"- 所属模块：{module_purpose}。",
        f"- 具体作用：{function_description(definition, briefs)}",
        "- 函数签名：",
        "",
        "```c",
        definition.signature,
        "```",
        "",
        f"- 返回值（`{return_type}`）：{return_text}（依据：{return_basis}）",
    ]
    if not parameters:
        lines.extend(["- 参数：无（`void`）。", ""])
        return lines, 0
    lines.extend(["- 参数明细：", "", "| 参数 | 声明类型 | 方向 | 具体意义 | 说明依据 |", "|---|---|---|---|---|"])
    for parameter in parameters:
        direction, meaning, basis = parameter_documentation(definition, parameter, header_docs)
        lines.append(
            f"| `{parameter.name}` | `{escape_table(parameter.type_text)}` | {direction} | {escape_table(meaning)} | {basis} |"
        )
    lines.append("")
    return lines, len(parameters)


def render_project(
    project: str,
    definitions: list[indexer.Definition],
    briefs: dict[tuple[str, str], str],
    header_docs: dict[tuple[str, str], HeaderDoc],
) -> tuple[str, dict[str, int]]:
    root = indexer.FUXIAN / project / "signal_contest_template_final"
    project_definitions = sorted(
        [definition for definition in definitions if definition.project == project],
        key=lambda definition: (definition.relative_file != "main.c", definition.relative_file.lower(), definition.line),
    )
    main_definitions = [definition for definition in project_definitions if definition.relative_file == "main.c"]
    module_definitions = [definition for definition in project_definitions if definition.relative_file != "main.c"]
    module_names = {definition.name for definition in module_definitions}
    main_calls = find_calls(root / "main.c", module_names)
    sysconfig_rows = parse_sysconfig(root / "signal_contest_template.syscfg")

    all_module_paths = sorted((root / "modules").glob("*.[ch]"))
    module_files: dict[str, set[str]] = defaultdict(set)
    for path in all_module_paths:
        module_files[path.stem].add(path.suffix)
    by_stem_definitions: dict[str, list[indexer.Definition]] = defaultdict(list)
    for definition in module_definitions:
        by_stem_definitions[Path(definition.relative_file).stem].append(definition)

    lines = [
        f"# {project}：工程模块与函数参数详解",
        "",
        f"> 工程定位：{indexer.PROJECT_PURPOSE[project]}。",
        "> 本文由当前源码静态生成，行号均指向函数定义或调用的起始行；修改源码后请重新运行生成器。",
        "",
        "## 1. 工程功能与主数据流",
        "",
        PROJECT_FLOW[project],
        "",
        "### 统计与阅读边界",
        "",
        f"- 工程内函数定义：**{len(project_definitions)}** 个，其中 `main.c` {len(main_definitions)} 个、`modules` {len(module_definitions)} 个。",
        f"- 软件模块文件族：**{len(module_files)}** 个；SysConfig 实例/单例记录：**{len(sysconfig_rows)}** 条。",
        "- 函数目录覆盖 `main.c`、`modules/*.c` 和 `modules/*.h` 中带函数体的内联函数；不重复收录纯声明、宏、`Debug/Release` 生成副本和 TI/CMSIS 外部库函数体。",
        "- 参数说明优先引用中文 `@param`；无注释时根据声明类型、命名、所属模块及实现语义整理，并在“说明依据”列标出。",
        "- `[in/out]` 表示指针指向的对象可能被读取并更新。实际数组长度、单位和合法范围仍应以相邻参数、配置宏和函数体检查为准。",
        "",
        "## 2. SysConfig 硬件/SDK 模块",
        "",
        "这些名称来自 `signal_contest_template.syscfg`。迁移硬件函数时，必须同步迁移实例名、物理外设、DMA 通道、事件、IRQ 和引脚。",
        "",
        "| DriverLib 模块 | SysConfig 变量 / 逻辑实例名 | 物理外设 | 关键配置摘要 |",
        "|---|---|---|---|",
    ]
    for row in sysconfig_rows:
        lines.append(
            f"| `{row['module']}` | `{row['variable']}` / `{row['logical']}` | `{row['physical']}` | {escape_table(row['summary'])} |"
        )

    lines.extend(
        [
            "",
            "## 3. 软件模块清单",
            "",
            "“main 直接调用”只统计 `main.c` 的显式调用；0 次不等于无用，它可能由其他模块调用、作为 ISR/回调入口，或仅提供类型与配置。",
            "",
            "| 模块文件族 | 功能分类 | 模块职责 | 函数定义数 | main 直接调用的接口 | 构建状态 |",
            "|---|---|---|---:|---|---|",
        ]
    )
    for stem in sorted(module_files):
        category, purpose = indexer.MODULE_INFO.get(
            stem, ("数学、状态与内部公共支持", f"{stem} 文件提供的内部/公共支持")
        )
        stem_definitions = by_stem_definitions.get(stem, [])
        called_names = sorted({definition.name for definition in stem_definitions if definition.name in main_calls})
        calls_text = "、".join(f"`{name}`" for name in called_names) if called_names else "—"
        source_names = {path.name for path in all_module_paths if path.stem == stem}
        excluded_sources = source_names & indexer.EXCLUDED_FROM_DEBUG.get(project, set())
        build_status = "**Debug 构建排除**" if excluded_sources else "随工程编译/头文件依赖"
        source_display = "、".join(
            f"`modules/{path.name}`" for path in sorted(all_module_paths) if path.stem == stem
        )
        lines.append(
            f"| {source_display} | {category} | {escape_table(purpose)} | {len(stem_definitions)} | {calls_text} | {build_status} |"
        )

    lines.extend(
        [
            "",
            "## 4. main.c 实际使用的模块接口",
            "",
            "下表回答“这个工程实际调用了模块里的哪些函数”。调用位置给出当前实参；完整形参、方向和意义见后面的函数目录。",
            "",
        ]
    )
    if not main_calls:
        lines.extend(["`main.c` 未直接调用 `modules` 中定义的函数。", ""])
    else:
        lines.extend(["| 模块接口 | 定义文件 | 作用 | main.c 调用位置与当前实参 |", "|---|---|---|---|"])
        by_name = {definition.name: definition for definition in module_definitions}
        for name in sorted(main_calls):
            definition = by_name[name]
            call_text = "<br>".join(
                f"{source_link('main.c', call.line)}：`{name}({escape_table(call.arguments)})`" for call in main_calls[name]
            )
            lines.append(
                f"| `{name}` | `{definition.relative_file}` | {escape_table(function_description(definition, briefs))} | {call_text} |"
            )
        lines.append("")

    lines.extend(render_external_calls(root / "main.c"))
    lines.extend(
        [
            "## 6. 全部函数定义、作用、返回值与参数",
            "",
            "本节逐个列出本工程源码中的所有函数定义。公开接口适合跨文件调用；`static` 函数通常依赖同一 `.c` 的静态状态，复用时优先整体复制所属模块。",
            "",
            "### 文件导航",
            "",
        ]
    )
    by_file: dict[str, list[indexer.Definition]] = defaultdict(list)
    for definition in project_definitions:
        by_file[definition.relative_file].append(definition)
    for relative_file in sorted(by_file, key=lambda item: (item != "main.c", item.lower())):
        anchor = re.sub(r"[^A-Za-z0-9_-]", "-", f"file-{relative_file}")
        lines.append(f"- [`{relative_file}`：{len(by_file[relative_file])} 个](#{anchor})")

    parameter_total = 0
    function_render_count = 0
    for relative_file in sorted(by_file, key=lambda item: (item != "main.c", item.lower())):
        file_anchor = re.sub(r"[^A-Za-z0-9_-]", "-", f"file-{relative_file}")
        lines.extend(["", f'<a id="{file_anchor}"></a>', "", f"## `{relative_file}`", ""])
        for definition in by_file[relative_file]:
            rendered, parameter_count = render_function(definition, briefs, header_docs)
            lines.extend(rendered)
            parameter_total += parameter_count
            function_render_count += 1

    lines.extend(
        [
            "## 7. 赛场复用检查表",
            "",
            "1. 先从第 4 节确认应用实际调用的公开 API，再到第 6 节核对参数、返回值及 `static` 依赖。",
            "2. 复制纯算法模块时同时复制同名 `.c/.h`、公共状态类型以及 `signal_math*` 依赖。",
            "3. 复制 ADC/DMA、DAC、Capture、TFT、键盘、OPA/GPAMP/COMP 等硬件模块时，同步复现第 2 节 SysConfig 实例名和事件/IRQ/引脚。",
            "4. 核对 `signal_config.h` 中的 Fs、N、VREF、ADC/DAC 位数、DMA 块数和显示尺寸；这些量决定参数单位和数组容量。",
            "5. `example05` 标为构建排除的旧 DAC/DDS 文件不能因为“目录里存在”就认为当前固件会编译执行。",
            "6. 本手册是静态审计结果，不等同于示波器/逻辑分析仪/实板验证；赛前至少对采样率、相位符号、门限电压和引脚复用做一次硬件复核。",
            "",
            "---",
            "",
            f"生成器：`tools/generate_per_project_function_docs.py`；本文件校验记录：{function_render_count} 个函数、{parameter_total} 个具名参数。",
            "",
        ]
    )
    return "\n".join(lines), {
        "functions": function_render_count,
        "parameters": parameter_total,
        "main_calls": sum(len(items) for items in main_calls.values()),
        "module_families": len(module_files),
    }


def validate_project(
    project: str,
    content: str,
    definitions: list[indexer.Definition],
    expected_stats: dict[str, int],
) -> None:
    root = indexer.FUXIAN / project / "signal_contest_template_final"
    project_definitions = [definition for definition in definitions if definition.project == project]
    if expected_stats["functions"] != len(project_definitions):
        raise AssertionError(f"{project}: rendered function count mismatch")
    if content.count("- 定义位置：") != len(project_definitions):
        raise AssertionError(f"{project}: definition sections missing")
    parsed_parameter_count = sum(len(parse_signature(definition)[1]) for definition in project_definitions)
    if parsed_parameter_count != expected_stats["parameters"]:
        raise AssertionError(f"{project}: parameter count mismatch")
    parameter_rows = len(re.findall(r"^\| `[^`]+` \| `[^`]*` \| \[(?:in|out|in/out)\] \|", content, re.M))
    if parameter_rows != parsed_parameter_count:
        raise AssertionError(
            f"{project}: parameter rows={parameter_rows}, expected={parsed_parameter_count}"
        )
    for definition in project_definitions:
        path = root / definition.relative_file
        if not path.exists():
            raise AssertionError(f"{project}: missing source {path}")
        source_lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        if definition.line < 1 or definition.line > len(source_lines):
            raise AssertionError(f"{project}: invalid line {definition.relative_file}:{definition.line}")
        nearby = " ".join(source_lines[definition.line - 1 : min(definition.line + 3, len(source_lines))])
        if definition.name not in nearby:
            raise AssertionError(f"{project}: function name not near reported line {definition.name}")
    main_path = root / "main.c"
    source = main_path.read_text(encoding="utf-8", errors="replace")
    masked = indexer.mask_non_code(source)
    all_call_names = {match.group(1) for match in re.finditer(r"\b([A-Za-z_]\w*)\s*\(", masked)}
    library_like = {
        name
        for name in all_call_names
        if name.startswith(("DL_", "NVIC_", "SYSCFG_", "SysTick_", "arm_", "__"))
        or name in {"sinf", "cosf", "sqrtf", "fabsf", "atan2f", "floorf", "ceilf"}
    }
    library_like -= {definition.name for definition in project_definitions}
    missing_specs = library_like - set(EXTERNAL_SPECS)
    if missing_specs:
        raise AssertionError(f"{project}: undocumented external calls {sorted(missing_specs)}")
    external_calls = find_calls(main_path, set(EXTERNAL_SPECS))
    for name, calls in external_calls.items():
        expected = len(EXTERNAL_SPECS[name].parameters)
        for call in calls:
            actual = len(split_top_level(call.arguments))
            if actual != expected:
                raise AssertionError(
                    f"{project}: external arity mismatch {name} at main.c:{call.line}, "
                    f"actual={actual}, documented={expected}"
                )
    for target in re.findall(r"\]\(([^)]+)\)", content):
        if target.startswith("#"):
            continue
        relative_target = target.split("#", 1)[0]
        if not (root / relative_target).exists():
            raise AssertionError(f"{project}: broken markdown link {target}")


def render_navigation(stats: dict[str, dict[str, int]]) -> str:
    lines = [
        "# 各工程模块与函数参数详解导航",
        "",
        "> 每个工程一份独立手册，覆盖 SysConfig 硬件模块、软件模块、`main.c` 实际调用、全部工程函数、返回值和每个具名参数。",
        "",
        "| 工程 | 工程定位 | 函数定义 | 具名参数 | 软件模块族 | 手册 |",
        "|---|---|---:|---:|---:|---|",
    ]
    total_functions = total_parameters = 0
    for project in indexer.PROJECTS:
        item = stats[project]
        total_functions += item["functions"]
        total_parameters += item["parameters"]
        target = f"{project}/signal_contest_template_final/{OUTPUT_NAME}"
        lines.append(
            f"| `{project}` | {indexer.PROJECT_PURPOSE[project]} | {item['functions']} | {item['parameters']} | {item['module_families']} | [打开]({target}) |"
        )
    lines.extend(
        [
            "",
            f"总计：**{total_functions}** 个函数定义位置、**{total_parameters}** 个具名参数。",
            "",
            "## 更新方法",
            "",
            "源码修改后，在仓库根目录执行：",
            "",
            "```powershell",
            "python tools/generate_per_project_function_docs.py",
            "```",
            "",
            "生成器会重新扫描并校验每个定义行、函数数量和参数说明行数。",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> None:
    definitions, briefs = indexer.scan()
    header_docs = gather_header_docs()
    stats: dict[str, dict[str, int]] = {}
    for project in indexer.PROJECTS:
        content, project_stats = render_project(project, definitions, briefs, header_docs)
        validate_project(project, content, definitions, project_stats)
        output = indexer.FUXIAN / project / "signal_contest_template_final" / OUTPUT_NAME
        output.write_text(content, encoding="utf-8", newline="\n")
        stats[project] = project_stats
        print(
            f"wrote {output.relative_to(indexer.WORKSPACE)} "
            f"functions={project_stats['functions']} parameters={project_stats['parameters']}"
        )
    navigation = render_navigation(stats)
    for target in re.findall(r"\]\(([^)]+)\)", navigation):
        relative_target = target.split("#", 1)[0]
        if not (indexer.FUXIAN / relative_target).exists():
            raise AssertionError(f"navigation: broken markdown link {target}")
    NAVIGATION.write_text(navigation, encoding="utf-8", newline="\n")
    print(f"wrote {NAVIGATION.relative_to(indexer.WORKSPACE)}")
    print(
        "validated "
        f"projects={len(stats)} functions={sum(item['functions'] for item in stats.values())} "
        f"parameters={sum(item['parameters'] for item in stats.values())}"
    )


if __name__ == "__main__":
    main()
