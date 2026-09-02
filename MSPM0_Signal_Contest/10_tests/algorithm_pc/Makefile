CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -pedantic -DSIGNAL_FFT_BACKEND=0
LDLIBS := -lm
SDK_ROOT ?= C:/ti/mspm0_sdk_2_11_00_07

INCLUDES := \
	-I$(SDK_ROOT)/source/third_party/CMSIS/DSP/Include \
	-I$(SDK_ROOT)/source/third_party/CMSIS/Core/Include \
	-I../../03_measurement/common \
	-I../../11_legacy_compatibility/algorithms/03_measurement/adc_to_voltage \
	-I../../11_legacy_compatibility/algorithms/03_measurement/mean \
	-I../../11_legacy_compatibility/algorithms/03_measurement/minmax \
	-I../../11_legacy_compatibility/algorithms/03_measurement/vpp \
	-I../../11_legacy_compatibility/algorithms/03_measurement/rms \
	-I../../11_legacy_compatibility/algorithms/03_measurement/ac_rms \
	-I../../11_legacy_compatibility/algorithms/03_measurement/statistics \
	-I../../11_legacy_compatibility/algorithms/04_dsp/remove_dc \
	-I../../11_legacy_compatibility/algorithms/04_dsp/clipping_detect \
	-I../../03_measurement/frequency_zero_cross \
	-I../../05_precision/zero_cross_interpolation \
	-I../../11_legacy_compatibility/algorithms/05_precision/multi_cycle_average \
	-I../../04_dsp/moving_average \
	-I../../04_dsp/median_filter \
	-I../../04_dsp/mad \
	-I../../04_dsp/hampel_filter \
	-I../../11_legacy_compatibility/algorithms/04_dsp/fir \
	-I../../11_legacy_compatibility/algorithms/04_dsp/iir_biquad \
	-I../../04_dsp/window \
	-I../../04_dsp/window/rectangular \
	-I../../04_dsp/window/hann \
	-I../../04_dsp/window/hamming \
	-I../../04_dsp/window/blackman \
	-I../../11_legacy_compatibility/algorithms/04_dsp/fft \
	-I../../11_legacy_compatibility/algorithms/04_dsp/fft_magnitude \
	-I../../11_legacy_compatibility/algorithms/04_dsp/peak_detect \
	-I../../05_precision/fft_parabolic_interpolation \
	-I../../05_precision/log_parabolic_interpolation \
	-I../../05_precision/window_gain_correction \
	-I../../05_precision/multi_bin_energy \
	-I../../04_dsp/harmonic \
	-I../../04_dsp/thd \
	-I../../04_dsp/snr \
	-I../../04_dsp/sfdr \
	-I../../03_measurement/phase \
	-I../../04_dsp/correlation \
	-I../../04_dsp/autocorrelation \
	-I../../05_precision/adc_gain_offset_calibration \
	-I../../05_precision/channel_delay_calibration \
	-I../../05_precision/robust_peak_to_peak \
	-I../../05_precision/robust_rms \
	-I../../05_precision/sine_fit_3param \
	-I../../05_precision/sine_fit_4param \
	-I../../05_precision/lock_in

FIRST_BATCH_SOURCES := \
	test_first_batch.c \
	../../11_legacy_compatibility/algorithms/03_measurement/adc_to_voltage/signal_adc_to_voltage.c \
	../../11_legacy_compatibility/algorithms/03_measurement/mean/signal_mean.c \
	../../11_legacy_compatibility/algorithms/03_measurement/minmax/signal_minmax.c \
	../../11_legacy_compatibility/algorithms/03_measurement/vpp/signal_vpp.c \
	../../11_legacy_compatibility/algorithms/03_measurement/rms/signal_rms.c \
	../../11_legacy_compatibility/algorithms/03_measurement/ac_rms/signal_ac_rms.c \
	../../11_legacy_compatibility/algorithms/03_measurement/statistics/signal_statistics.c \
	../../11_legacy_compatibility/algorithms/04_dsp/remove_dc/signal_remove_dc.c \
	../../11_legacy_compatibility/algorithms/04_dsp/clipping_detect/signal_clipping_detect.c

