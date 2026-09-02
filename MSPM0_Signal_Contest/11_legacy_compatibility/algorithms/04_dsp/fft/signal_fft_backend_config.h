#ifndef SIGNAL_FFT_BACKEND_CONFIG_H
#define SIGNAL_FFT_BACKEND_CONFIG_H

/*
 * All contest targets are CMSIS-DSP-ready. Q15 is the default candidate until
 * board cycle benchmarks select a project-specific default. Reference C is
 * retained only for PC truth comparison and explicit compatibility tests.
 */
#define SIGNAL_FFT_BACKEND_REFERENCE_C 0
#define SIGNAL_FFT_BACKEND_CMSIS_Q15   1
#define SIGNAL_FFT_BACKEND_CMSIS_Q31   2
#define SIGNAL_FFT_BACKEND_CMSIS_F32   3

#ifndef SIGNAL_FFT_BACKEND
#define SIGNAL_FFT_BACKEND SIGNAL_FFT_BACKEND_CMSIS_Q15
#endif

#if ((SIGNAL_FFT_BACKEND != SIGNAL_FFT_BACKEND_REFERENCE_C) && \
     (SIGNAL_FFT_BACKEND != SIGNAL_FFT_BACKEND_CMSIS_Q15) && \
     (SIGNAL_FFT_BACKEND != SIGNAL_FFT_BACKEND_CMSIS_Q31) && \
     (SIGNAL_FFT_BACKEND != SIGNAL_FFT_BACKEND_CMSIS_F32))
#error "Unsupported SIGNAL_FFT_BACKEND value"
#endif

#endif /* SIGNAL_FFT_BACKEND_CONFIG_H */
