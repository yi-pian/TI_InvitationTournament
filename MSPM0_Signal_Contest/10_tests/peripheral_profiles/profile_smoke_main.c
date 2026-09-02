/**
 * @file profile_smoke_main.c
 * @brief TEST ONLY entry point used to compile and link SysConfig profiles.
 *
 * This file deliberately does not implement a signal algorithm.  A successful
 * build proves that the generated pin, clock, event, DMA and peripheral setup
 * can coexist in one MSPM0G3507 image.
 */

#include "ti_msp_dl_config.h"

int main(void)
{
    SYSCFG_DL_init();

    for (;;) {
        __WFI();
    }
}
