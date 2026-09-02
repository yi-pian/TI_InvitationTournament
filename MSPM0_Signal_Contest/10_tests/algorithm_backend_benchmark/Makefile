CC := gcc
SDK_ROOT ?= C:/ti/mspm0_sdk_2_11_00_07

CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -pedantic -fno-strict-aliasing
ALGORITHM_INCLUDES := \
	-I../../03_measurement/common \
	-I../../11_legacy_compatibility/algorithms/04_dsp/fft \
	-I../../04_dsp/backend_adapter
CMSIS_INCLUDES := \
	-I$(SDK_ROOT)/source/third_party/CMSIS/DSP/Include \
	-I$(SDK_ROOT)/source/third_party/CMSIS/Core/Include

ALGORITHM_SOURCES := \
	test_fft_backends.c \
	../../11_legacy_compatibility/algorithms/04_dsp/fft/signal_fft.c \
	../../04_dsp/backend_adapter/signal_backend_adapter.c

CMSIS_SOURCE_ROOT := $(SDK_ROOT)/source/third_party/CMSIS/DSP/Source
CMSIS_SOURCES := \
	$(CMSIS_SOURCE_ROOT)/CommonTables/arm_common_tables.c \
	$(CMSIS_SOURCE_ROOT)/CommonTables/arm_const_structs.c \
	$(CMSIS_SOURCE_ROOT)/TransformFunctions/arm_bitreversal.c \
	$(CMSIS_SOURCE_ROOT)/TransformFunctions/arm_bitreversal2.c \
	$(CMSIS_SOURCE_ROOT)/TransformFunctions/arm_cfft_q15.c \
	$(CMSIS_SOURCE_ROOT)/TransformFunctions/arm_cfft_radix4_q15.c \
	$(CMSIS_SOURCE_ROOT)/TransformFunctions/arm_cfft_q31.c \
	$(CMSIS_SOURCE_ROOT)/TransformFunctions/arm_cfft_radix4_q31.c \
	$(CMSIS_SOURCE_ROOT)/TransformFunctions/arm_cfft_f32.c \
	$(CMSIS_SOURCE_ROOT)/TransformFunctions/arm_cfft_radix8_f32.c

.PHONY: all test clean

all: build/reference build/cmsis_q15 build/cmsis_q31 build/cmsis_f32

build:
	if not exist build mkdir build

build/reference: $(ALGORITHM_SOURCES) | build
	$(CC) $(CFLAGS) -DARM_MATH_CM0 $(ALGORITHM_INCLUDES) $(CMSIS_INCLUDES) \
		-DSIGNAL_FFT_BACKEND=0 \
		-DBENCH_BACKEND_NAME=\"REFERENCE_C\" \
		-DBENCH_NORMALIZED_TOLERANCE=0.0005f \
		$(ALGORITHM_SOURCES) -o $@ -lm

build/cmsis_q15: $(ALGORITHM_SOURCES) $(CMSIS_SOURCES) | build
	$(CC) $(CFLAGS) -DARM_MATH_CM0 $(ALGORITHM_INCLUDES) $(CMSIS_INCLUDES) \
		-DSIGNAL_FFT_BACKEND=1 -DBENCH_BACKEND_NAME=\"CMSIS_Q15\" \
		-DBENCH_NORMALIZED_TOLERANCE=0.02f \
		$(ALGORITHM_SOURCES) $(CMSIS_SOURCES) -o $@ -lm

build/cmsis_q31: $(ALGORITHM_SOURCES) $(CMSIS_SOURCES) | build
	$(CC) $(CFLAGS) -DARM_MATH_CM0 $(ALGORITHM_INCLUDES) $(CMSIS_INCLUDES) \
		-DSIGNAL_FFT_BACKEND=2 -DBENCH_BACKEND_NAME=\"CMSIS_Q31\" \
		-DBENCH_NORMALIZED_TOLERANCE=0.001f \
		$(ALGORITHM_SOURCES) $(CMSIS_SOURCES) -o $@ -lm

build/cmsis_f32: $(ALGORITHM_SOURCES) $(CMSIS_SOURCES) | build
	$(CC) $(CFLAGS) -DARM_MATH_CM0 $(ALGORITHM_INCLUDES) $(CMSIS_INCLUDES) \
		-DSIGNAL_FFT_BACKEND=3 -DBENCH_BACKEND_NAME=\"CMSIS_F32\" \
		-DBENCH_NORMALIZED_TOLERANCE=0.0005f \
		$(ALGORITHM_SOURCES) $(CMSIS_SOURCES) -o $@ -lm

test: all
	./build/reference
	./build/cmsis_q15
	./build/cmsis_q31
	./build/cmsis_f32

clean:
	if exist build rmdir /S /Q build
