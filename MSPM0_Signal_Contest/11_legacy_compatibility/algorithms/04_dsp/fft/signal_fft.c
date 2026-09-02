#include "signal_fft.h"
#include "signal_fft_backend_config.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#if SIGNAL_FFT_BACKEND != SIGNAL_FFT_BACKEND_REFERENCE_C
#include "arm_const_structs.h"
#include "arm_math.h"
#endif

#define SIGNAL_FFT_PI_F 3.14159265358979323846f

static int SignalFFT_IsPowerOfTwo(uint32_t value)
{
    return (value != 0U) && ((value & (value - 1U)) == 0U);
}

static void SignalFFT_Swap(signal_complex_f32_t *left, signal_complex_f32_t *right)
{
    signal_complex_f32_t temporary = *left;
    *left = *right;
    *right = temporary;
}

static signal_algorithm_status_t SignalFFT_ForwardComplexReference(
    signal_complex_f32_t *data,
    uint32_t count)
{
    uint32_t index;
    uint32_t reversed = 0U;
    uint32_t length;

    if (data == NULL)
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((count < 2U) || !SignalFFT_IsPowerOfTwo(count))
    {
        return SIGNAL_ALGORITHM_NOT_SUPPORTED;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(data[index].real) || !isfinite(data[index].imag))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }

    for (index = 1U; index < count; ++index)
    {
        uint32_t bit = count >> 1U;
        while ((reversed & bit) != 0U)
        {
            reversed ^= bit;
            bit >>= 1U;
        }
        reversed ^= bit;
        if (index < reversed)
        {
            SignalFFT_Swap(&data[index], &data[reversed]);
        }
    }

    for (length = 2U; length <= count; length <<= 1U)
    {
        uint32_t block_start;
        uint32_t half_length = length >> 1U;
        float angle = (-2.0f * SIGNAL_FFT_PI_F) / (float)length;
        float step_real = cosf(angle);
        float step_imag = sinf(angle);

        for (block_start = 0U; block_start < count; block_start += length)
        {
            uint32_t offset;
            float twiddle_real = 1.0f;
            float twiddle_imag = 0.0f;

            for (offset = 0U; offset < half_length; ++offset)
            {
                uint32_t even_index = block_start + offset;
                uint32_t odd_index = even_index + half_length;
                float odd_real = (data[odd_index].real * twiddle_real) -
                                 (data[odd_index].imag * twiddle_imag);
                float odd_imag = (data[odd_index].real * twiddle_imag) +
                                 (data[odd_index].imag * twiddle_real);
                float even_real = data[even_index].real;
                float even_imag = data[even_index].imag;
                float next_twiddle_real;

                data[even_index].real = even_real + odd_real;
                data[even_index].imag = even_imag + odd_imag;
                data[odd_index].real = even_real - odd_real;
                data[odd_index].imag = even_imag - odd_imag;

                next_twiddle_real = (twiddle_real * step_real) -
                                     (twiddle_imag * step_imag);
                twiddle_imag = (twiddle_real * step_imag) +
                                (twiddle_imag * step_real);
                twiddle_real = next_twiddle_real;
            }
        }
        if (length == count)
        {
            break;
        }
    }

    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(data[index].real) || !isfinite(data[index].imag))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }
    return SIGNAL_ALGORITHM_OK;
}

#if SIGNAL_FFT_BACKEND != SIGNAL_FFT_BACKEND_REFERENCE_C

static int SignalFFT_InputIsFinite(
    const signal_complex_f32_t *data,
    uint32_t count)
{
    uint32_t index;

    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(data[index].real) || !isfinite(data[index].imag))
        {
            return 0;
        }
    }
    return 1;
}

#if SIGNAL_FFT_BACKEND == SIGNAL_FFT_BACKEND_CMSIS_Q15

static const arm_cfft_instance_q15 *SignalFFT_GetQ15Instance(uint32_t count)
{
    switch (count)
    {
        case 16U: return &arm_cfft_sR_q15_len16;
        case 32U: return &arm_cfft_sR_q15_len32;
        case 64U: return &arm_cfft_sR_q15_len64;
        case 128U: return &arm_cfft_sR_q15_len128;
        case 256U: return &arm_cfft_sR_q15_len256;
        case 512U: return &arm_cfft_sR_q15_len512;
        case 1024U: return &arm_cfft_sR_q15_len1024;
        case 2048U: return &arm_cfft_sR_q15_len2048;
        case 4096U: return &arm_cfft_sR_q15_len4096;
        default: return NULL;
    }
}

