cmake_minimum_required(VERSION 3.20)
project(signal_backend_benchmark C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(MSPM0_SDK_ROOT "" CACHE PATH "MSPM0 SDK 2.11.00.07 root")
if(NOT MSPM0_SDK_ROOT)
    message(FATAL_ERROR "Set -DMSPM0_SDK_ROOT=<SDK root>")
endif()

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(CMSIS_ROOT "${MSPM0_SDK_ROOT}/source/third_party/CMSIS")

add_executable(backend_benchmark_host
    backend_benchmark_host.c
    ${REPO_ROOT}/algorithm_backends/signal_backend.c
    ${REPO_ROOT}/algorithm_backends/signal_fixed_point.c
    ${REPO_ROOT}/algorithm_backends/reference/signal_reference_backend.c
    ${REPO_ROOT}/algorithm_backends/cmsis_dsp/signal_cmsis_dsp_backend.c
    ${REPO_ROOT}/11_legacy_compatibility/algorithms/04_dsp/fft/signal_fft.c

    ${CMSIS_ROOT}/DSP/Source/CommonTables/arm_common_tables.c
    ${CMSIS_ROOT}/DSP/Source/CommonTables/arm_const_structs.c
    ${CMSIS_ROOT}/DSP/Source/BasicMathFunctions/arm_abs_q15.c
    ${CMSIS_ROOT}/DSP/Source/TransformFunctions/arm_bitreversal.c
    ${CMSIS_ROOT}/DSP/Source/TransformFunctions/arm_bitreversal2.c
    ${CMSIS_ROOT}/DSP/Source/TransformFunctions/arm_cfft_f32.c
    ${CMSIS_ROOT}/DSP/Source/TransformFunctions/arm_cfft_q15.c
    ${CMSIS_ROOT}/DSP/Source/TransformFunctions/arm_cfft_q31.c
    ${CMSIS_ROOT}/DSP/Source/TransformFunctions/arm_cfft_radix4_f32.c
    ${CMSIS_ROOT}/DSP/Source/TransformFunctions/arm_cfft_radix4_q15.c
    ${CMSIS_ROOT}/DSP/Source/TransformFunctions/arm_cfft_radix4_q31.c
    ${CMSIS_ROOT}/DSP/Source/TransformFunctions/arm_cfft_radix8_f32.c
    ${CMSIS_ROOT}/DSP/Source/ComplexMathFunctions/arm_cmplx_mag_f32.c
    ${CMSIS_ROOT}/DSP/Source/ComplexMathFunctions/arm_cmplx_mag_q15.c
    ${CMSIS_ROOT}/DSP/Source/StatisticsFunctions/arm_rms_f32.c
    ${CMSIS_ROOT}/DSP/Source/StatisticsFunctions/arm_rms_q15.c
    ${CMSIS_ROOT}/DSP/Source/FastMathFunctions/arm_atan2_f32.c
    ${CMSIS_ROOT}/DSP/Source/FastMathFunctions/arm_atan2_q15.c
    ${CMSIS_ROOT}/DSP/Source/FastMathFunctions/arm_divide_q15.c
    ${CMSIS_ROOT}/DSP/Source/FastMathFunctions/arm_cos_f32.c
    ${CMSIS_ROOT}/DSP/Source/FastMathFunctions/arm_cos_q15.c
    ${CMSIS_ROOT}/DSP/Source/FastMathFunctions/arm_sin_f32.c
    ${CMSIS_ROOT}/DSP/Source/FastMathFunctions/arm_sin_q15.c
    ${CMSIS_ROOT}/DSP/Source/FastMathFunctions/arm_sqrt_q15.c
    ${CMSIS_ROOT}/DSP/Source/FastMathFunctions/arm_sqrt_q31.c
)

target_compile_definitions(backend_benchmark_host PRIVATE ARM_MATH_CM0)
target_include_directories(backend_benchmark_host PRIVATE
    ${REPO_ROOT}/01_bsp/common
    ${REPO_ROOT}/11_legacy_compatibility/algorithms/04_dsp/fft
    ${REPO_ROOT}/algorithm_backends
    ${REPO_ROOT}/algorithm_backends/reference
    ${REPO_ROOT}/algorithm_backends/cmsis_dsp
    ${CMSIS_ROOT}/Core/Include
    ${CMSIS_ROOT}/DSP/Include
    ${CMSIS_ROOT}/DSP/PrivateInclude
)

if(MSVC)
    target_compile_options(backend_benchmark_host PRIVATE /W4 /WX)
else()
    target_compile_options(backend_benchmark_host PRIVATE
        -Wall -Wextra -Werror)
    target_link_libraries(backend_benchmark_host PRIVATE m)
endif()
