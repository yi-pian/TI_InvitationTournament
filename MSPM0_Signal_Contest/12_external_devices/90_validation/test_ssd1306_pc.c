#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ssd1306.h"

typedef struct {
    uint8_t controls[32];
    size_t counts[32];
    uint8_t first_bytes[32];
    size_t calls;
    bool fail;
} fake_bus_t;

static bool fake_write(void *context,
    uint8_t control,
    const uint8_t *data,
    size_t count)
{
    fake_bus_t *bus = (fake_bus_t *) context;

    if (bus->fail || (data == NULL) || (count == 0U) ||
        (bus->calls >= 32U)) {
        return false;
    }
    bus->controls[bus->calls] = control;
    bus->counts[bus->calls] = count;
    bus->first_bytes[bus->calls] = data[0];
    ++bus->calls;
    return true;
}

#define CHECK(expression) do { \
    if (!(expression)) { \
        (void) fprintf(stderr, "CHECK failed: %s:%d: %s\n", \
            __FILE__, __LINE__, #expression); \
        return 1; \
    } \
} while (0)

int main(void)
{
    fake_bus_t bus;
    ssd1306_t display;
    uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE];
    const ssd1306_config_t config = {
        .io_context = &bus,
        .write = fake_write,
        .contrast = 0xCFU,
        .rotate_180 = false
    };
    size_t calls_before_update;

    (void) memset(&bus, 0, sizeof(bus));
    (void) memset(&display, 0, sizeof(display));
    (void) memset(framebuffer, 0xFF, sizeof(framebuffer));

    SSD1306_ClearBuffer(framebuffer);
    CHECK(framebuffer[0] == 0U);
    CHECK(framebuffer[SSD1306_FRAMEBUFFER_SIZE - 1U] == 0U);
    CHECK(SSD1306_DrawPixel(framebuffer, 127U, 63U, true));
    CHECK(framebuffer[SSD1306_FRAMEBUFFER_SIZE - 1U] == 0x80U);
    CHECK(SSD1306_DrawPixel(framebuffer, 127U, 63U, false));
    CHECK(framebuffer[SSD1306_FRAMEBUFFER_SIZE - 1U] == 0U);
    CHECK(!SSD1306_DrawPixel(framebuffer, 128U, 0U, true));

    SSD1306_DrawLine(framebuffer, 0, 0, 7, 7, true);
    CHECK(framebuffer[0] == 0x01U);
    CHECK(framebuffer[7] == 0x80U);
    CHECK(SSD1306_DrawChar6x8(framebuffer, 10U, 16U, 'A', true));
    CHECK(SSD1306_DrawString6x8(
        framebuffer, 0U, 48U, "OK", true) == 2U);
    CHECK(!SSD1306_DrawChar6x8(framebuffer, 123U, 0U, 'A', true));

    CHECK(SSD1306_Update(&display, framebuffer) ==
        SSD1306_STATUS_NOT_INITIALIZED);
    CHECK(SSD1306_Init(&display, &config) == SSD1306_STATUS_OK);
    CHECK(display.initialized);
    CHECK(bus.calls > 0U);
    calls_before_update = bus.calls;
    CHECK(SSD1306_Update(&display, framebuffer) == SSD1306_STATUS_OK);
    CHECK(bus.calls == calls_before_update + (2U * SSD1306_PAGE_COUNT));
    CHECK(bus.controls[calls_before_update] == 0x00U);
    CHECK(bus.counts[calls_before_update] == 3U);
    CHECK(bus.first_bytes[calls_before_update] == 0xB0U);
    CHECK(bus.controls[calls_before_update + 1U] == 0x40U);
    CHECK(bus.counts[calls_before_update + 1U] == SSD1306_WIDTH);

    bus.fail = true;
    CHECK(SSD1306_SetInverse(&display, true) == SSD1306_STATUS_IO_ERROR);
    (void) puts("SSD1306 PC test: PASS");
    return 0;
}