static signal_algorithm_status_t SignalFFT_ForwardComplexCMSISQ15(
    signal_complex_f32_t *data,
    uint32_t count)
{
    const arm_cfft_instance_q15 *instance = SignalFFT_GetQ15Instance(count);
    q15_t *fixed_data = (q15_t *)(void *)data;
    float maximum_absolute = 0.0f;
    float restore_scale;
    uint32_t index;

    if (instance == NULL)
    {
        return SIGNAL_ALGORITHM_NOT_SUPPORTED;
    }
    for (index = 0U; index < count; ++index)
    {
        float real_absolute = fabsf(data[index].real);
        float imag_absolute = fabsf(data[index].imag);
        if (real_absolute > maximum_absolute)
        {
            maximum_absolute = real_absolute;
        }
        if (imag_absolute > maximum_absolute)
        {
            maximum_absolute = imag_absolute;
        }
    }
    if (maximum_absolute == 0.0f)
    {
        return SIGNAL_ALGORITHM_OK;
    }

    /*
     * 向前覆盖是安全的：每个复数 float 占 8 字节，转换后的两个 Q15
     * 只占 4 字节。CMSIS 定点 CFFT 前向结果具有 1/N 缩放。
     */
    for (index = 0U; index < count; ++index)
    {
        float real = data[index].real / maximum_absolute;
        float imag = data[index].imag / maximum_absolute;
        fixed_data[2U * index] = (q15_t)(real * 32767.0f);
        fixed_data[2U * index + 1U] = (q15_t)(imag * 32767.0f);
    }

    arm_cfft_q15(instance, fixed_data, 0U, 1U);
    restore_scale = maximum_absolute * (float)count / 32767.0f;

    /* 反向展开，防止 8 字节 float 输出覆盖尚未读取的 4 字节 Q15 输入。 */
    for (index = count; index > 0U; --index)
    {
        uint32_t source_index = index - 1U;
        q15_t real = fixed_data[2U * source_index];
        q15_t imag = fixed_data[2U * source_index + 1U];
        data[source_index].real = (float)real * restore_scale;
        data[source_index].imag = (float)imag * restore_scale;
    }
    return SIGNAL_ALGORITHM_OK;
}

#elif SIGNAL_FFT_BACKEND == SIGNAL_FFT_BACKEND_CMSIS_Q31

static const arm_cfft_instance_q31 *SignalFFT_GetQ31Instance(uint32_t count)
{
    switch (count)
    {
        case 16U: return &arm_cfft_sR_q31_len16;
        case 32U: return &arm_cfft_sR_q31_len32;
        case 64U: return &arm_cfft_sR_q31_len64;
        case 128U: return &arm_cfft_sR_q31_len128;
        case 256U: return &arm_cfft_sR_q31_len256;
        case 512U: return &arm_cfft_sR_q31_len512;
        case 1024U: return &arm_cfft_sR_q31_len1024;
        case 2048U: return &arm_cfft_sR_q31_len2048;
        case 4096U: return &arm_cfft_sR_q31_len4096;
        default: return NULL;
    }
}

static signal_algorithm_status_t SignalFFT_ForwardComplexCMSISQ31(
    signal_complex_f32_t *data,
    uint32_t count)
{
    const arm_cfft_instance_q31 *instance = SignalFFT_GetQ31Instance(count);
    q31_t *fixed_data = (q31_t *)(void *)data;
    float maximum_absolute = 0.0f;
    float restore_scale;
    uint32_t index;

    if (instance == NULL)
    {
        return SIGNAL_ALGORITHM_NOT_SUPPORTED;
    }
    for (index = 0U; index < count; ++index)
    {
        float real_absolute = fabsf(data[index].real);
        float imag_absolute = fabsf(data[index].imag);
        if (real_absolute > maximum_absolute)
        {
            maximum_absolute = real_absolute;
        }
        if (imag_absolute > maximum_absolute)
        {
            maximum_absolute = imag_absolute;
        }
    }
    if (maximum_absolute == 0.0f)
    {
        return SIGNAL_ALGORITHM_OK;
    }
    for (index = 0U; index < count; ++index)
    {
        float real = data[index].real / maximum_absolute;
        float imag = data[index].imag / maximum_absolute;
        fixed_data[2U * index] = (q31_t)(real * 2147483520.0f);
        fixed_data[2U * index + 1U] = (q31_t)(imag * 2147483520.0f);
    }

    arm_cfft_q31(instance, fixed_data, 0U, 1U);
    restore_scale = maximum_absolute * (float)count / 2147483520.0f;
    for (index = 0U; index < count; ++index)
    {
        q31_t real = fixed_data[2U * index];
        q31_t imag = fixed_data[2U * index + 1U];
        data[index].real = (float)real * restore_scale;
        data[index].imag = (float)imag * restore_scale;
    }
    return SIGNAL_ALGORITHM_OK;
}

