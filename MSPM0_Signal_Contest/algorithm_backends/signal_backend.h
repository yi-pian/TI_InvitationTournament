#ifndef SIGNAL_BACKEND_H
#define SIGNAL_BACKEND_H

/** Backend identity only. Selection remains explicit at build/test level. */
typedef enum {
    SIGNAL_BACKEND_REFERENCE_C = 0,
    SIGNAL_BACKEND_CMSIS_DSP_Q15,
    SIGNAL_BACKEND_CMSIS_DSP_Q31,
    SIGNAL_BACKEND_CMSIS_DSP_F32,
    SIGNAL_BACKEND_IQMATH_RTS_Q24,
    SIGNAL_BACKEND_IQMATH_MATHACL_Q24
} signal_backend_id_t;

const char *SignalBackend_Name(signal_backend_id_t backend);

#endif