SECOND_BATCH_SOURCES := \
	test_second_batch.c \
	../../03_measurement/frequency_zero_cross/signal_zero_cross.c \
	../../05_precision/zero_cross_interpolation/signal_zero_cross_interpolation.c \
	../../11_legacy_compatibility/algorithms/05_precision/multi_cycle_average/signal_multi_cycle_average.c \
	../../11_legacy_compatibility/algorithms/04_dsp/remove_dc/signal_remove_dc.c

THIRD_BATCH_SOURCES := \
	test_third_batch.c \
	../../04_dsp/moving_average/signal_moving_average.c \
	../../04_dsp/median_filter/signal_median_filter.c \
	../../04_dsp/mad/signal_mad.c \
	../../04_dsp/hampel_filter/signal_hampel.c \
	../../11_legacy_compatibility/algorithms/04_dsp/fir/signal_fir.c \
	../../11_legacy_compatibility/algorithms/04_dsp/iir_biquad/signal_iir_biquad.c

FOURTH_BATCH_SOURCES := \
	test_fourth_batch.c \
	../../04_dsp/window/signal_window.c \
	../../04_dsp/window/rectangular/signal_rectangular.c \
	../../04_dsp/window/hann/signal_hann.c \
	../../04_dsp/window/hamming/signal_hamming.c \
	../../04_dsp/window/blackman/signal_blackman.c \
	../../11_legacy_compatibility/algorithms/04_dsp/fft/signal_fft.c \
	../../11_legacy_compatibility/algorithms/04_dsp/fft_magnitude/signal_fft_magnitude.c \
	../../11_legacy_compatibility/algorithms/04_dsp/peak_detect/signal_peak_detect.c \
	../../05_precision/fft_parabolic_interpolation/signal_fft_parabolic_interpolation.c \
	../../05_precision/log_parabolic_interpolation/signal_log_parabolic_interpolation.c \
	../../05_precision/window_gain_correction/signal_window_gain_correction.c

FIFTH_BATCH_SOURCES := \
	test_fifth_batch.c \
	../../04_dsp/window/signal_window.c \
	../../04_dsp/window/hann/signal_hann.c \
	../../11_legacy_compatibility/algorithms/04_dsp/fft/signal_fft.c \
	../../11_legacy_compatibility/algorithms/04_dsp/fft_magnitude/signal_fft_magnitude.c \
	../../05_precision/multi_bin_energy/signal_multi_bin_energy.c \
	../../04_dsp/harmonic/signal_harmonic.c \
	../../04_dsp/thd/signal_thd.c \
	../../04_dsp/snr/signal_snr.c \
	../../04_dsp/sfdr/signal_sfdr.c

SIXTH_BATCH_SOURCES := \
	test_sixth_batch.c \
	../../03_measurement/phase/signal_phase.c \
	../../04_dsp/correlation/signal_correlation.c \
	../../04_dsp/autocorrelation/signal_autocorrelation.c \
	../../11_legacy_compatibility/algorithms/04_dsp/fft/signal_fft.c

SEVENTH_BATCH_SOURCES := \
	test_seventh_batch.c \
	../../05_precision/adc_gain_offset_calibration/signal_adc_gain_offset_calibration.c \
	../../05_precision/channel_delay_calibration/signal_channel_delay_calibration.c \
	../../05_precision/robust_peak_to_peak/signal_robust_peak_to_peak.c \
	../../05_precision/robust_rms/signal_robust_rms.c \
	../../05_precision/sine_fit_3param/signal_sine_fit_3param.c \
	../../05_precision/sine_fit_4param/signal_sine_fit_4param.c \
	../../05_precision/lock_in/signal_lock_in.c