#else

static const arm_cfft_instance_f32 *SignalFFT_GetF32Instance(uint32_t count)
{
    switch (count)
    {
        case 16U: return &arm_cfft_sR_f32_len16;
        case 32U: return &arm_cfft_sR_f32_len32;
        case 64U: return &arm_cfft_sR_f32_len64;
        case 128U: return &arm_cfft_sR_f32_len128;
        case 256U: return &arm_cfft_sR_f32_len256;
        case 512U: return &arm_cfft_sR_f32_len512;
        case 1024U: return &arm_cfft_sR_f32_len1024;
        case 2048U: return &arm_cfft_sR_f32_len2048;
        case 4096U: return &arm_cfft_sR_f32_len4096;
        default: return NULL;
    }
}

static signal_algorithm_status_t SignalFFT_ForwardComplexCMSISF32(
    signal_complex_f32_t *data,
    uint32_t count)
{
    const arm_cfft_instance_f32 *instance = SignalFFT_GetF32Instance(count);

    if (instance == NULL)
    {
        return SIGNAL_ALGORITHM_NOT_SUPPORTED;
    }
    if (sizeof(signal_complex_f32_t) != (2U * sizeof(float32_t)))
    {
        return SIGNAL_ALGORITHM_NOT_SUPPORTED;
    }
    arm_cfft_f32(instance, (float32_t *)(void *)data, 0U, 1U);
    return SIGNAL_ALGORITHM_OK;
}

#endif
#endif

signal_algorithm_status_t SignalFFT_ForwardComplexInPlace(
    signal_complex_f32_t *data,
    uint32_t count)
{
    signal_algorithm_status_t status;
    uint32_t index;

    if (data == NULL)
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if ((count < 2U) || !SignalFFT_IsPowerOfTwo(count))
    {
        return SIGNAL_ALGORITHM_NOT_SUPPORTED;
    }

#if SIGNAL_FFT_BACKEND == SIGNAL_FFT_BACKEND_REFERENCE_C
    status = SignalFFT_ForwardComplexReference(data, count);
#else
    if (!SignalFFT_InputIsFinite(data, count))
    {
        return SIGNAL_ALGORITHM_NUMERIC_ERROR;
    }
#if SIGNAL_FFT_BACKEND == SIGNAL_FFT_BACKEND_CMSIS_Q15
    status = SignalFFT_ForwardComplexCMSISQ15(data, count);
#elif SIGNAL_FFT_BACKEND == SIGNAL_FFT_BACKEND_CMSIS_Q31
    status = SignalFFT_ForwardComplexCMSISQ31(data, count);
#else
    status = SignalFFT_ForwardComplexCMSISF32(data, count);
#endif
    /* CMSIS CFFT 表只覆盖 16..4096；其他合法 2 次幂继续走 Reference，保持旧行为。 */
    if (status == SIGNAL_ALGORITHM_NOT_SUPPORTED)
    {
        status = SignalFFT_ForwardComplexReference(data, count);
    }
#endif

    if (status != SIGNAL_ALGORITHM_OK)
    {
        return status;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(data[index].real) || !isfinite(data[index].imag))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
    }
    return SIGNAL_ALGORITHM_OK;
}

signal_algorithm_status_t SignalFFT_ForwardReal(
    const float *input_samples,
    signal_complex_f32_t *spectrum,
    uint32_t count,
    uint32_t spectrum_capacity)
{
    uint32_t index;

    if ((input_samples == NULL) || (spectrum == NULL))
    {
        return SIGNAL_ALGORITHM_INVALID_ARGUMENT;
    }
    if (spectrum_capacity < count)
    {
        return SIGNAL_ALGORITHM_BUFFER_TOO_SMALL;
    }
    if ((count < 2U) || !SignalFFT_IsPowerOfTwo(count))
    {
        return SIGNAL_ALGORITHM_NOT_SUPPORTED;
    }
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(input_samples[index]))
        {
            return SIGNAL_ALGORITHM_NUMERIC_ERROR;
        }
        spectrum[index].real = input_samples[index];
        spectrum[index].imag = 0.0f;
    }
    return SignalFFT_ForwardComplexInPlace(spectrum, count);
}
