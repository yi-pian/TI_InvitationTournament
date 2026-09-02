#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "signal_config.h"

volatile int32_t g_contest_status;

int main(void)
{
    SYSCFG_DL_init();

    /* ===== 模块初始化区：按模块 README 粘贴到这里 ===== */

    while (1) {
        /* ===== 模块调用区：按模块 README 粘贴到这里 ===== */

        /* ===== 这里写你自己的题目逻辑 ===== */
        g_contest_status = 0;

        __WFI();
    }
}
