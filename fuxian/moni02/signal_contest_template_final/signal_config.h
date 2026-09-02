#ifndef SIGNAL_CONTEST_CONFIG_H
#define SIGNAL_CONTEST_CONFIG_H

/* ============================================================
 * DAC / DDS 公共接口参数
 * 这些名字与 fuyong/90_dds_usage 保持一致，复制 DDS 初始化代码时不用改接口名。
 * ============================================================ */
#define SIGNAL_DAC_UPDATE_RATE_HZ       (100000U)
#define SIGNAL_DDS_FREQUENCY_HZ         (1000.0f)
#define SIGNAL_DAC_REFERENCE_V          (3.3f)
#define SIGNAL_DAC_OFFSET_V             (1.65f)

/* ============================================================
 * moni02 题目参数边界
 * 只在这一处修改范围和默认值，主程序不会再散落同义常数。
 * ============================================================ */
#define MONI02_FREQUENCY_MIN_HZ         (100.0f)
#define MONI02_FREQUENCY_MAX_HZ         (10000.0f)

#define MONI02_VPP_DEFAULT_V            (1.0f)
#define MONI02_VPP_MIN_V                (0.2f)
#define MONI02_VPP_MAX_V                (3.0f)

#define MONI02_SQUARE_DUTY_DEFAULT      (0.50f)
#define MONI02_SQUARE_DUTY_MIN          (0.05f)
#define MONI02_SQUARE_DUTY_MAX          (0.95f)

#define MONI02_SAW_SYMMETRY_DEFAULT     (1.00f)
#define MONI02_SAW_SYMMETRY_MIN         (0.05f)
#define MONI02_SAW_SYMMETRY_MAX         (1.00f)

/* 100 kHz / 100 Hz = 1000 点；1024 点容量覆盖题目最低频率。 */
#define MONI02_WAVE_TABLE_COUNT         (256U)
#define MONI02_DAC_OUTPUT_CAPACITY      (1024U)

#endif /* SIGNAL_CONTEST_CONFIG_H */