VECTOR_SOURCES := \
	test_signal_vectors.c \
	signal_test_vectors.c

.PHONY: all test clean

all: build/test_first_batch build/test_second_batch build/test_third_batch build/test_fourth_batch build/test_fifth_batch build/test_sixth_batch build/test_seventh_batch build/test_signal_vectors

build:
	if not exist build mkdir build

build/test_first_batch: $(FIRST_BATCH_SOURCES) | build
	$(CC) $(CFLAGS) $(INCLUDES) $(FIRST_BATCH_SOURCES) -o $@ $(LDLIBS)

build/test_second_batch: $(SECOND_BATCH_SOURCES) | build
	$(CC) $(CFLAGS) $(INCLUDES) $(SECOND_BATCH_SOURCES) -o $@ $(LDLIBS)

build/test_third_batch: $(THIRD_BATCH_SOURCES) | build
	$(CC) $(CFLAGS) $(INCLUDES) $(THIRD_BATCH_SOURCES) -o $@ $(LDLIBS)

build/test_fourth_batch: $(FOURTH_BATCH_SOURCES) | build
	$(CC) $(CFLAGS) $(INCLUDES) $(FOURTH_BATCH_SOURCES) -o $@ $(LDLIBS)

build/test_fifth_batch: $(FIFTH_BATCH_SOURCES) | build
	$(CC) $(CFLAGS) $(INCLUDES) $(FIFTH_BATCH_SOURCES) -o $@ $(LDLIBS)

build/test_sixth_batch: $(SIXTH_BATCH_SOURCES) | build
	$(CC) $(CFLAGS) $(INCLUDES) $(SIXTH_BATCH_SOURCES) -o $@ $(LDLIBS)

build/test_seventh_batch: $(SEVENTH_BATCH_SOURCES) | build
	$(CC) $(CFLAGS) $(INCLUDES) $(SEVENTH_BATCH_SOURCES) -o $@ $(LDLIBS)

build/test_signal_vectors: $(VECTOR_SOURCES) | build
	$(CC) $(CFLAGS) $(INCLUDES) $(VECTOR_SOURCES) -o $@ $(LDLIBS)

test: build/test_first_batch build/test_second_batch build/test_third_batch build/test_fourth_batch build/test_fifth_batch build/test_sixth_batch build/test_seventh_batch build/test_signal_vectors
	./build/test_first_batch
	./build/test_second_batch
	./build/test_third_batch
	./build/test_fourth_batch
	./build/test_fifth_batch
	./build/test_sixth_batch
	./build/test_seventh_batch
	./build/test_signal_vectors

clean:
	if exist build\test_first_batch del /Q build\test_first_batch
	if exist build\test_first_batch.exe del /Q build\test_first_batch.exe
	if exist build\test_second_batch del /Q build\test_second_batch
	if exist build\test_second_batch.exe del /Q build\test_second_batch.exe
	if exist build\test_third_batch del /Q build\test_third_batch
	if exist build\test_third_batch.exe del /Q build\test_third_batch.exe
	if exist build\test_fourth_batch del /Q build\test_fourth_batch
	if exist build\test_fourth_batch.exe del /Q build\test_fourth_batch.exe
	if exist build\test_fifth_batch del /Q build\test_fifth_batch
	if exist build\test_fifth_batch.exe del /Q build\test_fifth_batch.exe
	if exist build\test_sixth_batch del /Q build\test_sixth_batch
	if exist build\test_sixth_batch.exe del /Q build\test_sixth_batch.exe
	if exist build\test_seventh_batch del /Q build\test_seventh_batch
	if exist build\test_seventh_batch.exe del /Q build\test_seventh_batch.exe
	if exist build\test_signal_vectors del /Q build\test_signal_vectors
	if exist build\test_signal_vectors.exe del /Q build\test_signal_vectors.exe
