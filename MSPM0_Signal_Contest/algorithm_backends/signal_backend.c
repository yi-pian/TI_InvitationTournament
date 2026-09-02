#include "signal_backend.h"

const char *SignalBackend_Name(signal_backend_id_t backend)
{
    switch (backend) {
        case SIGNAL_BACKEND_REFERENCE_C: return "REFERENCE_C";
        case SIGNAL_BACKEND_CMSIS_DSP_Q15: return "CMSIS_DSP_Q15";
        case SIGNAL_BACKEND_CMSIS_DSP_Q31: return "CMSIS_DSP_Q31";
        case SIGNAL_BACKEND_CMSIS_DSP_F32: return "CMSIS_DSP_F32";
        case SIGNAL_BACKEND_IQMATH_RTS_Q24: return "IQMATH_RTS_Q24";
        case SIGNAL_BACKEND_IQMATH_MATHACL_Q24:
            return "IQMATH_MATHACL_Q24";
        default: return "UNKNOWN";
    }
}
