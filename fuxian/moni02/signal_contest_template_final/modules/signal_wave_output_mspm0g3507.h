#ifndef SIGNAL_WAVE_OUTPUT_MSPM0G3507_H
#define SIGNAL_WAVE_OUTPUT_MSPM0G3507_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "signal_status.h"
#include "signal_dac_dma_mspm0g3507.h"

/*
 * 统一波形类型接口。
 * 新工程只需要修改这个枚举值，不必在 main.c 中维护多套底层调用。
 */
typedef enum {
    SIGNAL_WAVE_OUTPUT_SINE = 0,
    SIGNAL_WAVE_OUTPUT_SQUARE,
    SIGNAL_WAVE_OUTPUT_TRIANGLE,
    SIGNAL_WAVE_OUTPUT_SAWTOOTH
} signal_wave_output_type_t;

typedef struct {
    uint16_t *wave_table;
    size_t wave_table_count;
    uint16_t *output_buffer;
    size_t output_capacity;
    signal_dac_dma_mspm0_config_t dac_config;
    uint32_t dac_bits;
    float reference_voltage_v;
} signal_wave_output_config_t;

typedef struct {
    float requested_frequency_hz;
    float actual_frequency_hz;
    float requested_vpp_v;
    float actual_vpp_v;
    float offset_v;
    uint32_t dma_point_count;
} signal_wave_output_result_t;

signal_result_t SignalWaveOutput_Init(
    const signal_wave_output_config_t *config);
/*
 * 统一启动接口：
 * - frequency_hz：输出频率，单位 Hz；
 * - vpp_v：输出峰峰值，单位 V；
 * - offset_v：直流偏置，单位 V；
 * - shape_fraction：方波时表示占空比，锯齿波时表示对称度，范围 0~1；
 *   正弦波和三角波会忽略该参数，可统一传入 0.5f。
 */
signal_result_t SignalWaveOutput_Start(
    signal_wave_output_type_t type, float frequency_hz, float vpp_v,
    float offset_v, float shape_fraction);
signal_result_t SignalWaveOutput_SineWithOffset(
    float frequency_hz, float vpp_v, float offset_v);
signal_result_t SignalWaveOutput_SquareWithOffset(
    float frequency_hz, float vpp_v, float offset_v);
signal_result_t SignalWaveOutput_SquareWithDuty(
    float frequency_hz, float vpp_v, float offset_v, float duty_fraction);
signal_result_t SignalWaveOutput_TriangleWithOffset(
    float frequency_hz, float vpp_v, float offset_v);
signal_result_t SignalWaveOutput_SawtoothWithOffset(
    float frequency_hz, float vpp_v, float offset_v);
signal_result_t SignalWaveOutput_SawtoothWithSymmetry(
    float frequency_hz, float vpp_v, float offset_v,
    float symmetry_fraction);
signal_result_t SignalWaveOutput_GetLastResult(
    signal_wave_output_result_t *result);
void SignalWaveOutput_Stop(void);
signal_module_status_t SignalWaveOutput_GetModuleStatus(void);

#endif
