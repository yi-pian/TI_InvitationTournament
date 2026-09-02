#include "calibration.h"

int main(void)
{
    float y = 0.0f;
    if (!Calibration_Apply(2.5f, &y)) return 1;
    if ((y < 5.999f) || (y > 6.001f)) return 2;
    if (Calibration_Apply(-1.0f, &y)) return 3;
    return 0;
}

