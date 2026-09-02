# 三线 GPIO 器件接入 Recipe

适用于 X9C104 一类 `CS + U/D + INC` 的数字电位器，以及不是标准 SPI、但依靠三个 GPIO 时序控制的器件。

## 1. 为什么不要硬套 SPI

X9C 的 `INC` 下降沿让滑动端移动一步，`U/D` 决定方向，`CS` 决定选中/保存。它没有标准 SPI 的 MOSI/MISO 字节帧。用三个 GPIO 写时序更直接，也更容易与数据手册逐边沿核对。

## 2. SysConfig

1. 添加 3 个 GPIO Output：`X9C_CS`、`X9C_UD`、`X9C_INC`。
2. 初始值：CS 高（未选中）、INC 高；U/D 任意但建议低。
3. 若使用软件延时，确认系统时钟。首次联调可用保守微秒级 delay，不能用编译器可能优化掉的空循环。

## 3. 移动一步

### 【比赛现场直接复制】

```c
static void x9c_step(bool up)
{
    if (up) {
        DL_GPIO_setPins(GPIO_X9C_PORT, GPIO_X9C_UD_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_X9C_PORT, GPIO_X9C_UD_PIN);
    }

    delay_us(3U);                    /* 满足 U/D 建立时间 */
    DL_GPIO_clearPins(GPIO_X9C_PORT, GPIO_X9C_CS_PIN);
    DL_GPIO_clearPins(GPIO_X9C_PORT, GPIO_X9C_INC_PIN);
    delay_us(2U);
    DL_GPIO_setPins(GPIO_X9C_PORT, GPIO_X9C_INC_PIN);
    delay_us(100U);                  /* 等待滑动端稳定 */
    DL_GPIO_setPins(GPIO_X9C_PORT, GPIO_X9C_CS_PIN);
}
```

`delay_us()` 可用已验证的项目延时函数；具体时序数字必须按器件数据手册替换。

## 4. 保存还是不保存

很多三线数字电位器在取消 CS 时决定是否写非易失存储。X9C 系列中，CS 在 INC 为高时上升会存储，存储期间需等待；频繁存储会增加等待并消耗写寿命。比赛中连续调节时，保持 CS 低完成多步，只在最终值需要掉电保留时保存。

## 5. 没有绝对位置读回

X9C 没有位置寄存器回读。MCU 只能维护“自己走了多少步”，掉电后的绝对位置依赖上次存储。需要可靠定位时，可先向一个方向走超过总步数使其到端点，再反向走到目标位置；但要检查滑动端和端点允许电流。

