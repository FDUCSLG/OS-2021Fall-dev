#include <core/console.h>

static ConsoleContext ctx;

void init_console() {
    init_spinlock(&ctx.lock);
    ctx.device = get_uart_char_device();
}

void kernel_puts(const char *str) {
    acquire_spinlock(&ctx.lock);

    while (*str != '\0') {
        ctx.device.put(*str++);
    }

    // add a trailing newline.
    ctx.device.put('\n');

    release_spinlock(&ctx.lock);
}
