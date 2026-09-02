# I2C 寄存器器件接入 Recipe

适用于 ADS112C04、TPL0401、TCA6408A 等“7-bit 地址 + 寄存器/命令 + 数据”的器件。

## 1. 接线与上拉

| 器件 | MSPM0G3507 | 说明 |
|---|---|---|
| SDA | I2C SDA | 双向开漏 |
| SCL | I2C SCL | 开漏时钟 |
| ADDR/A0/A1 | GND/VDD/SDA/SCL（按手册） | 决定 7-bit 地址 |
| INT/DRDY | 可选 GPIO 输入 | 初次可先轮询寄存器 |
| RESET | 可选 GPIO 输出 | 没有则用软件命令 |
| GND | GND | 必须共地 |

SDA 和 SCL 必须有上拉。很多模块板已有上拉，裸芯片通常没有。上拉电压不得高于 MSPM0 引脚允许电压；不确定时用 3.3 V。

## 2. SysConfig 手动配置

1. 添加 I2C 外设并选择 **Controller**。
2. 选择 SDA、SCL 引脚，Standard mode 100 kHz 起步。
3. 保留开漏复用功能；确认板上有 2.2 kΩ～10 kΩ 量级上拉，常用 4.7 kΩ。
4. 本 Recipe 采用轮询，不需要先开启 I2C 中断。
5. 保存后查看 `ti_msp_dl_config.h`，使用真实生成宏，例如 `I2C_0_INST`。

## 3. 地址不要左移

DriverLib `DL_I2C_startControllerTransfer()` 接收 **7-bit 地址**。例如数据手册写 `0x40`，就传 `0x40`，不要传 `0x80`。若手册给出 8-bit 写地址/读地址，需要先右移一位得到 7-bit 地址。

## 4. 写一个寄存器

仓库公共实现 `MSPM0_EXT_I2C_Write()` 会填 TX FIFO、调用：

```c
DL_I2C_startControllerTransfer(i2c,
                               address_7bit,
                               DL_I2C_CONTROLLER_DIRECTION_TX,
                               count);
```

### 【比赛现场直接复制】

```c
uint8_t tx[2] = { register_address, register_value };
bool ok = MSPM0_EXT_I2C_Write(I2C_0_INST, DEVICE_ADDRESS, tx, 2U);
```

工程需加入：

- `12_external_devices/00_common/mspm0_blocking_bus.c`
- 该目录加入 include path，并 `#include "mspm0_blocking_bus.h"`

## 5. 读一个寄存器：重复起始

典型流程是：START + 地址写 + 寄存器地址 + repeated START + 地址读 + 数据 + STOP。

```c
uint8_t reg = register_address;
uint8_t value;
bool ok = MSPM0_EXT_I2C_WriteRead(I2C_0_INST,
                                  DEVICE_ADDRESS,
                                  &reg, 1U,
                                  &value, 1U);
```

不要默认“写完寄存器地址后先 STOP”也能工作；是否允许由器件协议决定。公共 helper 使用 repeated START。

## 6. 完整最小上下文

```c
#include "ti_msp_dl_config.h"
#include "mspm0_blocking_bus.h"

#define DEVICE_ADDRESS  (0x40U)

int main(void)
{
    uint8_t value = 0U;
    SYSCFG_DL_init();

    uint8_t reg = 0x00U;
    bool ok = MSPM0_EXT_I2C_WriteRead(I2C_0_INST,
                                     DEVICE_ADDRESS,
                                     &reg, 1U,
                                     &value, 1U);
    (void)ok;
    while (1) {
    }
}
```

## 7. 先用什么验证

优先读取有固定复位值的寄存器或器件 ID。数字电位器可先写中间位置再读回；GPIO 扩展器可先读默认方向寄存器。出现 NACK 时依次检查：共地、上拉、7-bit 地址、ADDR 管脚、供电、SDA/SCL 是否接反、器件是否仍在复位。

首次 bring-up 不要同时开启 DMA、I2C 中断状态机和多个器件。先让一笔阻塞读写稳定，再扩展。

